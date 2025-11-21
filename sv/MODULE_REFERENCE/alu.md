# ALU Module Reference

## Overview
The Arithmetic Logic Unit (ALU) performs 16-bit arithmetic and logic operations for the NeoCore16x32 CPU. It supports all ALU operations defined in the ISA and generates zero (Z) and overflow (V) flags.

## Module: `alu`

### Ports

| Port | Direction | Width | Description |
|------|-----------|-------|-------------|
| `clk` | input | 1 | Clock signal (kept for consistency, not actively used) |
| `rst` | input | 1 | Reset signal (kept for consistency, not actively used) |
| `operand_a` | input | 16 | First operand (16-bit) |
| `operand_b` | input | 16 | Second operand (16-bit) |
| `alu_op` | input | `alu_op_e` | ALU operation select |
| `result` | output | 32 | Result (32-bit to detect overflow) |
| `z_flag` | output | 1 | Zero flag (result == 0) |
| `v_flag` | output | 1 | Overflow flag |

### Parameters
None.

### Supported Operations

The ALU supports the following operations via the `alu_op_e` enum:

- **`ALU_ADD`**: Addition (operand_a + operand_b)
- **`ALU_SUB`**: Subtraction (operand_a - operand_b, saturates to 0 if negative)
- **`ALU_AND`**: Bitwise AND
- **`ALU_OR`**: Bitwise OR  
- **`ALU_XOR`**: Bitwise XOR
- **`ALU_LSH`**: Logical shift left
- **`ALU_RSH`**: Logical shift right
- **`ALU_PASS`**: Pass-through (result = operand_a)

### Behavior

#### Combinational Logic
The ALU is purely combinational - results are computed in the same cycle as inputs are applied.

#### Subtraction Saturation
Per the C emulator specification, subtraction returns 0 for negative results rather than wrapping:
```systemverilog
if (operand_a >= operand_b)
  result = operand_a - operand_b;
else
  result = 0;  // Saturate to zero
```

#### Flag Generation
- **Z flag**: Set when result[15:0] == 0
- **V flag**: Set when result[31:16] != 0 (overflow beyond 16 bits)

### Usage Example

```systemverilog
alu alu_inst (
  .clk(clk),
  .rst(rst),
  .operand_a(16'h1234),
  .operand_b(16'h5678),
  .alu_op(ALU_ADD),
  .result(alu_result),  // 32'h000068AC
  .z_flag(z),           // 0
  .v_flag(v)            // 0
);
```

### Implementation Notes

1. **32-bit Result**: The result is 32 bits to allow detection of overflow/carry beyond the 16-bit operand width.

2. **Unused Clock/Reset**: Clock and reset inputs are present for interface consistency but not functionally used since the ALU is combinational.

3. **ISA Compliance**: All operations match the behavior specified in the ISA Reference and verified against the C emulator.

### Related Modules
- `execute_stage.sv`: Uses the ALU for arithmetic/logic instructions
- `neocore_pkg.sv`: Defines the `alu_op_e` enumeration
