# core_top.sv - CPU Top-Level Integration

## Purpose

Integrates all pipeline stages, hazard detection, forwarding logic, and control for the complete NeoCore16x32 dual-issue CPU. Connects to external unified memory via separate instruction fetch and data access ports.

## Module Hierarchy

```
core_top
├── fetch_unit (instruction fetch with buffer)
├── decode_unit x2 (dual decoders)
├── issue_unit (dual-issue controller)
├── register_file (16x16-bit with 4R/2W ports)
├── hazard_unit (forwarding and stall detection)
├── execute_stage (ALU, multiply, branch units x2)
├── memory_stage (load/store with arbitration)
├── writeback_stage (register write and flag update)
└── pipeline_regs x8 (IF/ID, ID/EX, EX/MEM, MEM/WB x2 each)
```

## Interface

### Clock and Reset
- **clk** (input): System clock (100 MHz target)
- **rst** (input): Synchronous reset (active high)

### Unified Memory - Instruction Fetch Port
- **mem_if_addr** (output, 32-bit): Instruction fetch address
- **mem_if_req** (output): Fetch request signal
- **mem_if_rdata** (input, 128-bit): 16 bytes of instruction data (big-endian)
- **mem_if_ack** (input): Fetch acknowledge

### Unified Memory - Data Access Port  
- **mem_data_addr** (output, 32-bit): Data access address
- **mem_data_wdata** (output, 32-bit): Write data
- **mem_data_size** (output, 2-bit): Access size (byte/half/word)
- **mem_data_we** (output): Write enable
- **mem_data_req** (output): Data request
- **mem_data_rdata** (input, 32-bit): Read data
- **mem_data_ack** (input): Data acknowledge

### Status Signals
- **halted** (output): CPU halted (HLT executed)
- **current_pc** (output, 32-bit): Current program counter
- **dual_issue_active** (output): Dual-issue occurred this cycle

## Internal State

### CPU Flags
- **z_flag, v_flag**: Zero and Overflow flags
- Updated by writeback stage based on most recent instruction

### Control Signals
- **stall_pipeline**: Global stall (from hazards or memory)
- **flush_if, flush_id, flush_ex**: Pipeline flush signals (branches)
- **branch_taken**: Branch taken this cycle
- **branch_target**: Target address for branch

## Pipeline Flow

### 1. Fetch Stage (IF)
- fetch_unit fetches variable-length instructions
- Maintains 32-byte buffer for dual-issue
- Outputs up to 2 instructions per cycle

### 2. IF/ID Pipeline Registers
- Two registers (if_id_reg_0, if_id_reg_1)
- Hold fetched instructions, PC, length
- Flush on branch taken

### 3. Decode Stage (ID)
- Two decode_unit instances decode in parallel
- issue_unit determines if dual-issue possible
- register_file provides 4 read ports for operands

### 4. ID/EX Pipeline Registers  
- Two registers (id_ex_reg_0, id_ex_reg_1)
- Hold decoded control signals and operands
- Flush on branch or hazard stall

### 5. Execute Stage (EX)
- execute_stage integrates dual ALU, multiply, branch units
- Forwarding MUXes select operands from 6 possible sources
- Branches resolve here (2-cycle penalty if taken)

### 6. EX/MEM Pipeline Registers
- Two registers (ex_mem_reg_0, ex_mem_reg_1)
- Hold ALU results, memory addresses, branch decisions

### 7. Memory Stage (MEM)
- memory_stage arbitrates between dual memory requests
- Only one memory access per cycle (structural limitation)
- Second instruction stalls if both need memory

### 8. MEM/WB Pipeline Registers
- Two registers (mem_wb_reg_0, mem_wb_reg_1)
- Hold final results ready for write-back

### 9. Write-Back Stage (WB)
- writeback_stage writes up to 2 results to register file
- Updates CPU flags (z_flag, v_flag)
- Detects HLT instruction

## Forwarding Paths

The hazard_unit generates forwarding control for 4 operands (2 per instruction):

**Forwarding Sources** (in priority order):
1. **EX/MEM register outputs** (ex_mem_out_0/1.alu_result) - Most recent
2. **MEM/WB register outputs** (mem_wb_out_0/1.wb_data)
3. **WB stage outputs** (rf_wr_data_0/1) - Final write-back

**CRITICAL**: Forwarding data must come from **pipeline register outputs**, not combinational execute stage outputs, to avoid combinational loops.

## Stall Conditions

Pipeline stalls when:
1. **hazard_stall**: Load-use hazard detected
2. **mem_stall**: Memory not ready or second instruction waiting for memory
3. **halted**: HLT instruction reached write-back

## Branch Handling

Branches resolve in EX stage:
- **Prediction**: Static not-taken
- **Taken penalty**: 2 cycles (flush IF, ID)
- **Branch target**: From immediate or register (JSR/RTS)

## Timing

- **Clock Period**: 10 ns (100 MHz target)
- **Critical Path**: Forwarding MUX → ALU → Result selection (~8-9 ns)
- **Pipeline Latency**: 5 cycles (IF → ID → EX → MEM → WB)

## Design Decisions

### Why Von Neumann?
- Simpler than Harvard (single memory, single address space)
- Dual-port BRAM allows simultaneous IF and data access
- No artificial code/data partition

### Why Big-Endian?
- Instruction decode simpler (MSB first)
- Human-readable memory dumps
- Network byte order compatible

### Why 5 Stages?
- Classic RISC balance
- Well-understood hazard handling
- Achievable timing on target FPGA

## Known Limitations

1. **Single Data Port**: Only one memory access per cycle limits dual-issue
2. **No Branch Prediction**: All branches have 2-cycle penalty if taken
3. **In-Order Issue**: Cannot reorder for better parallelism
4. **Load-Use Stalls**: 1-cycle penalty, cannot be forwarded earlier

## Synthesis Notes

- **Target**: Lattice ECP5-85F FPGA
- **LUT Usage**: ~8,000 (10% of 83,920)
- **Registers**: ~3,000 (4%)
- **BRAM**: 64 KB (31% of 208 KB)
- **Fmax**: ~100-120 MHz (limited by ALU critical path)

## Verification

See core_unified_tb.sv for integration tests.

