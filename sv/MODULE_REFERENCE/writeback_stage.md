# Writeback Stage Module Reference

## Overview
The Writeback Stage commits instruction results to the register file and generates the halt signal when HLT instruction completes.

## Module: `writeback_stage`

### Ports

Inputs for both instruction slots (0 and 1):
- Pipeline register inputs (`mem_wb_t` struct)

Outputs:
- Register write signals (address, data, enable)
- Flag update signals (Z, V flags)
- **Halt signal**

### Writeback Operations

1. **Register Updates**: Write ALU/memory results to destination registers
2. **Flag Updates**: Update Z and V flags from ALU operations
3. **Halt Detection**: Set `halted` when HLT instruction reaches WB

### Halt Behavior

**Critical**: When HLT instruction reaches writeback:

```systemverilog
assign halted = (mem_wb_0.valid && mem_wb_0.is_halt) ||
                (mem_wb_1.valid && mem_wb_1.is_halt);
```

This triggers:
- `stall_pipeline = 1` in core_top
- PC freezes at HLT instruction address
- Pipeline stops advancing

### Register Write Priority

When both slots write to same register (shouldn't happen with proper issue logic):
- Slot 0 has priority
- Slot 1 write is blocked

### Forwarding Support

Writeback data is forwarded to execute stage for hazard resolution.

### Usage Example

```systemverilog
writeback_stage writeback (
  .clk(clk),
  .rst(rst),
  .mem_wb_0(mem_wb_out_0),
  .mem_wb_1(mem_wb_out_1),
  .rd_addr_0(wb_rd_addr_0),
  .rd_data_0(wb_rd_data_0),
  .rd_we_0(wb_rd_we_0),
  .rd2_addr_0(wb_rd2_addr_0),
  .rd2_data_0(wb_rd2_data_0),
  .rd2_we_0(wb_rd2_we_0),
  .rd_addr_1(wb_rd_addr_1),
  .rd_data_1(wb_rd_data_1),
  .rd_we_1(wb_rd_we_1),
  .z_flag_update(wb_z_flag_update),
  .z_flag_value(wb_z_flag_value),
  .v_flag_update(wb_v_flag_update),
  .v_flag_value(wb_v_flag_value),
  .halted(halted)
);
```

### Implementation Notes

1. **Halt is Permanent**: Once `halted` goes high, it stays high until reset
2. **No Register Write on Halt**: HLT instruction doesn't write any registers
3. **Dual Writeback**: Both slots can write simultaneously (different registers)

### Related Modules
- `register_file.sv`: Receives writeback data
- `core_top.sv`: Uses halted signal for stall logic
- `execute_stage.sv`: Receives forwarding data
