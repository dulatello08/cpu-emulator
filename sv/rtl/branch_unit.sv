//
// branch_unit.sv
// NeoCore 16x32 CPU - Branch Unit
//
// Evaluates branch conditions and determines if a branch should be taken.
// Handles all conditional branch instructions (BE, BNE, BLT, BGT, BRO).
// Unconditional branches (B, JSR) are always taken.
//

module branch_unit
  import neocore_pkg::*;
(
  input  logic        clk,      // Not used, but kept for consistency
  input  logic        rst,      // Not used, but kept for consistency
  
  // Branch instruction information
  input  opcode_e     opcode,
  input  logic [15:0] operand_a,  // First comparison operand (rs1)
  input  logic [15:0] operand_b,  // Second comparison operand (rs2)
  input  logic        v_flag_in,  // Current overflow flag (for BRO)
  input  logic [31:0] branch_target,  // Target address
  
  // Branch decision
  output logic        branch_taken,
  output logic [31:0] branch_pc
);

  // Suppress unused warnings for clock/reset
  logic unused;
  assign unused = ^{clk, rst};

  // ============================================================================
  // Branch Condition Evaluation
  // ============================================================================
  
  always_comb begin
    // Default: no branch
    branch_taken = 1'b0;
    branch_pc = branch_target;
    
    case (opcode)
      // Unconditional branch
      OP_B: begin
        branch_taken = 1'b1;
      end
      
      // Branch if equal
      OP_BE: begin
        branch_taken = (operand_a == operand_b);
      end
      
      // Branch if not equal
      OP_BNE: begin
        branch_taken = (operand_a != operand_b);
      end
      
      // Branch if less than (unsigned comparison)
      OP_BLT: begin
        branch_taken = (operand_a < operand_b);
      end
      
      // Branch if greater than (unsigned comparison)
      OP_BGT: begin
        branch_taken = (operand_a > operand_b);
      end
      
      // Branch if overflow flag is set
      OP_BRO: begin
        branch_taken = v_flag_in;
      end
      
      // Jump to subroutine (always taken)
      OP_JSR: begin
        branch_taken = 1'b1;
      end
      
      default: begin
        branch_taken = 1'b0;
      end
    endcase
  end

endmodule : branch_unit
