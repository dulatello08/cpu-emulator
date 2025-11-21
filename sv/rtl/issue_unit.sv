//
// issue_unit.sv
// NeoCore 16x32 CPU - Dual-Issue Control Unit
//
// Determines which instructions can be issued together in the same cycle.
// Enforces dual-issue restrictions:
// - At most one memory operation per cycle
// - Branches issue alone
// - No structural hazards (register write port conflicts)
// - No data dependencies between the two issuing instructions
//

module issue_unit
  import neocore_pkg::*;
(
  input  logic        clk,
  input  logic        rst,
  
  // Decoded instruction 0 (from decode unit)
  input  logic        inst0_valid,
  input  itype_e      inst0_type,
  input  logic        inst0_mem_read,
  input  logic        inst0_mem_write,
  input  logic        inst0_is_branch,
  input  logic        inst0_is_halt,
  input  logic [3:0]  inst0_rd_addr,
  input  logic        inst0_rd_we,
  input  logic [3:0]  inst0_rd2_addr,
  input  logic        inst0_rd2_we,
  
  // Decoded instruction 1 (from decode unit)
  input  logic        inst1_valid,
  input  itype_e      inst1_type,
  input  logic        inst1_mem_read,
  input  logic        inst1_mem_write,
  input  logic        inst1_is_branch,
  input  logic        inst1_is_halt,
  input  logic [3:0]  inst1_rs1_addr,
  input  logic [3:0]  inst1_rs2_addr,
  input  logic [3:0]  inst1_rd_addr,
  input  logic        inst1_rd_we,
  input  logic [3:0]  inst1_rd2_addr,
  input  logic        inst1_rd2_we,
  
  // Issue decision
  output logic        issue_inst0,    // Issue instruction 0
  output logic        issue_inst1,    // Issue instruction 1 (dual-issue)
  output logic        dual_issue      // Both instructions issued this cycle
);

  // ============================================================================
  // Structural Hazard Detection
  // ============================================================================
  
  logic mem_port_conflict;
  logic write_port_conflict;
  logic branch_restriction;
  logic halt_restriction;
  logic data_dependency;
  logic mul_restriction;
  
  always_comb begin
    // Memory port conflict: both instructions need memory
    mem_port_conflict = (inst0_mem_read || inst0_mem_write) && 
                        (inst1_mem_read || inst1_mem_write);
    
    // Write port conflict: both instructions write to same register
    write_port_conflict = 1'b0;
    if (inst0_rd_we && inst1_rd_we && (inst0_rd_addr == inst1_rd_addr) && (inst0_rd_addr != 4'h0)) begin
      write_port_conflict = 1'b1;
    end
    if (inst0_rd2_we && inst1_rd_we && (inst0_rd2_addr == inst1_rd_addr) && (inst0_rd2_addr != 4'h0)) begin
      write_port_conflict = 1'b1;
    end
    if (inst0_rd_we && inst1_rd2_we && (inst0_rd_addr == inst1_rd2_addr) && (inst0_rd_addr != 4'h0)) begin
      write_port_conflict = 1'b1;
    end
    if (inst0_rd2_we && inst1_rd2_we && (inst0_rd2_addr == inst1_rd2_addr) && (inst0_rd2_addr != 4'h0)) begin
      write_port_conflict = 1'b1;
    end
    
    // Branch restriction: branches must issue alone
    branch_restriction = inst0_is_branch || inst1_is_branch;
    
    // Halt restriction: HLT must issue alone (CRITICAL FIX)
    halt_restriction = inst0_is_halt || inst1_is_halt;
    
    // Multiply restriction: UMULL/SMULL cannot dual-issue (implementation choice)
    mul_restriction = (inst0_type == ITYPE_MUL) || (inst1_type == ITYPE_MUL);
  end
  
  // ============================================================================
  // Data Dependency Detection (between inst0 and inst1)
  // ============================================================================
  
  always_comb begin
    data_dependency = 1'b0;
    
    // If inst0 writes a register that inst1 reads, there's a dependency
    if (inst0_rd_we && (inst0_rd_addr != 4'h0)) begin
      if ((inst0_rd_addr == inst1_rs1_addr) || (inst0_rd_addr == inst1_rs2_addr)) begin
        data_dependency = 1'b1;
      end
    end
    
    // Check second destination of inst0 (for 32-bit operations)
    if (inst0_rd2_we && (inst0_rd2_addr != 4'h0)) begin
      if ((inst0_rd2_addr == inst1_rs1_addr) || (inst0_rd2_addr == inst1_rs2_addr)) begin
        data_dependency = 1'b1;
      end
    end
  end
  
  // ============================================================================
  // Issue Decision Logic
  // ============================================================================
  
  always_comb begin
    // Default: try to issue both instructions
    issue_inst0 = inst0_valid;
    issue_inst1 = 1'b0;
    dual_issue = 1'b0;
    
    // If only inst0 is valid, issue it alone
    if (inst0_valid && !inst1_valid) begin
      issue_inst0 = 1'b1;
      issue_inst1 = 1'b0;
      dual_issue = 1'b0;
    end
    // If only inst1 is valid, don't issue (should not happen in normal operation)
    else if (!inst0_valid && inst1_valid) begin
      issue_inst0 = 1'b0;
      issue_inst1 = 1'b0;
      dual_issue = 1'b0;
    end
    // Both instructions valid: check if they can dual-issue
    else if (inst0_valid && inst1_valid) begin
      // Check all dual-issue restrictions
      if (mem_port_conflict || write_port_conflict || branch_restriction || 
          halt_restriction || data_dependency || mul_restriction) begin
        // Cannot dual-issue: issue only inst0
        issue_inst0 = 1'b1;
        issue_inst1 = 1'b0;
        dual_issue = 1'b0;
      end else begin
        // Can dual-issue: issue both
        issue_inst0 = 1'b1;
        issue_inst1 = 1'b1;
        dual_issue = 1'b1;
      end
    end
    // Neither valid
    else begin
      issue_inst0 = 1'b0;
      issue_inst1 = 1'b0;
      dual_issue = 1'b0;
    end
  end

endmodule : issue_unit
