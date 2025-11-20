# Pipeline Registers Module Reference

## Overview
Pipeline registers hold data between pipeline stages and implement stall and flush functionality.

## Modules

### `if_id_reg`
Fetch → Decode pipeline register

### `id_ex_reg`
Decode → Execute pipeline register

### `ex_mem_reg`
Execute → Memory pipeline register

### `mem_wb_reg`
Memory → Writeback pipeline register

## Common Ports

| Port | Direction | Width | Description |
|------|-----------|-------|-------------|
| `clk` | input | 1 | Clock signal |
| `rst` | input | 1 | Reset signal |
| `stall` | input | 1 | Stall this stage (hold current value) |
| `flush` | input | 1 | Flush this stage (insert NOP/bubble) |
| `data_in` | input | struct | Input data from previous stage |
| `data_out` | output | struct | Output data to next stage |

## Behavior

### Normal Operation
```systemverilog
if (!stall) begin
  data_out <= data_in;
end
// else: hold current value
```

### Flush
```systemverilog
if (flush) begin
  data_out.valid <= 1'b0;  // Invalidate instruction
  // Other fields may be cleared or preserved
end
```

### Reset
All pipeline registers clear to invalid state on reset.

## Pipeline Register Types

### `if_id_t`
- Instruction data (up to 13 bytes)
- PC
- Valid flag
- Instruction length

### `id_ex_t`  
- Decoded instruction fields
- Register addresses (rs1, rs2, rd)
- Immediate value
- Control signals (ALU op, branch condition, etc.)
- Flags (is_branch, is_halt, mem_read, mem_write)
- PC
- Valid flag

### `ex_mem_t`
- ALU result
- Memory operation info (address, data, size)
- Branch info (taken, target)
- Write-back info (rd_addr, rd_we)
- Flags (Z, V)
- PC
- Valid flag
- is_halt

### `mem_wb_t`
- Write-back data (wb_data, wb_data2)
- Destination info (rd_addr, rd_we, rd2_addr, rd2_we)
- Flags (Z, V)
- PC
- Valid flag
- is_halt

## Usage Example

```systemverilog
if_id_reg if_id_0 (
  .clk(clk),
  .rst(rst),
  .stall(stall_pipeline),
  .flush(flush_if),
  .data_in(if_id_in_0),
  .data_out(if_id_out_0)
);
```

## Implementation Notes

1. **Stall Priority**: When both stall and flush asserted, stall takes priority
2. **Valid Bit**: Used to track instruction validity through pipeline
3. **Bubble Insertion**: Flush injects pipeline bubble (valid=0)

## Related Modules
- `core_top.sv`: Instantiates all pipeline registers
- `neocore_pkg.sv`: Defines pipeline register structures
