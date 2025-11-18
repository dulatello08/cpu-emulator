//
// alu.sv
// NeoCore 16x32 CPU - Arithmetic Logic Unit
//
// Performs arithmetic and logic operations on 16-bit operands.
// Supports all ALU operations defined in the ISA.
// Generates zero (Z) and overflow (V) flags.
//

module alu
  import neocore_pkg::*;
(
  input  logic        clk,      // Not used, but kept for consistency
  input  logic        rst,      // Not used, but kept for consistency
  
  // Operands
  input  logic [15:0] operand_a,
  input  logic [15:0] operand_b,
  input  alu_op_e     alu_op,
  
  // Results
  output logic [31:0] result,   // Result (may be 32-bit for overflow detection)
  output logic        z_flag,   // Zero flag
  output logic        v_flag    // Overflow flag
);

  // Suppress unused warnings for clock/reset
  logic unused;
  assign unused = ^{clk, rst};

  // ============================================================================
  // ALU Operation Logic
  // ============================================================================
  
  logic [31:0] result_int;
  logic [31:0] operand_a_ext;
  logic [31:0] operand_b_ext;
  
  always_comb begin
    // Default values
    result_int = 32'h0000_0000;
    operand_a_ext = {16'h0000, operand_a};
    operand_b_ext = {16'h0000, operand_b};
    
    case (alu_op)
      // Addition
      ALU_ADD: begin
        result_int = operand_a_ext + operand_b_ext;
      end
      
      // Subtraction
      ALU_SUB: begin
        // Note: C emulator returns 0 for negative results
        if (operand_a >= operand_b) begin
          result_int = operand_a_ext - operand_b_ext;
        end else begin
          result_int = 32'h0000_0000;
        end
      end
      
      // Multiplication (truncated to 16 bits)
      ALU_MUL: begin
        result_int = operand_a_ext * operand_b_ext;
      end
      
      // Bitwise AND
      ALU_AND: begin
        result_int = operand_a_ext & operand_b_ext;
      end
      
      // Bitwise OR
      ALU_OR: begin
        result_int = operand_a_ext | operand_b_ext;
      end
      
      // Bitwise XOR
      ALU_XOR: begin
        result_int = operand_a_ext ^ operand_b_ext;
      end
      
      // Left Shift
      ALU_LSH: begin
        // Shift amount comes from lower bits of operand_b
        result_int = (operand_a_ext << operand_b[4:0]) & 32'hFFFF_FFFF;
      end
      
      // Right Shift (logical)
      ALU_RSH: begin
        result_int = (operand_a_ext >> operand_b[4:0]) & 32'hFFFF_FFFF;
      end
      
      // Pass through operand A
      ALU_PASS: begin
        result_int = operand_a_ext;
      end
      
      // No operation
      default: begin
        result_int = 32'h0000_0000;
      end
    endcase
  end
  
  // ============================================================================
  // Flag Generation
  // ============================================================================
  
  always_comb begin
    // Zero flag: Set if lower 16 bits of result are zero
    z_flag = (result_int[15:0] == 16'h0000);
    
    // Overflow flag: Set if result exceeds 16 bits
    v_flag = (result_int > 32'h0000_FFFF);
  end
  
  // ============================================================================
  // Output Assignment
  // ============================================================================
  
  assign result = result_int;

endmodule : alu
