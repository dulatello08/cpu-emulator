# Multiply Unit Module Reference

## Overview
The Multiply Unit performs 16-bit × 16-bit multiplication, producing a 32-bit result.

## Module: `multiply_unit`

### Ports

| Port | Direction | Width | Description |
|------|-----------|-------|-------------|
| `clk` | input | 1 | Clock signal (unused, for consistency) |
| `rst` | input | 1 | Reset signal (unused, for consistency) |
| `operand_a` | input | 16 | First operand |
| `operand_b` | input | 16 | Second operand |
| `mul_op` | input | `mul_op_e` | Multiply operation type |
| `result` | output | 32 | 32-bit product |

### Supported Operations

| Operation | Type | Description |
|-----------|------|-------------|
| `MUL_UMULL` | Unsigned | Unsigned 16×16 = 32-bit result |
| `MUL_SMULL` | Signed | Signed 16×16 = 32-bit result |

### Behavior

#### Unsigned Multiply
```systemverilog
result = operand_a * operand_b;  // Zero-extended
```

#### Signed Multiply
```systemverilog
result = $signed(operand_a) * $signed(operand_b);
```

### Result Storage

The 32-bit result is stored in two registers:
- Lower 16 bits → rd_addr (destination register)
- Upper 16 bits → rd2_addr (second destination)

### Latency

The multiply operation is combinational in the current implementation (1 cycle).

### Usage Example

```systemverilog
multiply_unit mul (
  .clk(clk),
  .rst(rst),
  .operand_a(rs1_data),
  .operand_b(rs2_data),
  .mul_op(MUL_UMULL),
  .result(mul_result)  // 32-bit result
);

// In writeback:
// registers[rd_addr] <= mul_result[15:0];   // Lower 16 bits
// registers[rd2_addr] <= mul_result[31:16]; // Upper 16 bits
```

### Implementation Notes

1. **Combinational**: Uses `*` operator, synthesizes to multiplier
2. **No Pipeline**: Single-cycle operation (may be multi-cycle in FPGA)
3. **Sign Extension**: Uses `$signed()` for signed multiply

### Related Modules
- `execute_stage.sv`: Instantiates multiply_unit
- `writeback_stage.sv`: Writes 32-bit result to two registers
- `neocore_pkg.sv`: Defines `mul_op_e` enum
