/******************************************************************************/
/* An evaluation module of cycle counter integrated into BSP Ryohei Kobayashi */
/*                                                         Version 2018-08-29 */
/******************************************************************************/
`default_nettype none

/*****  main module                                                       *****/
/******************************************************************************/
module wait_func #(parameter               WIDTH = 64)
                  (
                   /* global clock and low-actived reset */
                   input  wire             clock,
                   input  wire             resetn,
                   /* mapped to arguments from cl code */
                   input  wire [WIDTH-1:0] m_input_value,
                   output wire [WIDTH-1:0] m_output_value,
                   /* Avalon-ST Interface */
                   output reg              m_ready_out,
                   input  wire             m_valid_in,
                   output reg              m_valid_out,
                   input  wire             m_ready_in
                   );

  wire            CLK;
  wire            RST;
  wire            start;

  reg [WIDTH-1:0] wait_cycles;
  reg [WIDTH-1:0] cycle;
  reg             finish;
  reg             returned;
  
  assign CLK            = clock;
  assign RST            = ~resetn;
  assign start          = &{m_ready_out, m_valid_in};
  assign m_output_value = cycle;

  // counter
  always @(posedge CLK) begin
    if (RST) begin
      wait_cycles <= 0;
      cycle       <= 0;
      finish      <= 0;
    end else if (start) begin
      wait_cycles <= m_input_value;
      cycle       <= 0;
      finish      <= 0;
    end else begin
      cycle <= cycle + 1;
      if (cycle == wait_cycles) begin
        finish <= 1;
      end
    end
  end

  // return flag
  always @(posedge CLK) begin
    if (RST) begin
      m_ready_out <= 1;
      m_valid_out <= 0;
      returned    <= 0;
    end else if (start) begin
      m_ready_out <= 0;
      m_valid_out <= 0;
      returned    <= 0;
    end else begin
      if (&{m_valid_out, m_ready_in}) begin
        m_ready_out <= 1;
        m_valid_out <= 0;
        returned    <= 1;
      end else begin
        m_valid_out <= (&{finish, ~returned}); 
      end
    end
  end
  
endmodule

`default_nettype wire
