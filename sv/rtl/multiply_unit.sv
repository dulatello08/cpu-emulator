//
// multiply_unit.sv
// NeoCore 16x32 CPU - Multiply Unit
//
// Performs 16x16 → 32-bit multiplication.
// Supports both unsigned (UMULL) and signed (SMULL) multiplication.
// Results are split into lower and upper 16 bits.
//

module multiply_unit
  import neocore_pkg::*;
(
  input  logic        clk,      // Not used, but kept for consistency
  input  logic        rst,      // Not used, but kept for consistency
  
  // Operands
  input  logic [15:0] operand_a,
  input  logic [15:0] operand_b,
  input  logic        is_signed,  // 1 = signed, 0 = unsigned
  
  // Results
  output logic [15:0] result_lo,  // Lower 16 bits
  output logic [15:0] result_hi   // Upper 16 bits
);

  // Suppress unused warnings for clock/reset
  logic unused;
  assign unused = ^{clk, rst};

  // ============================================================================
  // Multiplication Logic
  // ============================================================================
  
  logic [31:0] product;
  
  always_comb begin
    if (is_signed) begin
      // Signed multiplication
      // Cast to signed, multiply, then cast back to unsigned for storage
      logic signed [15:0] signed_a;
      logic signed [15:0] signed_b;
      logic signed [31:0] signed_product;
      
      signed_a = signed'(operand_a);
      signed_b = signed'(operand_b);
      signed_product = signed_a * signed_b;
      product = unsigned'(signed_product);
    end else begin
      // Unsigned multiplication
      product = {16'h0000, operand_a} * {16'h0000, operand_b};
    end
  end
  
  // ============================================================================
  // Result Splitting
  // ============================================================================
  
  assign result_lo = product[15:0];   // Lower 16 bits
  assign result_hi = product[31:16];  // Upper 16 bits

endmodule : multiply_unit
