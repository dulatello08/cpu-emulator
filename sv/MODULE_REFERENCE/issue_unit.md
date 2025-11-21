# Issue Unit Module Reference

## Overview
The Issue Unit determines whether one or two instructions can be issued simultaneously based on resource hazards, data dependencies, and instruction types. It implements the dual-issue decision logic for the NeoCore16x32 pipeline.

## Module: `issue_unit`

### Ports

| Port | Direction | Width | Description |
|------|-----------|-------|-------------|
| `clk` | input | 1 | Clock signal |
| `rst` | input | 1 | Reset signal |
| **Instruction 0 Inputs** | | | |
| `inst0_valid` | input | 1 | First instruction valid |
| `inst0_type` | input | `itype_e` | Instruction type |
| `inst0_mem_read` | input | 1 | Memory read flag |
| `inst0_mem_write` | input | 1 | Memory write flag |
| `inst0_is_branch` | input | 1 | Branch instruction flag |
| `inst0_rd_addr` | input | 4 | Destination register |
| `inst0_rd_we` | input | 1 | Destination write enable |
| `inst0_rd2_addr` | input | 4 | Second destination (32-bit ops) |
| `inst0_rd2_we` | input | 1 | Second destination write enable |
| **Instruction 1 Inputs** | | | |
| `inst1_valid` | input | 1 | Second instruction valid |
| `inst1_type` | input | `itype_e` | Instruction type |
| `inst1_mem_read` | input | 1 | Memory read flag |
| `inst1_mem_write` | input | 1 | Memory write flag |
| `inst1_is_branch` | input | 1 | Branch instruction flag |
| `inst1_rs1_addr` | input | 4 | Source register 1 |
| `inst1_rs2_addr` | input | 4 | Source register 2 |
| `inst1_rd_addr` | input | 4 | Destination register |
| `inst1_rd_we` | input | 1 | Destination write enable |
| `inst1_rd2_addr` | input | 4 | Second destination |
| `inst1_rd2_we` | input | 1 | Second destination write enable |
| **Outputs** | | | |
| `issue_inst0` | output | 1 | Issue instruction 0 |
| `issue_inst1` | output | 1 | Issue instruction 1 |
| `dual_issue` | output | 1 | **Both instructions issued (sent to fetch_unit)** |

### Parameters
None.

### Dual-Issue Rules

Instructions can be dual-issued if **ALL** of these conditions are met:

1. **Both Valid**: `inst0_valid && inst1_valid`

2. **No Resource Hazards**:
   - At most one memory operation (read or write)
   - At most one branch/control instruction

3. **No Write-After-Write (WAW) Hazards**:
   - Inst0 and Inst1 must not write to same register
   - Check both primary and secondary destinations (for 32-bit ops)

4. **No Read-After-Write (RAW) Hazards**:
   - Inst1 sources must not depend on Inst0 destinations
   - If Inst0 writes Rd, Inst1 cannot read Rd as Rs1 or Rs2

### Hazard Detection Logic

```systemverilog
// WAW hazard
waw_hazard = (inst0_rd_we && inst1_rd_we && inst0_rd_addr == inst1_rd_addr) ||
             (inst0_rd2_we && inst1_rd2_we && inst0_rd2_addr == inst1_rd2_addr) ||
             (inst0_rd_we && inst1_rd2_we && inst0_rd_addr == inst1_rd2_addr) ||
             (inst0_rd2_we && inst1_rd_we && inst0_rd2_addr == inst1_rd_addr);

// RAW hazard
raw_hazard = (inst0_rd_we && inst0_rd_addr != 0 && 
              ((inst1_rs1_addr == inst0_rd_addr) || (inst1_rs2_addr == inst0_rd_addr))) ||
             (inst0_rd2_we && inst0_rd2_addr != 0 &&
              ((inst1_rs1_addr == inst0_rd2_addr) || (inst1_rs2_addr == inst0_rd2_addr)));

// Resource hazards
mem_conflict = (inst0_mem_read || inst0_mem_write) && 
               (inst1_mem_read || inst1_mem_write);
               
branch_conflict = inst0_is_branch && inst1_is_branch;
```

### Issue Decision

```systemverilog
assign dual_issue = inst0_valid && inst1_valid &&
                    !raw_hazard && !waw_hazard &&
                    !mem_conflict && !branch_conflict;

assign issue_inst0 = inst0_valid;
assign issue_inst1 = dual_issue;  // Only issue inst1 if dual-issuing
```

### Critical Integration

**The `dual_issue` output MUST be connected to `fetch_unit`** so fetch knows how many instruction bytes to consume from the buffer.

### Usage Example

```systemverilog
issue_unit issue (
  .clk(clk),
  .rst(rst),
  .inst0_valid(decode_valid_0),
  .inst0_type(decode_itype_0),
  .inst0_mem_read(decode_mem_read_0),
  .inst0_mem_write(decode_mem_write_0),
  .inst0_is_branch(decode_is_branch_0),
  .inst0_rd_addr(decode_rd_addr_0),
  .inst0_rd_we(decode_rd_we_0),
  // ... inst0 inputs
  .inst1_valid(decode_valid_1),
  // ... inst1 inputs
  .issue_inst0(issue_inst0),
  .issue_inst1(issue_inst1),
  .dual_issue(dual_issue)  // CONNECT TO FETCH_UNIT!
);
```

### Performance Impact

Dual-issue capability can achieve up to **2 IPC (instructions per cycle)** for independent instruction pairs. Actual performance depends on:
- Instruction mix (memory ops, branches limit dual-issue)
- Data dependencies (RAW hazards force single-issue)
- Code scheduling (compiler/programmer optimization)

### Implementation Notes

1. **Conservative Approach**: Issue unit prevents hazards pessimistically
2. **No Forwarding**: RAW hazards always prevent dual-issue (no bypass paths)
3. **R0 Exception**: Register R0 reads don't cause RAW hazards (hardwired to 0)

### Related Modules
- `decode_unit.sv`: Provides instruction type and operand information
- `fetch_unit.sv`: **Receives dual_issue to determine byte consumption**
- `hazard_unit.sv`: Detects pipeline hazards for single-issue stalls
- `core_top.sv`: Integrates issue_unit and connects dual_issue signal
