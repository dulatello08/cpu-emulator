# Register File Module Reference

## Overview
The Register File provides 16 general-purpose 16-bit registers with multi-port read/write capability for dual-issue execution.

## Module: `register_file`

### Ports

| Port | Direction | Width | Description |
|------|-----------|-------|-------------|
| `clk` | input | 1 | Clock signal |
| `rst` | input | 1 | Reset signal |
| **Read Ports (Slot 0)** | | | |
| `rs1_addr_0` | input | 4 | Source register 1 address |
| `rs2_addr_0` | input | 4 | Source register 2 address |
| `rs1_data_0` | output | 16 | Source register 1 data |
| `rs2_data_0` | output | 16 | Source register 2 data |
| **Read Ports (Slot 1)** | | | |
| `rs1_addr_1` | input | 4 | Source register 1 address |
| `rs2_addr_1` | input | 4 | Source register 2 address |
| `rs1_data_1` | output | 16 | Source register 1 data |
| `rs2_data_1` | output | 16 | Source register 2 data |
| **Write Ports (Slot 0)** | | | |
| `rd_addr_0` | input | 4 | Destination register address |
| `rd_data_0` | input | 16 | Data to write |
| `rd_we_0` | input | 1 | Write enable |
| `rd2_addr_0` | input | 4 | Second destination (32-bit ops) |
| `rd2_data_0` | input | 16 | Second destination data |
| `rd2_we_0` | input | 1 | Second write enable |
| **Write Ports (Slot 1)** | | | |
| `rd_addr_1` | input | 4 | Destination register address |
| `rd_data_1` | input | 16 | Data to write |
| `rd_we_1` | input | 1 | Write enable |
| `rd2_addr_1` | input | 4 | Second destination |
| `rd2_data_1` | input | 16 | Second destination data |
| `rd2_we_1` | input | 1 | Second write enable |

### Register Organization

- **16 registers**: R0 through R15
- **16-bit width**: Each register holds a 16-bit value
- **R0 special**: Hardwired to 0, writes to R0 are ignored

### Multi-Port Configuration

- **4 read ports**: Supports reading 4 registers simultaneously (2 per slot)
- **4 write ports**: Supports writing 4 registers simultaneously (2 per slot for 32-bit ops)

### R0 Hardwiring

```systemverilog
assign rs1_data_0 = (rs1_addr_0 == 4'h0) ? 16'h0000 : registers[rs1_addr_0];
assign rs2_data_0 = (rs2_addr_0 == 4'h0) ? 16'h0000 : registers[rs2_addr_0];
// Similar for slot 1

// Write logic
if (rd_we_0 && rd_addr_0 != 4'h0) begin
  registers[rd_addr_0] <= rd_data_0;
end
```

### 32-bit Operations

For 32-bit multiply operations:
- Result stored in two consecutive registers
- rd_addr holds lower 16 bits
- rd2_addr holds upper 16 bits

### Reset Behavior

All registers initialized to 0x0000 on reset.

### Usage Example

```systemverilog
register_file regfile (
  .clk(clk),
  .rst(rst),
  .rs1_addr_0(decode_rs1_addr_0),
  .rs2_addr_0(decode_rs2_addr_0),
  .rs1_data_0(rf_rs1_data_0),
  .rs2_data_0(rf_rs2_data_0),
  .rs1_addr_1(decode_rs1_addr_1),
  .rs2_data_1(rf_rs2_data_1),
  .rd_addr_0(wb_rd_addr_0),
  .rd_data_0(wb_rd_data_0),
  .rd_we_0(wb_rd_we_0),
  // ... other ports
);
```

### Implementation Notes

1. **Combinational Reads**: Register reads are combinational
2. **Synchronous Writes**: Register writes occur on clock edge
3. **Write Conflicts**: Issue unit prevents dual writes to same register
4. **Bypassing**: R0 reads don't access array, directly return 0

### Related Modules
- `decode_unit.sv`: Generates read addresses
- `writeback_stage.sv`: Generates write addresses and data
- `execute_stage.sv`: Receives read data, detects hazards
