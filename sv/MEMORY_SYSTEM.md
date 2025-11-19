# NeoCore16x32 Memory System

## Overview

The NeoCore16x32 uses a **Von Neumann architecture** with a single unified memory for both instructions and data. All memory accesses use **big-endian byte ordering**, and the memory is implemented using FPGA block RAM (BRAM) for synthesis efficiency.

## Unified Memory Architecture

### Von Neumann vs. Harvard

Unlike Harvard architectures with separate instruction and data memories, the NeoCore16x32 uses a **single unified address space**:

**Advantages:**
- Simpler memory management
- Code can be treated as data (self-modifying code possible, though not recommended)
- No artificial division of memory resources
- Single memory simplifies debugging

**Trade-offs:**
- Single memory bandwidth shared between instruction fetch and data access
- Potential structural hazard if both need memory simultaneously

### Dual-Port Access

The unified_memory module provides **two independent ports**:

1. **Instruction Fetch Port (IF)**
   - **Width**: 128 bits (16 bytes)
   - **Address**: 32-bit byte address
   - **Latency**: 1 cycle
   - **Purpose**: Fetch variable-length instructions
   - **Access**: Read-only

2. **Data Access Port (DATA)**
   - **Width**: 32 bits (4 bytes max)
   - **Address**: 32-bit byte address
   - **Latency**: 1 cycle
   - **Purpose**: Load/store operations
   - **Access**: Read/write
   - **Granularity**: Byte, halfword (16-bit), or word (32-bit)

Both ports can access memory **simultaneously** in the same cycle (true dual-port).

---

## Memory Organization

### Address Space

- **Logical Address Space**: 32-bit (4 GB addressable)
- **Physical Implementation**: Typically 64 KB (configurable via parameter)
- **Byte-Addressable**: Every byte has a unique address
- **Alignment**: No enforced alignment (software responsibility)

### Memory Array

The memory is implemented as a **byte array** for FPGA BRAM inference:

```verilog
logic [7:0] mem [0:MEM_SIZE_BYTES-1];
```

**Default Size**: 65536 bytes (64 KB) = `MEM_SIZE_BYTES` parameter

**Address Mapping**:
```
Logical Address:  Physical Address:
0x00000000    ->  mem[0]
0x00000001    ->  mem[1]
...
0x0000FFFF    ->  mem[65535]
0x00010000+   ->  Out of bounds (returns 0 on read, ignores write)
```

---

## Big-Endian Byte Ordering

**All memory accesses use big-endian byte ordering** where the **most significant byte (MSB)** is stored at the **lowest address**.

### Byte Access

Single byte - no endianness issue:
```
Address 0x1000: 0x AB
```

### Halfword (16-bit) Access

Big-endian layout:
```
Address:  0x1000   0x1001
Content:  [MSB ]   [LSB ]
Value:    0x12     0x34
Halfword = 0x1234
```

**RTL Implementation** (read):
```verilog
data_rdata <= {
  16'h0,
  mem[data_addr + 0],  // MSB
  mem[data_addr + 1]   // LSB
};
```

**RTL Implementation** (write):
```verilog
mem[data_addr + 0] <= data_wdata[15:8];  // MSB first
mem[data_addr + 1] <= data_wdata[7:0];   // LSB second
```

### Word (32-bit) Access

Big-endian layout:
```
Address:  0x2000   0x2001   0x2002   0x2003
Content:  [MSB ]   [ . . ]  [ . . ]  [LSB ]
Value:    0xDE     0xAD     0xBE     0xEF
Word = 0xDEADBEEF
```

**RTL Implementation** (read):
```verilog
data_rdata <= {
  mem[data_addr + 0],  // Bits 31-24 (MSB)
  mem[data_addr + 1],  // Bits 23-16
  mem[data_addr + 2],  // Bits 15-8
  mem[data_addr + 3]   // Bits 7-0 (LSB)
};
```

---

## Instruction Fetch Port

### Wide Instruction Fetch

To support **variable-length instructions** (2-13 bytes), the instruction fetch port fetches **16 bytes (128 bits)** per cycle:

**Fetch Operation**:
```verilog
if (if_req && if_addr < (MEM_SIZE_BYTES - 16)) begin
  if_rdata <= {
    mem[if_addr + 0],   // Byte 0 -> bits [127:120] (MSB)
    mem[if_addr + 1],   // Byte 1 -> bits [119:112]
    mem[if_addr + 2],   // Byte 2 -> bits [111:104]
    ...
    mem[if_addr + 15]   // Byte 15 -> bits [7:0] (LSB)
  };
  if_ack <= 1'b1;
end
```

**Big-Endian Note**: The first byte fetched (at if_addr) appears in the **most significant bits** of if_rdata.

