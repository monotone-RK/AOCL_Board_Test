/******************************************************************************/
/* A evaluation module of bandwidth of memory store access   Ryohei Kobayashi */
/*                                                         Version 2018-04-14 */
/******************************************************************************/
`default_nettype none

/***** A control logic of memory store access from an RTL module in OpenCL ****/
/******************************************************************************/
module DRAM_WRITE #(parameter                             MAXBURST_LOG   = 4, 
                    parameter                             WRITENUM_SIZE  = 32, // how many data in 512 bit are stored (log scale)
                    parameter                             DRAM_ADDRSPACE = 64,
                    parameter                             DRAM_DATAWIDTH = 512)
                   (input  wire                           CLK,
                    input  wire                           RST,
                    ////////// User logic interface ports ///////////////
                    input  wire                           WRITE_REQ,
                    input  wire [DRAM_ADDRSPACE-1     :0] WRITE_INITADDR,
                    input  wire [WRITENUM_SIZE        :0] WRITE_NUM,
                    input  wire [DRAM_DATAWIDTH-1     :0] WRITE_DATA,
                    output wire                           WRITE_DATA_ACCEPTABLE,
                    output wire                           WRITE_RDY,
                    output wire                           WRITE_REQ_DONE,
                    ////////// Avalon-MM interface ports for write //////
                    input  wire [DRAM_DATAWIDTH-1     :0] AVALON_MM_READDATA,      // unused
                    input  wire                           AVALON_MM_READDATAVALID, // unused
                    input  wire                           AVALON_MM_WAITREQUEST,
                    output wire [DRAM_ADDRSPACE-1     :0] AVALON_MM_ADDRESS,
                    output wire                           AVALON_MM_READ,          // unused
                    output wire                           AVALON_MM_WRITE,
                    input  wire                           AVALON_MM_WRITEACK,
                    output wire [DRAM_DATAWIDTH-1     :0] AVALON_MM_WRITEDATA,
                    output wire [(DRAM_DATAWIDTH>>3)-1:0] AVALON_MM_BYTEENABLE,
                    output wire [MAXBURST_LOG         :0] AVALON_MM_BURSTCOUNT);

  localparam MAXBURST_NUM  = (1 << MAXBURST_LOG);
  localparam ACCESS_STRIDE = ((DRAM_DATAWIDTH>>3) << MAXBURST_LOG);

  reg [1:0]                          state;
  reg                                busy;
  reg [DRAM_ADDRSPACE-1:0]           address;
  reg                                write_request;
  reg [MAXBURST_LOG:0]               remaining_datanum;
  reg [MAXBURST_LOG:0]               burstcount;
  reg [MAXBURST_LOG:0]               last_burstcount;
  reg [WRITENUM_SIZE-MAXBURST_LOG:0] burstnum;  // # of burst accesses operated
  
  // state machine for read
  always @(posedge CLK) begin
    if (RST) begin
      state             <= 0;
      busy              <= 0;
      address           <= 0;
      write_request     <= 0;
      remaining_datanum <= 0;
      burstcount        <= 0;
      last_burstcount   <= 0;
      burstnum          <= 0;
    end else begin
      case (state)
        ///// wait write request /////
        0: begin
          if (WRITE_REQ) begin
            state           <= 1;
            busy            <= 1;
            address         <= WRITE_INITADDR;
            last_burstcount <= (WRITE_NUM[MAXBURST_LOG-1:0] == 0) ? MAXBURST_NUM : {1'b0, WRITE_NUM[MAXBURST_LOG-1:0]};
            burstnum        <= (WRITE_NUM + (MAXBURST_NUM-1)) >> MAXBURST_LOG;
          end
        end
        ///// send write request /////
        1: begin
          state             <= 2;
          write_request     <= 1;
          remaining_datanum <= (burstnum == 1) ? last_burstcount : MAXBURST_NUM;
          burstcount        <= (burstnum == 1) ? last_burstcount : MAXBURST_NUM;
        end
        ///// write transfer     /////
        2: begin
          if (!AVALON_MM_WAITREQUEST) begin
            remaining_datanum <= remaining_datanum - 1;
            if (remaining_datanum == 1) begin
              state         <= (burstnum == 1) ? 3 : 1;
              address       <= address + ACCESS_STRIDE;
              write_request <= 0;
              burstnum      <= burstnum - 1;
            end
          end
        end
        ///// wait writeack     //////
        3: begin
          if (AVALON_MM_WRITEACK) begin
            state <= 0;
            busy  <= 0;
          end
        end
      endcase
    end
  end

  // Output to user logic interface
  assign WRITE_DATA_ACCEPTABLE = &{~AVALON_MM_WAITREQUEST, write_request};
  assign WRITE_RDY             = ~busy;
  assign WRITE_REQ_DONE        = &{(state == 3), AVALON_MM_WRITEACK};
  
  // Output to Avalon-MM interface
  assign AVALON_MM_ADDRESS     = address;
  assign AVALON_MM_READ        = 0;
  assign AVALON_MM_WRITE       = write_request;
  assign AVALON_MM_WRITEDATA   = WRITE_DATA;
  assign AVALON_MM_BYTEENABLE  = {(DRAM_DATAWIDTH>>3){1'b1}};
  assign AVALON_MM_BURSTCOUNT  = burstcount;
  
endmodule


/***** A control logic of memory load access from an RTL module in OpenCL *****/
/******************************************************************************/
module write(input  wire         clock,
             input  wire         resetn,
             /* mapped to arguments from cl code */
             input  wire [ 63:0] m_dst_addr,       // *Y
             input  wire [ 31:0] m_input_index,    // N
             output wire [ 31:0] m_output_value,   // tmp
             /* Avalon-ST Interface */
             output reg          m_ready_out,
             input  wire         m_valid_in,
             output reg          m_valid_out,
             input  wire         m_ready_in,
             /* Avalon-MM Interface for write */
             input  wire [511:0] dst_readdata,
             input  wire         dst_readdatavalid,
             input  wire         dst_waitrequest,
             output wire [ 31:0] dst_address,
             output wire         dst_read,
             output wire         dst_write,
             input  wire         dst_writeack,
             output wire [511:0] dst_writedata,
             output wire [ 63:0] dst_byteenable,
             output wire [  4:0] dst_burstcount);

  localparam WIDTH            = 32;
  localparam ELEMS_PER_ACCESS = (512/WIDTH);

  wire              CLK;
  wire              RST;
  wire              start;
  reg  [ 31:0]      cycle;
  reg               finish;
  reg               returned;
  reg  [  1:0]      state;
  reg               request;
  reg  [ 31:0]      init_waddr;
  reg  [ 31:0]      datanum;
  wire [511:0]      din;
  wire              din_acceptable;
  wire              ready;
  wire              request_done;

  assign CLK            = clock;
  assign RST            = ~resetn;
  assign start          = &{m_ready_out, m_valid_in};
  assign m_output_value = cycle;
  
  DRAM_WRITE #(4, 31, 32, 512)
  dram_write(CLK,
             RST, 
            ////////// User logic interface ///////////////
             request,
             init_waddr,
             datanum,
             din,
             din_acceptable,
             ready,
             request_done,
            ////////// Avalon-MM interface  ///////////////
             dst_readdata,
             dst_readdatavalid,
             dst_waitrequest,
             dst_address,
             dst_read,
             dst_write,
             dst_writeack,
             dst_writedata,
             dst_byteenable,
             dst_burstcount);

  // counter
  always @(posedge CLK) begin
    if (RST || start) begin
      cycle  <= 0;
      finish <= 0;
    end else begin
      if (~|{request_done, finish}) cycle  <= cycle + 1;
      if (request_done)             finish <= 1;
    end
  end
    
  // din value generator
  genvar i;
  generate
    for (i=0; i<ELEMS_PER_ACCESS; i=i+1) begin: element
      reg [WIDTH-1:0] value;
      always @(posedge CLK) begin
        if      (RST || start)   value <= i;
        else if (din_acceptable) value <= value + ELEMS_PER_ACCESS;
      end
      assign din[WIDTH*(i+1)-1:WIDTH*i] = value;
    end
  endgenerate

  // return flag
  always @(posedge CLK) begin
    if (RST) begin
      returned    <= 0;
      m_ready_out <= 1;
      m_valid_out <= 0;
    end else if (start) begin
      returned    <= 0;
      m_ready_out <= 0;
      m_valid_out <= 0;
    end else begin
      if (&{m_valid_out, m_ready_in}) begin
        returned    <= 1;
        m_ready_out <= 1;
        m_valid_out <= 0;
      end else begin
        m_valid_out <= (&{finish, ~returned}); 
      end
    end
  end

  // state machine
  always @(posedge CLK) begin
    if (RST) begin
      state      <= 0;
      request    <= 0;
      init_waddr <= 0;
      datanum    <= 0;
    end else begin
      case (state)
        0: begin
          if (start) begin
            state      <= 1;
            request    <= 1;
            init_waddr <= m_dst_addr;
            datanum    <= (m_input_index + (ELEMS_PER_ACCESS-1)) >> 4;  // 4 is parameter
          end
        end
        1: begin
          state   <= 2;
          request <= 0;
        end
        2: begin
          if (finish) state <= 0;
        end
      endcase
    end
  end

endmodule

`default_nettype wire

