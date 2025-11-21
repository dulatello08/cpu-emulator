# Execute Stage Module Reference

## Overview
The Execute Stage performs ALU operations, evaluates branch conditions, computes memory addresses, and handles multiplication. It supports dual-issue execution with two parallel execution paths.

## Module: `execute_stage`

### Key Features
- Dual execution paths (slot 0 and slot 1)
- ALU operations via integrated ALU module
- Branch condition evaluation via branch_unit
- Memory address computation
- **Fixed: MOV immediate instruction handling**

### Ports

Inputs for both instruction slots (0 and 1):
- Pipeline register inputs (`id_ex_t` struct)
- Register file operands (rs1_data, rs2_data)
- Forwarding data from memory and writeback stages

Outputs for both slots:
- Pipeline register outputs (`ex_mem_t` struct)
- Branch taken/target signals
- Forwarding data for hazard resolution

### Critical Bug Fix: MOV Immediate

**FIXED**: MOV instructions with immediate specifiers now correctly use the immediate value instead of ALU result.

**Before (WRONG)**:
```systemverilog
ex_mem_0.alu_result = alu_result_0;  // Returns 0x00000000 for MOV!
```

**After (CORRECT)**:
```systemverilog
if (id_ex_0.itype == ITYPE_MOV) begin
  if (id_ex_0.specifier == 8'h02) begin
    ex_mem_0.alu_result = {16'h0, operand_a_0};  // Register-to-register
  end else begin
    ex_mem_0.alu_result = id_ex_0.immediate;  // Immediate value
  end
end
```

### Execution Paths

**Slot 0**: Always executes when valid
**Slot 1**: Executes only when dual-issue is active

### ALU Integration

Each slot has its own ALU instance:
```systemverilog
alu alu_0 (
  .operand_a(operand_a_0),
  .operand_b(operand_b_0),
  .alu_op(id_ex_0.alu_op),
  .result(alu_result_0),
  .z_flag(alu_z_0),
  .v_flag(alu_v_0)
);
```

### Branch Evaluation

Branch unit evaluates conditions:
- BEQ, BNE: Compare register values
- BLT, BGE: Signed comparison
- Unconditional: B (always taken)

### Memory Address Computation

For load/store instructions:
- Base + offset addressing
- Register indirect
- Absolute addressing

### Usage Example

```systemverilog
execute_stage execute (
  .clk(clk),
  .rst(rst),
  .id_ex_0(id_ex_out_0),
  .rs1_data_0(rf_rs1_data_0),
  .rs2_data_0(rf_rs2_data_0),
  .mem_fwd_data_0(mem_fwd_data_0),
  .mem_fwd_valid_0(mem_fwd_valid_0),
  .wb_fwd_data_0(wb_fwd_data_0),
  .wb_fwd_valid_0(wb_fwd_valid_0),
  // ... dual-issue slot 1 inputs
  .ex_mem_0(ex_mem_in_0),
  .ex_mem_1(ex_mem_in_1),
  .branch_taken(branch_taken),
  .branch_target(branch_target),
  // ... forwarding outputs
);
```

### Implementation Notes

1. **MOV Instruction**: Special handling to use immediate for non-register specifiers
2. **Forwarding**: Supports forwarding from both MEM and WB stages
3. **Flags**: Z and V flags computed but not yet fully integrated into branch logic

### Related Modules
- `alu.sv`: Arithmetic/logic operations
- `branch_unit.sv`: Branch condition evaluation
- `multiply_unit.sv`: Multiplication (if used)
- `hazard_unit.sv`: Determines forwarding requirements