### Instruction Fetch Timing

```
Cycle 1: if_req = 1, if_addr = 0x1000
Cycle 2: if_ack = 1, if_rdata = {mem[0x1000], mem[0x1001], ..., mem[0x100F]}
```

**Latency**: 1 cycle (registered output)  
**Throughput**: Up to 16 bytes per cycle

### Bandwidth

- **Theoretical max**: 16 bytes/cycle
- **Practical**: Varies based on instruction lengths and dual-issue success
- **Average**: ~6-10 bytes/cycle for typical code

---

## Data Access Port

### Access Granularity

The data port supports **three access sizes**:

| Size | Encoding | Bytes | Alignment | Use Case |
|------|----------|-------|-----------|----------|
| Byte | 2'b00 | 1 | None | Character/flag access |
| Halfword | 2'b01 | 2 | Even address recommended | Register load/store |
| Word | 2'b10 | 4 | 4-byte recommended | 32-bit values |

### Data Read Operation

**Byte Read**:
```verilog
if (data_size == 2'b00) begin
  data_rdata <= {24'h0, mem[data_addr]};
end
```

**Halfword Read** (big-endian):
```verilog
if (data_size == 2'b01) begin
  data_rdata <= {
    16'h0,
    mem[data_addr + 0],  // MSB
    mem[data_addr + 1]   // LSB
  };
end
```

**Word Read** (big-endian):
```verilog
if (data_size == 2'b10) begin
  data_rdata <= {
    mem[data_addr + 0],  // MSB (bits 31-24)
    mem[data_addr + 1],  // bits 23-16
    mem[data_addr + 2],  // bits 15-8
    mem[data_addr + 3]   // LSB (bits 7-0)
  };
end
```

### Data Write Operation

**Byte Write**:
```verilog
if (data_we && data_size == 2'b00) begin
  mem[data_addr] <= data_wdata[7:0];
end
```

**Halfword Write** (big-endian):
```verilog
if (data_we && data_size == 2'b01) begin
  mem[data_addr + 0] <= data_wdata[15:8];  // MSB
  mem[data_addr + 1] <= data_wdata[7:0];   // LSB
end
```

**Word Write** (big-endian):
```verilog
if (data_we && data_size == 2'b10) begin
  mem[data_addr + 0] <= data_wdata[31:24];  // MSB
  mem[data_addr + 1] <= data_wdata[23:16];
  mem[data_addr + 2] <= data_wdata[15:8];
  mem[data_addr + 3] <= data_wdata[7:0];    // LSB
end
```

### Data Access Timing

```
Cycle 1: data_req = 1, data_addr = 0x2000, data_we = 1 (write)
Cycle 2: data_ack = 1 (write complete)

Cycle 3: data_req = 1, data_addr = 0x2000, data_we = 0 (read)
Cycle 4: data_ack = 1, data_rdata = mem contents
```

**Latency**: 1 cycle for both reads and writes  
**Throughput**: 1 access per cycle

---

## Memory Initialization

### Hex File Format

Memory can be initialized from hex files for simulation and synthesis:

**Example**: test_simple.hex
```
@0000
00 00   // NOP at address 0x0000
00 01 05 12 34   // ADD R5, #0x1234 at 0x0002
...
```

**Format**:
- `@XXXX` sets address
- Following bytes are data (space-separated hex)
- Big-endian: first byte goes to lowest address

### Loading Memory

**SystemVerilog `$readmemh`**:
```verilog
initial begin
  $readmemh("test_simple.hex", mem);
end
```

---

## Address Space Usage

While the architecture doesn't enforce a memory map, typical usage follows:

### Suggested Memory Map

```
0x00000000 - 0x00007FFF   Boot Sector (32 KB)
                          - Entry point at 0x00000000
                          - Main program code

0x00008000 - 0x00008FFF   Reserved for MMIO (UART)
                          - Memory-mapped I/O devices
                          - Implementation-dependent

0x00010000 - 0x00010FFF   Reserved for MMIO (PIC)
                          - Interrupt controller
                          - Implementation-dependent

0x00020000 - 0x00027FFF   Stack Memory (32 KB)
                          - Stack grows downward from top
                          - R14 used as stack pointer

0x00030000 - 0x0004FFFF   Flash/Data Memory (128 KB)
                          - Non-volatile storage simulation
                          - Static data

0x00050000 - 0xFFFFFFFF   Unused / Future Expansion
```

**Note**: This is a **suggested** map from config.ini. The CPU RTL is agnostic to memory mapping.

---

## Memory Access Conflicts

### Simultaneous Access

Both instruction fetch and data access ports can operate **simultaneously**:

**No Conflict** (both ports access different addresses):
```
Cycle 1:
  IF port:   Fetch 16 bytes from 0x1000
  DATA port: Read halfword from 0x3000
  Result: Both succeed in same cycle
```

