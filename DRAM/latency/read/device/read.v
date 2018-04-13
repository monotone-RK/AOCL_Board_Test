/******************************************************************************/
/* A evaluation module of latency of memory load access      Ryohei Kobayashi */
/*                                                         Version 2018-04-13 */
/******************************************************************************/
`default_nettype none

`define WIDTH (32)

/*****  main module                                                       *****/
/******************************************************************************/
module read(input  wire              clock,
            input  wire              resetn,
            /* mapped to arguments from cl code */
            input  wire [      63:0] m_src_addr,      // X
            input  wire [      31:0] m_input_index,   // index
            input  wire [`WIDTH-1:0] m_input_value,   // value
            output wire [      31:0] m_output_value,  // cycle
            /* Avalon-ST Interface */
            output reg               m_ready_out,
            input  wire              m_valid_in,
            output reg               m_valid_out,
            input  wire              m_ready_in,
            /* Avalon-MM Interface for read */
            input  wire [     511:0] src_readdata,
            input  wire              src_readdatavalid,
            input  wire              src_waitrequest,
            output reg  [      31:0] src_address,
            output reg               src_read,
            output wire              src_write,
            input  wire              src_writeack,
            output wire [     511:0] src_writedata,
            output wire [      63:0] src_byteenable,
            output wire [       4:0] src_burstcount);

  wire             CLK;
  wire             RST;
  wire             start;

  reg [31:0]       cycle;
  reg              finish;
  reg [`WIDTH-1:0] expected_value;
  reg [`WIDTH-1:0] read_value;
  reg              is_match;
  reg              returned;
  reg              state;
  
  assign CLK            = clock;
  assign RST            = ~resetn;
  assign start          = &{m_ready_out, m_valid_in};
  assign m_output_value = (is_match) ? cycle : 0;
  assign src_write      = 0;
  assign src_writedata  = 0;
  assign src_byteenable = {(64){1'b1}};
  assign src_burstcount = 1;

  // counter
  always @(posedge CLK) begin
    if (RST || start) begin
      cycle  <= 0;
      finish <= 0;
    end else begin
      if (~|{src_readdatavalid,finish}) cycle  <= cycle + 1;
      else                              finish <= 1;
    end
  end

  // read value verification
  always @(posedge CLK) begin
    if (RST) begin
      expected_value <= 0;
      read_value     <= 0;
      is_match       <= 0;
    end else if (start) begin
      expected_value <= m_input_value;
      read_value     <= ~m_input_value;  // make read_value different from expected value
      is_match       <= 0;
    end else begin
      if (src_readdatavalid) read_value <= src_readdata[`WIDTH-1:0];
      if (finish)            is_match   <= (expected_value == read_value);
    end
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

  // state machine for read
  always @(posedge CLK) begin
    if (RST) begin
      state       <= 0;
      src_address <= 0;
      src_read    <= 0;
    end else begin
      case (state)
        0: begin
          if (start) begin
            state       <= 1;
            src_address <= m_src_addr + (`WIDTH >> 3) * m_input_index;
            src_read    <= 1;
          end
        end
        1: begin
          if (!src_waitrequest) begin
            state       <= 0;
            src_address <= 0;
            src_read    <= 0;
          end
        end
      endcase
    end
  end
  
endmodule  

`default_nettype wire

// module read(input  wire         clock,
//             input  wire         resetn,
//             /* mapped to arguments from cl code */
//             input  wire [ 31:0] m_src_addr,      // *X
//             input  wire [ 31:0] m_input_index,   // N
//             output reg  [ 31:0] m_output_value,  // Y[i]
//             /* Avalon-ST Interface */
//             output wire         m_ready_out,
//             input  wire         m_valid_in,
//             output reg          m_valid_out,
//             input  wire         m_ready_in,
//             /* Avalon-MM Interface for read */
//             input  wire [511:0] src_readdata,
//             input  wire         src_readdatavalid,
//             input  wire         src_waitrequest,
//             output reg  [ 31:0] src_address,
//             output reg          src_read,
//             output wire         src_write,
//             input  wire         src_writeack,
//             output wire [511:0] src_writedata,
//             output wire [ 63:0] src_byteenable,
//             output reg  [  4:0] src_burstcount);

//   reg [27:0] pre_src_count;
//   reg [23:0] src_count;
//   reg [ 4:0] last_burstcount;
//   reg        pos_start;
//   wire       start = m_ready_out & m_valid_in;

//   assign m_ready_out    = 1'b1;
//   assign src_byteenable = 64'hffff_ffff_ffff_ffff;
//   assign src_write      = 1'b0;
//   assign src_writedata  = 512'b0;

//   // pre_src_count
//   always @(posedge clock) begin
//     if      (!resetn)           pre_src_count <= 28'd0;
//     else if (start)             pre_src_count <= (m_input_index + 4'b1111) >> 4;
//     else if (src_readdatavalid) pre_src_count <= pre_src_count - 1'b1;
//   end

//   // m_output_value
//   always @(posedge clock) begin
//     if      (!resetn)           m_output_value <= 32'd0;
//     else if (start)             m_output_value <= 32'd0;
//     else if (src_readdatavalid) m_output_value <= m_output_value + src_readdata[31:0];
//   end

//   // m_valid_out
//   always @(posedge clock) begin
//     if      (!resetn)                                 m_valid_out <= 1'd0;
//     else if (pre_src_count == 1 && src_readdatavalid) m_valid_out <= 1'b1;
//     else if (m_valid_out & m_ready_in)                m_valid_out <= 1'b0;
//   end

//   reg [3:0] rState;
//   // state machine for read
//   always @(posedge clock) begin
//     if (!resetn) begin
//       rState          <= 0;
//       src_address     <= 0;
//       src_count       <= 0;
//       last_burstcount <= 0;
//       src_burstcount  <= 0;
//       src_read        <= 0;
//     end else begin
//       case (rState)
//         0: begin  // set init values
//           if (start) begin
//             rState         <= 1;
//             src_address    <= m_src_addr;
//           end
//         end
//         1: begin  // calculate src_count and last_burstcount
//           rState          <= 2;
//           src_count       <= (pre_src_count + 4'b1111) >> 4;
//           last_burstcount <= (pre_src_count[3:0] == 4'b0000) ? 16 : {1'b0, pre_src_count[3:0]};
//         end
//         2: begin  // send read request
//           if (src_count != 0) begin
//             rState   <= 3;
//             src_read <= 1;
//           end
//           src_burstcount <= (src_count == 1) ? last_burstcount : 16;
//         end
//         3: begin  // read transfer
//           if (!src_waitrequest) begin
//             rState      <= (src_count == 1) ? 0 : 2;
//             src_address <= src_address + 1024;
//             src_count   <= src_count - 1;
//             src_read    <= 0;
//           end
//         end
//       endcase
//     end
//   end

// endmodule 
// module read(input  wire         clock,
//             input  wire         resetn,
//             /* mapped to arguments from cl code */
//             input  wire [ 63:0] m_src_addr,      // *X
//             input  wire [ 31:0] m_input_index,   // N
//             output reg  [ 31:0] m_output_value,  // Y[i]
//             /* Avalon-ST Interface */
//             output wire         m_ready_out,
//             input  wire         m_valid_in,
//             output reg          m_valid_out,
//             input  wire         m_ready_in,
//             /* Avalon-MM Interface for read */
//             input  wire [511:0] src_readdata,
//             input  wire         src_readdatavalid,
//             input  wire         src_waitrequest,
//             output reg  [ 31:0] src_address,
//             output reg          src_read,
//             output wire         src_write,
//             input  wire         src_writeack,
//             output wire [511:0] src_writedata,
//             output wire [ 63:0] src_byteenable,
//             output reg  [  4:0] src_burstcount);

//   reg [27:0] pre_src_count;
//   reg [23:0] src_count;
//   reg [ 4:0] last_burstcount;
//   reg        pos_start;
//   wire       start = m_ready_out & m_valid_in;

//   assign m_ready_out    = 1'b1;
//   assign src_byteenable = 64'hffff_ffff_ffff_ffff;
//   assign src_write      = 1'b0;
//   assign src_writedata  = 512'b0;

//   // pos_start
//   always @(posedge clock) begin
//     if (!resetn) pos_start <= 1'b0;
//     else         pos_start <= start;
//   end

//   // last_burstcount
//   always @(posedge clock) begin
//     if      (!resetn)                                      last_burstcount <= 5'd0;
//     // else if (pos_start && (pre_src_count[4:0] == 5'b10000)) last_burstcount <= 5'b10000;
//     else if (pos_start && (pre_src_count[3:0] == 4'b0000)) last_burstcount <= 5'b10000;
//     else if (pos_start)                                    last_burstcount <= {1'b0, pre_src_count[3:0]};
//   end

//   // pre_src_count
//   always @(posedge clock) begin
//     if      (!resetn)           pre_src_count <= 28'd0;
//     else if (start)             pre_src_count <= (m_input_index + 4'b1111) >> 4;
//     else if (src_readdatavalid) pre_src_count <= pre_src_count - 1'b1;
//   end

//   // src_count
//   always @(posedge clock) begin
//     if      (!resetn)                      src_count <= 24'd0;
//     else if (pos_start)                    src_count <= (pre_src_count + 4'b1111) >> 4;
//     else if (!src_waitrequest && src_read) src_count <= src_count - 1'b1;
//   end

//   // src_address
//   always @(posedge clock) begin
//     if      (!resetn)                      src_address <= 32'd0;
//     else if (start)                        src_address <= m_src_addr;
//     else if (!src_waitrequest && src_read) src_address <= src_address + 11'd1024;
//   end

//   // src_burstcount
//   always @(posedge clock) begin
//     if      (!resetn)            src_burstcount <= 5'd0;
//     else if (src_count == 24'd1) src_burstcount <= last_burstcount;
//     else                         src_burstcount <= 5'b10000;
//   end
  
//   // src_read
//   always @(posedge clock) begin
//     if      (!resetn)                      src_read <= 1'd0;
//     else if (!src_waitrequest && src_read) src_read <= 1'b0;
//     else if (src_count != 24'd0)           src_read <= 1'b1;
//   end
  
//   // m_output_value
//   always @(posedge clock) begin
//     if      (!resetn)           m_output_value <= 32'd0;
//     else if (src_readdatavalid) m_output_value <= m_output_value + src_readdata[31:0];
//   end

//   // m_valid_out
//   always @(posedge clock) begin
//     if      (!resetn)                                     m_valid_out <= 1'd0;
//     else if (pre_src_count == 28'b1 && src_readdatavalid) m_valid_out <= 1'b1;
//     else if (m_valid_out & m_ready_in)                    m_valid_out <= 1'b0;
//   end

// endmodule 