// module write(input  wire         clock,
//              input  wire         resetn,
//              /* mapped to arguments from cl code */
//              input  wire [ 63:0] m_dst_addr,       // *Y
//              input  wire [ 31:0] m_input_index,    // N
//              output wire [ 31:0] m_output_value,   // tmp
//              /* Avalon-ST Interface */
//              output wire         m_ready_out,
//              input  wire         m_valid_in,
//              output reg          m_valid_out,
//              input  wire         m_ready_in,
//              /* Avalon-MM Interface for write */
//              input  wire [511:0] dst_readdata,
//              input  wire         dst_readdatavalid,
//              input  wire         dst_waitrequest,
//              output reg  [ 31:0] dst_address,
//              output wire         dst_read,
//              output reg          dst_write,
//              input  wire         dst_writeack,
//              output wire [511:0] dst_writedata,
//              output wire [ 63:0] dst_byteenable,
//              output reg  [  4:0] dst_burstcount);

//   reg [31:0] writedata;
//   reg [27:0] pre_dst_count;
//   reg [23:0] dst_count;
//   reg [ 4:0] last_burstcount, write_count;
//   wire       start;
//   reg        finish;
//   reg        pos_start;

//   assign m_output_value = 32'd11;
//   assign start          = m_ready_out & m_valid_in;
//   assign m_ready_out    = 1'b1;
//   assign dst_byteenable = 64'hffff_ffff_ffff_ffff;
//   assign dst_read       = 1'b0;
//   assign dst_writedata  = {writedata+32'd15, writedata+32'd14, writedata+32'd13, writedata+32'd12,
//                            writedata+32'd11, writedata+32'd10, writedata+32'd9,  writedata+32'd8,
//                            writedata+32'd7,  writedata+32'd6,  writedata+32'd5,  writedata+32'd4,
//                            writedata+32'd3,  writedata+32'd2,  writedata+32'd1,  writedata+32'd0};

