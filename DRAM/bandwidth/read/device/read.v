/******************************************************************************/
/* A evaluation module of bandwidth of memory load access    Ryohei Kobayashi */
/*                                                         Version 2018-04-14 */
/******************************************************************************/
`default_nettype none
  
/***** A control logic of memory load access from an RTL module in OpenCL *****/
/******************************************************************************/
module DRAM_READ #(parameter                             MAXBURST_LOG   = 4, 
                   parameter                             READNUM_SIZE   = 32, // how many data in 512 bit are loaded (log scale)
                   parameter                             DRAM_ADDRSPACE = 64,
                   parameter                             DRAM_DATAWIDTH = 512)
                  (input  wire                           CLK,
                   input  wire                           RST,
                   ////////// User logic interface ports ///////////////
                   input  wire                           READ_REQ,
                   input  wire [DRAM_ADDRSPACE-1     :0] READ_INITADDR,
                   input  wire [READNUM_SIZE         :0] READ_NUM,
                   output wire [DRAM_DATAWIDTH-1     :0] READ_DATA,
                   output wire                           READ_DATAEN,
                   output wire                           READ_RDY,
                   ////////// Avalon-MM interface ports for read ///////
                   input  wire [DRAM_DATAWIDTH-1     :0] AVALON_MM_READDATA,
                   input  wire                           AVALON_MM_READDATAVALID,
                   input  wire                           AVALON_MM_WAITREQUEST,
                   output wire [DRAM_ADDRSPACE-1     :0] AVALON_MM_ADDRESS,
                   output wire                           AVALON_MM_READ,
                   output wire                           AVALON_MM_WRITE,      // unused
                   input  wire                           AVALON_MM_WRITEACK,   // unused
                   output wire [DRAM_DATAWIDTH-1     :0] AVALON_MM_WRITEDATA,  // unused
                   output wire [(DRAM_DATAWIDTH>>3)-1:0] AVALON_MM_BYTEENABLE,
                   output wire [MAXBURST_LOG         :0] AVALON_MM_BURSTCOUNT);

  localparam MAXBURST_NUM  = (1 << MAXBURST_LOG);
  localparam ACCESS_STRIDE = ((DRAM_DATAWIDTH>>3) << MAXBURST_LOG);

  reg [1:0]                         state;
  reg                               busy;
  reg [DRAM_ADDRSPACE-1:0]          address;
  reg                               read_request;
  reg [MAXBURST_LOG:0]              burstcount;
  reg [MAXBURST_LOG:0]              last_burstcount;
  reg [READNUM_SIZE-MAXBURST_LOG:0] burstnum;  // # of burst accesses operated
  
  // state machine for read
  always @(posedge CLK) begin
    if (RST) begin
      state           <= 0;
      busy            <= 0;
      address         <= 0;
      read_request    <= 0;
      burstcount      <= 0;
      last_burstcount <= 0;
      burstnum        <= 0;
    end else begin
      case (state)
        ///// wait read request /////
        0: begin
          if (READ_REQ) begin
            state           <= 1;
            busy            <= 1;
            address         <= READ_INITADDR;
            last_burstcount <= (READ_NUM[MAXBURST_LOG-1:0] == 0) ? MAXBURST_NUM : {1'b0, READ_NUM[MAXBURST_LOG-1:0]};
            burstnum        <= (READ_NUM + (MAXBURST_NUM-1)) >> MAXBURST_LOG;
          end
        end
        ///// send read request /////
        1: begin
          state        <= 2;
          read_request <= 1;
          burstcount   <= (burstnum == 1) ? last_burstcount : MAXBURST_NUM;
        end
        ///// read transfer     /////
        2: begin
          if (!AVALON_MM_WAITREQUEST) begin
            state        <= (burstnum == 1) ? 0 : 1;
            busy         <= (burstnum != 1);
            address      <= address + ACCESS_STRIDE;
            read_request <= 0;
            burstnum     <= burstnum - 1;
          end
        end
      endcase
    end
  end

  // Output to user logic interface
  assign READ_DATA            = AVALON_MM_READDATA;
  assign READ_DATAEN          = AVALON_MM_READDATAVALID;
  assign READ_RDY             = ~busy;

  // Output to Avalon-MM interface
  assign AVALON_MM_ADDRESS    = address;
  assign AVALON_MM_READ       = read_request;
  assign AVALON_MM_WRITE      = 0;
  assign AVALON_MM_WRITEDATA  = 0;
  assign AVALON_MM_BYTEENABLE = {(DRAM_DATAWIDTH>>3){1'b1}};
  assign AVALON_MM_BURSTCOUNT = burstcount;

endmodule


/*****  main module                                                       *****/
/******************************************************************************/
module read(input  wire         clock,
            input  wire         resetn,
            /* mapped to arguments from cl code */
            input  wire [ 63:0] m_src_addr,      // X (pointer)
            input  wire [ 31:0] m_input_index,   // N
            output wire [ 31:0] m_output_value,  // Y[i]
            /* Avalon-ST Interface */
            output reg          m_ready_out,
            input  wire         m_valid_in,
            output reg          m_valid_out,
            input  wire         m_ready_in,
            /* Avalon-MM Interface for read */
            input  wire [511:0] src_readdata,
            input  wire         src_readdatavalid,
            input  wire         src_waitrequest,
            output wire [ 31:0] src_address,
            output wire         src_read,
            output wire         src_write,
            input  wire         src_writeack,
            output wire [511:0] src_writedata,
            output wire [ 63:0] src_byteenable,
            output wire [  4:0] src_burstcount);

  localparam WIDTH            = 32;
  localparam ELEMS_PER_ACCESS = (512/WIDTH);
  
  wire              CLK;
  wire              RST;
  wire              start;
  reg  [ 31:0]      cycle;
  reg               finish;
  reg  [WIDTH-1:0]  check_value;
  reg               is_error;
  reg               returned;
  reg  [  1:0]      state;
  reg               request;
  reg  [ 31:0]      init_raddr;
  reg  [ 31:0]      datanum;
  wire [511:0]      dot;
  wire              doten;
  wire              ready;

  assign CLK            = clock;
  assign RST            = ~resetn;
  assign start          = &{m_ready_out, m_valid_in};
  assign m_output_value = (~is_error) ? cycle : 0;

  DRAM_READ #(4, 31, 32, 512)
  dram_read(CLK,
            RST, 
            ////////// User logic interface ///////////////
            request,
            init_raddr,
            datanum, 
            dot,
            doten,
            ready,
            ////////// Avalon-MM interface  ///////////////
            src_readdata,
            src_readdatavalid,
            src_waitrequest,
            src_address,
            src_read,
            src_write,
            src_writeack,
            src_writedata,
            src_byteenable,
            src_burstcount);

  // counter
  always @(posedge CLK) begin
    if (RST || start) begin
      cycle  <= 0;
      finish <= 0;
    end else begin
      if (!finish)                  cycle  <= cycle + 1;
      if (&{(datanum == 1), doten}) finish <= 1;
    end
  end

  // read value verification
  always @(posedge CLK) begin
    if      (RST || start) check_value <= 1;
    else if (doten)        check_value <= check_value + ELEMS_PER_ACCESS;
  end
  always @(posedge CLK) begin
    if      (RST || start)                              is_error <= 0;
    else if (&{(dot[WIDTH-1:0] != check_value), doten}) is_error <= 1;
  end
  
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
      init_raddr <= 0;
      datanum    <= 0;
    end else begin
      case (state)
        0: begin
          if (start) begin
            state      <= 1;
            request    <= 1;
            init_raddr <= m_src_addr;
            datanum    <= (m_input_index + (ELEMS_PER_ACCESS-1)) >> 4;  // 4 is parameter
          end
        end
        1: begin
          state   <= 2;
          request <= 0;
        end
        2: begin
          if (finish) state   <= 0;
          if (doten)  datanum <= datanum - 1;
        end
      endcase
    end
  end
  
endmodule

`default_nettype wire
