# Branch Unit Module Reference

## Overview
The Branch Unit evaluates branch conditions and computes branch target addresses for control flow instructions.

## Module: `branch_unit`

### Ports

| Port | Direction | Width | Description |
|------|-----------|-------|-------------|
| `clk` | input | 1 | Clock signal (unused, for consistency) |
| `rst` | input | 1 | Reset signal (unused, for consistency) |
| `branch_cond` | input | `branch_cond_e` | Branch condition type |
| `operand_a` | input | 16 | First operand (register value) |
| `operand_b` | input | 16 | Second operand (register or immediate) |
| `pc` | input | 32 | Current program counter |
| `offset` | input | 32 | Branch offset (sign-extended) |
| `is_branch` | input | 1 | Instruction is a branch |
| `branch_taken` | output | 1 | Branch condition met |
| `branch_target` | output | 32 | Computed branch target address |

### Supported Branch Conditions

| Condition | Encoding | Description |
|-----------|----------|-------------|
| `BCOND_ALWAYS` | - | Unconditional branch (B) |
| `BCOND_EQ` | BEQ | Branch if equal (a == b) |
| `BCOND_NE` | BNE | Branch if not equal (a != b) |
| `BCOND_LT` | BLT | Branch if less than (signed) |
| `BCOND_GE` | BGE | Branch if greater or equal (signed) |
| `BCOND_NEVER` | - | Never branch |

### Behavior

#### Condition Evaluation
```systemverilog
case (branch_cond)
  BCOND_ALWAYS: cond_met = 1'b1;
  BCOND_EQ:     cond_met = (operand_a == operand_b);
  BCOND_NE:     cond_met = (operand_a != operand_b);
  BCOND_LT:     cond_met = ($signed(operand_a) < $signed(operand_b));
  BCOND_GE:     cond_met = ($signed(operand_a) >= $signed(operand_b));
  BCOND_NEVER:  cond_met = 1'b0;
  default:      cond_met = 1'b0;
endcase
```

#### Target Computation
```systemverilog
branch_target = pc + offset;  // PC-relative addressing
branch_taken = is_branch && cond_met;
```

### Usage Example

```systemverilog
branch_unit branch (
  .clk(clk),
  .rst(rst),
  .branch_cond(id_ex_0.branch_cond),
  .operand_a(operand_a_0),
  .operand_b(operand_b_0),
  .pc(id_ex_0.pc),
  .offset(id_ex_0.immediate),
  .is_branch(id_ex_0.is_branch),
  .branch_taken(branch_taken),
  .branch_target(branch_target)
);
```

### Implementation Notes

1. **Combinational Logic**: Branch evaluation is purely combinational
2. **Signed Comparison**: Uses `$signed()` for BLT/BGE
3. **PC-Relative**: All branches compute target as PC + offset
4. **Pipeline Integration**: Branch taken signal triggers fetch redirect

### Related Modules
- `execute_stage.sv`: Instantiates branch_unit
- `fetch_unit.sv`: Redirects PC on branch taken
- `core_top.sv`: Routes branch signals