//   // m_valid_out
//   always @(posedge clock) begin
//     if      (!resetn)                  m_valid_out <= 1'b0;
//     else if (m_valid_out & m_ready_in) m_valid_out <= 1'b0;
//     else if (finish & dst_writeack)    m_valid_out <= 1'b1;
//   end

//   reg [3:0] wState;
//   // state machine for read
//   always @(posedge clock) begin
//     if (!resetn) begin
//       wState          <= 0;
//       pre_dst_count   <= 0;
//       dst_address     <= 0;
//       last_burstcount <= 0;
//       dst_count       <= 0;
//       dst_write       <= 0;
//       write_count     <= 0;
//       dst_burstcount  <= 0;
//       writedata       <= 0;
//       finish          <= 0;
//     end else begin
//       case (wState)
//         0: begin
//           if (start) begin
//             wState        <= 1;
//             pre_dst_count <= (m_input_index + 4'b1111) >> 4;
//             dst_address   <= m_dst_addr;
//             writedata     <= 0;
//             finish        <= 0;
//           end
//         end
//         1: begin
//           wState          <= 2;
//           last_burstcount <= (pre_dst_count[3:0] == 4'b0000) ? 16 : {1'b0, pre_dst_count[3:0]};
//           dst_count       <= (pre_dst_count + 4'b1111) >> 4;
//         end
//         2: begin
//           if (dst_count != 0) begin
//             wState    <= 3;
//             dst_write <= 1;
//             dst_count <= dst_count - 1;
//           end
//           write_count    <= (dst_count == 1) ? last_burstcount : 16;
//           dst_burstcount <= (dst_count == 1) ? last_burstcount : 16;
//         end
//         3: begin
//           if (!dst_waitrequest) begin
//             write_count <= write_count - 1;
//             writedata   <= writedata + 16;
//             if (write_count == 1) begin
//               wState      <= (dst_count == 0) ? 0 : 2;
//               dst_write   <= 0;
//               dst_address <= dst_address + 1024;
//               finish      <= (dst_count == 0);
//             end
//           end
//         end
//       endcase
//     end
//   end

// endmodule
