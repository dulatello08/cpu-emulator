# Fetch Unit Module Reference

## Overview
The Fetch Unit retrieves variable-length instructions from unified memory and maintains an instruction buffer for dual-issue capability. It handles PC updates for sequential execution, branches, and pipeline stalls.

## Module: `fetch_unit`

### Ports

| Port | Direction | Width | Description |
|------|-----------|-------|-------------|
| `clk` | input | 1 | Clock signal |
| `rst` | input | 1 | Reset signal |
| `branch_taken` | input | 1 | Branch taken signal from execute stage |
| `branch_target` | input | 32 | Branch target address |
| `stall` | input | 1 | Stall signal from hazard/memory/halt logic |
| `dual_issue` | input | 1 | **Dual-issue decision from issue_unit** |
| `mem_addr` | output | 32 | Memory address for instruction fetch |
| `mem_req` | output | 1 | Memory request signal |
| `mem_rdata` | input | 128 | 16 bytes of instruction data (big-endian) |
| `mem_ack` | input | 1 | Memory acknowledge signal |
| `inst_data_0` | output | 104 | First instruction bytes (up to 13 bytes) |
| `inst_len_0` | output | 4 | First instruction length in bytes |
| `pc_0` | output | 32 | PC of first instruction |
| `valid_0` | output | 1 | First instruction valid |
| `inst_data_1` | output | 104 | Second instruction (for dual-issue) |
| `inst_len_1` | output | 4 | Second instruction length in bytes |
| `pc_1` | output | 32 | PC of second instruction |
| `valid_1` | output | 1 | Second instruction valid |

### Parameters
None.

### Big-Endian Memory Model

Instructions are stored in **big-endian format**:
- Byte at address N is **more significant** than byte at address N+1
- Buffer layout: bits[255:248] = byte 0, bits[247:240] = byte 1, etc.

### Instruction Format

Per the ISA specification (Instructions.md):
- **Byte 0**: Specifier
- **Byte 1**: Opcode
- **Bytes 2+**: Operands (varying length based on specifier)

Instruction lengths range from 2 to 9 bytes.

### Buffer Management

The fetch unit maintains a **256-bit (32-byte) instruction buffer**:

1. **Refill**: When buffer has < 16 valid bytes, request 16-byte memory fetch
2. **Extraction**: Extract up to 2 instructions from buffer top (MSB)
3. **Consumption**: After issue_unit confirms dual-issue decision, shift consumed bytes out using **LEFT shift** (keeps remaining bytes at MSB)
4. **Alignment**: Buffer PC (`buffer_pc`) tracks address of byte 0 in buffer

### Critical Fix: Dual-Issue Awareness

**FIXED BUG**: The fetch unit now receives the `dual_issue` signal from `issue_unit` to determine how many instruction bytes to consume.

**Previous behavior** (WRONG):
- Consumed both instruction lengths even when hazards prevented dual-issue
- PC advanced incorrectly, skipping instructions

**Current behavior** (CORRECT):
```systemverilog
consumed_bytes = inst_len_0;
if (can_consume_1 && dual_issue) begin  // Check actual dual-issue decision
  consumed_bytes = consumed_bytes + inst_len_1;
end
```

### PC Update Logic

```systemverilog
if (branch_taken) begin
  pc_next = branch_target;  // Branch redirect
end else if (!stall) begin
  pc_next = pc + consumed_bytes;  // Advance by exact instruction lengths
end else begin
  pc_next = pc;  // Stalled
end
```

### Buffer Shift Direction

**CRITICAL**: Uses **LEFT shift** (`<<`) to consume bytes from big-endian buffer:
- LEFT shift removes consumed bytes from MSB
- Remaining bytes stay at MSB where extraction happens
- RIGHT shift would move remaining bytes to LSB (WRONG!)

Example:
```
Before: buffer[255:248] = 0x00 (byte 0), buffer[247:240] = 0x09 (byte 1)
After consuming 5 bytes with LEFT shift:
  buffer[255:248] = 0x02 (byte 5, now at top)
```

### Behavior

1. **Reset**: PC = 0x00000000, buffer empty
2. **Normal Operation**:
   - Fetch 16 bytes when buffer < 16 bytes valid
   - Extract up to 2 instructions from buffer
   - Compute instruction lengths from specifier
   - Output valid instructions to decode stage
3. **Branch**: Flush buffer, redirect PC
4. **Stall**: Hold PC, don't consume buffer

### Usage Example

```systemverilog
fetch_unit fetch (
  .clk(clk),
  .rst(rst),
  .branch_taken(branch_taken),
  .branch_target(branch_target),
  .stall(stall_pipeline),
  .dual_issue(dual_issue),  // FROM issue_unit
  .mem_addr(mem_if_addr),
  .mem_req(mem_if_req),
  .mem_rdata(mem_if_rdata),
  .mem_ack(mem_if_ack),
  .inst_data_0(fetch_inst_data_0),
  .inst_len_0(fetch_inst_len_0),
  .pc_0(fetch_pc_0),
  .valid_0(fetch_valid_0),
  .inst_data_1(fetch_inst_data_1),
  .inst_len_1(fetch_inst_len_1),
  .pc_1(fetch_pc_1),
  .valid_1(fetch_valid_1)
);
```

### Implementation Notes

1. **Buffer Overflow Protection**: Refill clamped to max 32 bytes total
2. **Variable Shift**: Uses `consumed_bytes * 8` bit shift (SystemVerilog supports this)
3. **Instruction Length Decoding**: Computed from specifier byte per ISA spec

### Known Limitations

None. All bugs related to byte consumption and PC advancement have been fixed.

### Related Modules
- `core_top.sv`: Instantiates fetch_unit and connects dual_issue signal
- `issue_unit.sv`: Generates dual_issue decision signal
- `unified_memory.sv`: Provides instruction data
- `decode_unit.sv`: Receives fetched instructions
