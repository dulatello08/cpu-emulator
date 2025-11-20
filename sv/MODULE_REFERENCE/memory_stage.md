# Memory Stage Module Reference

## Overview
The Memory Stage handles load and store operations, interfacing with the unified memory system for data accesses.

## Module: `memory_stage`

### Ports

Inputs for both instruction slots (0 and 1):
- Pipeline register inputs (`ex_mem_t` struct)
- Memory interface (unified memory data port)

Outputs for both slots:
- Pipeline register outputs (`mem_wb_t` struct)
- Memory request signals (address, data, control)

### Memory Operations

**Load**: Read data from memory into register
- Sizes: 8-bit (byte), 16-bit (word), 32-bit (long)
- Zero-extension for byte/word loads

**Store**: Write data from register to memory
- Sizes: 8-bit (byte), 16-bit (word), 32-bit (long)
- Byte alignment handled by memory interface

### Memory Interface

```systemverilog
output logic [31:0] mem_data_addr;   // Address
output logic [31:0] mem_data_wdata;  // Write data
output logic [1:0]  mem_data_size;   // Size (0=byte, 1=word, 2=long)
output logic        mem_data_we;     // Write enable
output logic        mem_data_req;    // Request
input  logic [31:0] mem_data_rdata;  // Read data
input  logic        mem_data_ack;    // Acknowledge
```

### Stall Generation

Generates `mem_stall` signal when:
- Memory request pending and not yet acknowledged
- Prevents pipeline advancement until memory operation completes

### Dual-Issue Constraints

Only **one** memory operation allowed per cycle (enforced by issue_unit).

### Usage Example

```systemverilog
memory_stage memory (
  .clk(clk),
  .rst(rst),
  .ex_mem_0(ex_mem_out_0),
  .ex_mem_1(ex_mem_out_1),
  .mem_data_addr(mem_data_addr),
  .mem_data_wdata(mem_data_wdata),
  .mem_data_size(mem_data_size),
  .mem_data_we(mem_data_we),
  .mem_data_req(mem_data_req),
  .mem_data_rdata(mem_data_rdata),
  .mem_data_ack(mem_data_ack),
  .mem_wb_0(mem_wb_in_0),
  .mem_wb_1(mem_wb_in_1),
  .mem_stall(mem_stall)
);
```

### Implementation Notes

1. **Single Memory Port**: Only slot 0 or slot 1 can access memory, not both
2. **Latency**: Memory operations may take multiple cycles
3. **Alignment**: Memory system handles byte alignment internally

### Related Modules
- `unified_memory.sv`: Provides data memory interface
- `execute_stage.sv`: Computes memory addresses
- `writeback_stage.sv`: Receives loaded data
