# Decode Unit Module Reference

## Overview
The Decode Unit decodes variable-length instructions and extracts operands, immediate values, and control signals. Supports decoding two instructions simultaneously for dual-issue capability.

## Module: `decode_unit`

### Ports

| Port | Direction | Width | Description |
|------|-----------|-------|-------------|
| `clk` | input | 1 | Clock signal |
| `rst` | input | 1 | Reset signal |
| `inst_data` | input | 104 | Raw instruction bytes (up to 13 bytes) |
| `inst_len` | input | 4 | Instruction length in bytes |
| `pc` | input | 32 | Program counter for this instruction |
| `valid_in` | input | 1 | Instruction valid signal |
| `opcode` | output | `opcode_e` | Decoded opcode |
| `specifier` | output | 8 | Instruction specifier byte |
| `itype` | output | `itype_e` | Instruction type (ALU, MOV, MEM, etc.) |
| `rd_addr` | output | 4 | Destination register address |
| `rs1_addr` | output | 4 | Source register 1 address |
| `rs2_addr` | output | 4 | Source register 2 address |
| `rd_we` | output | 1 | Destination register write enable |
| `rd2_addr` | output | 4 | Second destination register (for 32-bit ops) |
| `rd2_we` | output | 1 | Second destination write enable |
| `immediate` | output | 32 | Immediate value (sign/zero-extended) |
| `mem_read` | output | 1 | Memory read operation |
| `mem_write` | output | 1 | Memory write operation |
| `mem_size` | output | `mem_size_e` | Memory access size |
| `is_branch` | output | 1 | Branch instruction |
| `is_jsr` | output | 1 | Jump to subroutine |
| `is_rts` | output | 1 | Return from subroutine |
| `is_halt` | output | 1 | Halt instruction |
| `branch_cond` | output | `branch_cond_e` | Branch condition type |
| `alu_op` | output | `alu_op_e` | ALU operation |
| `valid_out` | output | 1 | Decoded instruction valid |

### Parameters
None.

### Instruction Format

Per Instructions.md (big-endian):
- **Byte 0**: Specifier (addressing mode / format)
- **Byte 1**: Opcode
- **Bytes 2+**: Operands (register addresses, immediates, offsets)

### Decoding Process

1. **Extract Fields**: Parse specifier, opcode, and operands from `inst_data`
2. **Determine Type**: Map opcode to instruction type (ALU, MOV, BRANCH, etc.)
3. **Extract Operands**: Based on specifier, extract register addresses and immediates
4. **Generate Control Signals**: Set ALU op, memory controls, branch conditions

### Specifier Encoding

The specifier byte determines operand format:
- `0x00`: Immediate operand
- `0x01`: Register indirect / indexed
- `0x02`: Register-register
- `0x03`: Absolute address
- ...and more per Instructions.md

### Supported Instructions

All instructions defined in ISA_REFERENCE.md:
- Arithmetic: ADD, SUB, MUL
- Logic: AND, OR, XOR
- Shift: LSH, RSH
- Data Movement: MOV
- Memory: LD, ST (various sizes)
- Branch: B, BEQ, BNE, BLT, etc.
- Control: JSR, RTS, HLT

### Usage Example

```systemverilog
decode_unit decode (
  .clk(clk),
  .rst(rst),
  .inst_data(fetch_inst_data_0),
  .inst_len(fetch_inst_len_0),
  .pc(fetch_pc_0),
  .valid_in(fetch_valid_0),
  .opcode(decode_opcode_0),
  .specifier(decode_specifier_0),
  .itype(decode_itype_0),
  .rd_addr(decode_rd_addr_0),
  .rs1_addr(decode_rs1_addr_0),
  .rs2_addr(decode_rs2_addr_0),
  .rd_we(decode_rd_we_0),
  .immediate(decode_immediate_0),
  .mem_read(decode_mem_read_0),
  .mem_write(decode_mem_write_0),
  // ... other outputs
  .valid_out(decode_valid_0)
);
```

### Implementation Notes

1. **Combinational Logic**: Decoding is purely combinational for low latency
2. **Big-Endian Extraction**: Operand bytes extracted accounting for big-endian layout
3. **Sign Extension**: Immediates sign-extended to 32 bits where appropriate
4. **Default R0**: Register R0 hardwired to 0 in register file

### Related Modules
- `fetch_unit.sv`: Provides instruction bytes
- `issue_unit.sv`: Receives decoded control signals
- `neocore_pkg.sv`: Defines opcode and type enumerations
- `execute_stage.sv`: Receives decoded instruction for execution