**Note**: The FPGA BRAM infers **true dual-port** memory, allowing parallel access.

### Memory Arbitration (None Required)

Because the memory is true dual-port, **no arbitration** is needed between instruction fetch and data access. They proceed independently.

**Implication for Pipeline**: The memory stage doesn't need to stall for instruction fetches.

---

## Memory Latency and Throughput

### Instruction Fetch

- **Request**: Cycle N (if_req = 1)
- **Acknowledge**: Cycle N+1 (if_ack = 1, if_rdata valid)
- **Latency**: 1 cycle
- **Bandwidth**: 16 bytes/cycle

### Data Access

- **Request**: Cycle N (data_req = 1)
- **Acknowledge**: Cycle N+1 (data_ack = 1, data_rdata valid for reads)
- **Latency**: 1 cycle
- **Bandwidth**: 1-4 bytes/cycle (depending on access size)

### Pipeline Impact

**Instruction Fetch**:
- Fetch unit maintains a buffer to hide latency
- Typically no stalls unless buffer underruns

**Data Access**:
- Load instructions complete in MEM stage (1 cycle)
- Store instructions complete in MEM stage (1 cycle)
- Load-use hazards cause 1-cycle stall (detected in hazard unit)

---

## Memory Alignment

### Alignment Recommendations

While the RTL **does not enforce** alignment, software should follow these guidelines:

**Halfword (16-bit) Access**:
- **Recommended**: Even address (bit 0 = 0)
- **Example**: 0x1000, 0x1002, 0x1004 (good)
- **Example**: 0x1001, 0x1003 (unaligned, avoid)

**Word (32-bit) Access**:
- **Recommended**: 4-byte aligned (bits 1-0 = 00)
- **Example**: 0x1000, 0x1004, 0x1008 (good)
- **Example**: 0x1001, 0x1002, 0x1003 (unaligned, avoid)

### Unaligned Access Behavior

The current RTL **allows** unaligned accesses:

**Unaligned Halfword Read** (address 0x1001):
```verilog
data_rdata = {16'h0, mem[0x1001], mem[0x1002]};
```
This works but may not match software expectations if porting code from architectures with enforced alignment.

**Best Practice**: Software should ensure proper alignment to avoid unexpected behavior in future hardware revisions or when porting to stricter architectures.

---

## Memory Synthesis Considerations

### FPGA Block RAM Inference

The memory array is designed to infer FPGA block RAM:

```verilog
logic [7:0] mem [0:MEM_SIZE_BYTES-1];
```

**Synthesis Guidelines**:
- Array declaration infers BRAM
- Sequential read (registered output) required for BRAM
- Dual-port access infers dual-port BRAM
- Size must match FPGA BRAM configuration

### Resource Usage

**Lattice ECP5-85F** (target FPGA):
- **BRAM blocks**: 208 KB total
- **This design**: 64 KB (30% utilization)
- **BRAM organization**: 18 Kbit blocks

**Scaling**:
- 16 KB memory: ~8% BRAM
- 64 KB memory: ~31% BRAM (current)
- 128 KB memory: ~62% BRAM
- 256 KB memory: Exceeds single FPGA capacity (need external SRAM)

---

## Memory Testing

### Testbench Memory Initialization

The core_unified_tb testbench initializes memory with test programs:

```verilog
initial begin
  // Load test program
  unified_memory.mem[0] = 8'h00;  // NOP
  unified_memory.mem[1] = 8'h00;
  unified_memory.mem[2] = 8'h00;  // ADD R1, R2
  unified_memory.mem[3] = 8'h01;
  unified_memory.mem[4] = 8'h01;
  unified_memory.mem[5] = 8'h02;
  ...
end
```

### Memory Debugging

**Waveform Analysis**:
- Monitor `mem_if_addr`, `mem_if_rdata` for instruction fetches
- Monitor `mem_data_addr`, `mem_data_wdata`, `mem_data_rdata` for data accesses
- Check `mem_if_ack`, `mem_data_ack` for timing

**Memory Dump**:
```verilog
initial begin
  #10000;
  $writememh("memory_dump.hex", mem);
end
```

---

## Summary

The NeoCore16x32 memory system provides:

1. **Unified Von Neumann architecture** for simplicity
2. **Big-endian byte ordering** consistently throughout
3. **Dual-port access** for simultaneous instruction fetch and data access
4. **Flexible access granularity** (byte, halfword, word)
5. **1-cycle latency** for all memory operations
6. **FPGA-optimized** for block RAM synthesis

This design balances performance (dual-port, wide fetch) with simplicity (single address space, straightforward byte ordering) while remaining FPGA-friendly for synthesis.

