# Unified Memory Module Reference

## Overview
The Unified Memory module implements a Von Neumann architecture memory system with separate instruction fetch and data access ports.

## Module: `unified_memory`

### Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `MEM_SIZE_BYTES` | 65536 | Total memory size in bytes (64 KB default) |
| `ADDR_WIDTH` | 32 | Address bus width |

### Ports

#### Instruction Fetch Port
| Port | Direction | Width | Description |
|------|-----------|-------|-------------|
| `if_addr` | input | 32 | Instruction fetch address |
| `if_req` | input | 1 | Instruction fetch request |
| `if_rdata` | output | 128 | 16 bytes of instruction data |
| `if_ack` | output | 1 | Instruction fetch acknowledge |

#### Data Access Port
| Port | Direction | Width | Description |
|------|-----------|-------|-------------|
| `data_addr` | input | 32 | Data access address |
| `data_wdata` | input | 32 | Data to write (for stores) |
| `data_size` | input | 2 | Access size (0=byte, 1=word, 2=long) |
| `data_we` | input | 1 | Write enable |
| `data_req` | input | 1 | Data access request |
| `data_rdata` | output | 32 | Data read (for loads) |
| `data_ack` | output | 1 | Data access acknowledge |

### Memory Organization

- **Byte-addressable**: Each address refers to one byte
- **Big-endian**: Most significant byte at lowest address
- **Unified**: Instructions and data share same address space

### Big-Endian Layout

```
Address:  0x00  0x01  0x02  0x03
Data:     MSB         ...   LSB
          |----------32-bit---------|
```

### Access Sizes

- **Byte** (size=0): 8-bit access
- **Word** (size=1): 16-bit access (2 bytes)
- **Long** (size=2): 32-bit access (4 bytes)

### Latency

- **Instruction Fetch**: 1 cycle (ack on next clock)
- **Data Access**: 1 cycle (ack on next clock)

### Usage Example

```systemverilog
unified_memory #(
  .MEM_SIZE_BYTES(65536),
  .ADDR_WIDTH(32)
) memory (
  .clk(clk),
  .rst(rst),
  .if_addr(mem_if_addr),
  .if_req(mem_if_req),
  .if_rdata(mem_if_rdata),
  .if_ack(mem_if_ack),
  .data_addr(mem_data_addr),
  .data_wdata(mem_data_wdata),
  .data_size(mem_data_size),
  .data_we(mem_data_we),
  .data_req(mem_data_req),
  .data_rdata(mem_data_rdata),
  .data_ack(mem_data_ack)
);
```

### Memory Initialization

For testbenches, memory can be initialized:

```systemverilog
// Initialize to zero
for (int i = 0; i < 256; i++) begin
  memory.mem[i] = 8'h00;
end

// Load program
memory.mem[32'h00] = 8'h00;  // Byte at address 0
memory.mem[32'h01] = 8'h09;  // Byte at address 1
// ...
```

### Implementation Notes

1. **Dual-Port**: Supports simultaneous instruction fetch and data access
2. **No Conflicts**: Instruction and data ports are independent
3. **Alignment**: Memory handles byte-aligned accesses internally
4. **Big-Endian**: All multi-byte values stored MSB first

### Related Modules
- `core_top.sv`: Connects to both memory ports
- `fetch_unit.sv`: Uses instruction fetch port
- `memory_stage.sv`: Uses data access port
