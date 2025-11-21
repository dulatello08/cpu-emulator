# NeoCore 16x32 Dual-Issue 5-Stage Pipelined CPU Core

## Overview

This directory contains a SystemVerilog implementation of a dual-issue, in-order, 5-stage pipelined CPU core that executes the NeoCore 16x32 ISA. The design faithfully implements the same instruction set and behavior as the C emulator found in the parent directory.

## Architecture Summary

### Register Architecture
- **General-purpose registers**: 16 registers (R0-R15), each 16 bits wide
- **Program Counter (PC)**: 32 bits
- **Flags**: 
  - Z (Zero flag): Set when result is zero
  - V (Overflow flag): Set when arithmetic overflow occurs
- **Stack Pointer**: Managed through memory-mapped region

### Memory Model
- **Address space**: 32-bit addressing (4GB addressable)
- **Data width**: 16-bit registers, 8/16/32-bit memory accesses
- **Page size**: 4KB pages
- **No caches**: Direct memory interface for simplicity
- **MMU**: Separate module interface (stub implementation for now)

### Instruction Set Architecture (ISA)

The NeoCore 16x32 ISA is a variable-length instruction set with instructions ranging from 2 to 9 bytes. Each instruction begins with:
- **Byte 0**: Specifier (addressing mode/variant)
- **Byte 1**: Opcode

#### Instruction Categories

1. **Arithmetic/Logic** (ADD, SUB, MUL, AND, OR, XOR, LSH, RSH)
   - 3 modes: Immediate, Register-Register, Memory
   - Length: 4-7 bytes depending on mode
   - Updates Z and V flags

2. **Data Movement** (MOV)
   - 19 specifiers for different addressing modes
   - Supports 8/16/32-bit transfers
   - Register-to-register, immediate, memory with various addressing modes
   - Length: 4-9 bytes

3. **Branches** (B, BE, BNE, BLT, BGT, BRO)
   - Unconditional branch (B): 6 bytes
   - Conditional branches: 8 bytes, compare two registers
   - BRO: Branch on overflow flag
   - All branches use 32-bit absolute addresses

4. **Multiply Long** (UMULL, SMULL)
   - Unsigned/Signed 16x16 → 32-bit multiplication
   - Results split across two registers (low/high)
   - Length: 4 bytes

5. **Stack Operations** (PSH, POP)
   - Push/Pop 16-bit values
   - Stack managed in dedicated memory region
   - Length: 3 bytes

6. **Subroutine** (JSR, RTS)
   - JSR: Jump to subroutine, saves 32-bit return address
   - RTS: Return from subroutine
   - Length: 6 bytes (JSR), 2 bytes (RTS)

7. **Control** (NOP, HLT, WFI)
   - NOP: No operation
   - HLT: Halt execution
   - WFI: Wait for interrupt
   - Length: 2 bytes

### Complete Instruction Encoding Reference

| Opcode | Mnemonic | Specifiers | Description |
|--------|----------|------------|-------------|
| 0x00 | NOP | - | No operation |
| 0x01 | ADD | 00,01,02 | Add (imm/reg/mem) |
| 0x02 | SUB | 00,01,02 | Subtract |
| 0x03 | MUL | 00,01,02 | Multiply (truncated 16-bit) |
| 0x04 | AND | 00,01,02 | Bitwise AND |
| 0x05 | OR | 00,01,02 | Bitwise OR |
| 0x06 | XOR | 00,01,02 | Bitwise XOR |
| 0x07 | LSH | 00,01,02 | Left shift |
| 0x08 | RSH | 00,01,02 | Right shift |
| 0x09 | MOV | 00-12 | Move data (19 modes) |
| 0x0A | B | 00 | Unconditional branch |
| 0x0B | BE | 00 | Branch if equal |
| 0x0C | BNE | 00 | Branch if not equal |
| 0x0D | BLT | 00 | Branch if less than |
| 0x0E | BGT | 00 | Branch if greater than |
| 0x0F | BRO | 00 | Branch if overflow |
| 0x10 | UMULL | 00 | Unsigned multiply long |
| 0x11 | SMULL | 00 | Signed multiply long |
| 0x12 | HLT | 00 | Halt |
| 0x13 | PSH | 00 | Push to stack |
| 0x14 | POP | 00 | Pop from stack |
| 0x15 | JSR | 00 | Jump to subroutine |
| 0x16 | RTS | 00 | Return from subroutine |
| 0x17 | WFI | 00 | Wait for interrupt |

## Microarchitecture

### Pipeline Stages

The core implements a classic 5-stage in-order pipeline:

1. **IF (Instruction Fetch)**
   - Fetches up to 2 instructions from memory
   - Handles variable-length instruction alignment
   - Pre-decodes instruction length for next fetch
   - PC is updated based on branch decisions or sequential flow

2. **ID (Instruction Decode)**
   - Decodes opcode and specifier
   - Extracts operands (registers, immediates, addresses)
   - Reads source registers from register file
   - Determines instruction type and resource requirements

3. **EX (Execute)**
   - ALU operations
   - Branch condition evaluation
   - Address calculation for memory operations
   - Forwarding from later stages if available

4. **MEM (Memory Access)**
   - Load/Store operations
   - Memory interface transactions
   - Stack operations (push/pop use memory)

5. **WB (Write Back)**
   - Write results to register file
   - Update flags (Z, V)
   - Commit architectural state

### Dual-Issue Rules

The core can issue up to 2 instructions per cycle, subject to these restrictions:

1. **Structural Hazards**
   - At most **one memory operation** (load/store) per cycle
   - At most **one branch** per cycle
   - Both instructions cannot write to the same register
   - Stack operations (PSH/POP) count as memory operations

2. **Data Hazards**
   - RAW (Read-After-Write): Detect and stall if second instruction reads a register written by first
   - WAW (Write-After-Write): Prevented by structural hazard rule
   - WAR (Write-After-Read): Not an issue in in-order pipeline
   - Forwarding from EX, MEM, and WB stages reduces stalls

3. **Control Hazards**
   - Branches cannot dual-issue (always issue alone)
   - Branch resolution in EX stage
   - Flush IF and ID stages on taken branch
   - 2-cycle penalty for taken branches

4. **Instruction Pairing Restrictions**
   - Simple ALU/logic operations can dual-issue together
   - Memory operations cannot dual-issue with each other
   - Branches always issue alone
   - Long multiply (UMULL/SMULL) cannot dual-issue

### Hazard Detection and Resolution

#### Data Hazard Forwarding
The core implements forwarding paths:
- **EX → EX**: Forward ALU result to next instruction in EX stage
- **MEM → EX**: Forward memory load or ALU result from MEM stage
- **WB → EX**: Forward write-back data to EX stage

#### Stall Conditions
The pipeline stalls when:
- Load-use hazard: Instruction in EX loads from memory, next instruction in ID needs that result
- Dual-issue conflict: Second instruction has unresolvable dependency
- Memory contention: Both instructions need memory access

#### Branch Handling
- Branches resolved in EX stage
- Flush IF/ID stages on taken branch
- Continue fetching sequentially for not-taken branches
- No branch prediction (always predict not-taken)

### Memory Interface

The core uses a simple synchronous memory interface:

```systemverilog
// Instruction Memory Interface
output logic [31:0] imem_addr;     // Instruction fetch address
output logic        imem_req;      // Request valid
input  logic [63:0] imem_rdata;    // Up to 8 bytes of instruction data
input  logic        imem_ack;      // Data valid

// Data Memory Interface  
output logic [31:0] dmem_addr;     // Data address
output logic [15:0] dmem_wdata;    // Write data (up to 32-bit across 2 cycles)
output logic [ 1:0] dmem_size;     // 00=byte, 01=halfword, 10=word
output logic        dmem_we;       // Write enable
output logic        dmem_req;      // Request valid
input  logic [31:0] dmem_rdata;    // Read data
input  logic        dmem_ack;      // Data valid
```

### Module Hierarchy

```
core_top.sv                 - Top-level CPU core
├── fetch_unit.sv          - Instruction fetch, PC management
├── decode_unit.sv         - Instruction decode, operand extraction
├── issue_unit.sv          - Dual-issue control, hazard detection
├── register_file.sv       - 16x16-bit register file with forwarding
├── alu.sv                 - Arithmetic/Logic unit
├── multiply_unit.sv       - 16x16 multiply (signed/unsigned)
├── load_store_unit.sv     - Memory access controller
├── branch_unit.sv         - Branch condition evaluation
├── pipeline_regs.sv       - Pipeline stage registers
│   ├── if_id_reg.sv
│   ├── id_ex_reg.sv
│   ├── ex_mem_reg.sv
│   └── mem_wb_reg.sv
└── mmu_stub.sv           - MMU interface (placeholder)
```

## Design Decisions and Assumptions

### Ambiguity Resolutions

1. **Variable-Length Instructions in Dual-Issue**
   - Fetch buffer holds up to 16 bytes
   - Pre-decode identifies instruction boundaries
   - Can issue 2 instructions only if both fully available in buffer

2. **Memory Access Ordering**
   - Memory operations are strictly in-order
   - No speculative memory accesses
   - Single memory port (shared by instruction and data when needed)

3. **Stack Pointer Management**
   - Stack pointer stored in first 4 bytes of STACK memory region
   - PSH/POP operations update SP via memory write-back
   - SP not visible as a CPU register in this implementation

4. **Interrupt Handling**
   - WFI instruction stalls pipeline until interrupt
   - Interrupt handling logic deferred to future implementation
   - Core provides signals for interrupt controller integration

5. **Unimplemented Features (Future Work)**
   - No caching
   - No branch prediction beyond static not-taken
   - No out-of-order execution
   - MMU is stub only

### Reset Behavior

All synchronous logic resets on the positive edge of `clk` when `rst` is high:
- PC resets to 0x00000000
- All pipeline registers cleared
- Flags cleared (Z=0, V=0)
- Register file contents undefined (can be initialized to 0 for simulation)

### Coding Conventions

- **Language**: SystemVerilog (IEEE 1800-2012)
- **Naming**: snake_case for all signals, modules, parameters
- **Clock**: `input logic clk` in all sequential modules
- **Reset**: `input logic rst` (active-high, synchronous) in all sequential modules
- **Sequential logic**: Use `always_ff @(posedge clk)`
- **Combinational logic**: Use `always_comb`
- **No synthesis-unfriendly constructs**: No `#` delays in RTL, no `$display` in RTL
- **Comments**: Extensive comments explaining design intent

## Testing Strategy

### Unit Tests

1. **alu_tb.sv** - Test all ALU operations, flag generation
2. **register_file_tb.sv** - Test reads, writes, forwarding
3. **multiply_unit_tb.sv** - Test UMULL and SMULL
4. **fetch_unit_tb.sv** - Test instruction fetch, alignment, PC update
5. **decode_unit_tb.sv** - Test instruction decoding for all opcodes
6. **issue_unit_tb.sv** - Test dual-issue logic, hazard detection
7. **branch_unit_tb.sv** - Test branch conditions

### Integration Tests

1. **core_smoke_tb.sv** - End-to-end test with small programs
   - Simple arithmetic sequence
   - Branch taken/not-taken
   - Load/store operations
   - Subroutine call/return

### Test Programs

Test programs are written in assembly (using existing toolchain in parent directory) and converted to hex format for `$readmemh` in testbenches.

Example test program structure:
```
program.asm → assembler → program.bin → bin2hex → program.hex
                                          ↓
                                    testbench loads program.hex
                                          ↓
                                    simulation runs
                                          ↓
                                    compare against emulator
```

## Building and Running

### Prerequisites
- Icarus Verilog (`iverilog`) version 10.0 or later
- VVP (Verilog simulation runtime)
- GTKWave (optional, for waveform viewing)

### Build Commands

```bash
# Build and run all tests
make all

# Build specific testbench
make alu_tb

# Run specific test
make run_alu_tb

# View waveforms (if .vcd files generated)
make wave_alu_tb

# Clean build artifacts
make clean
```

### Makefile Targets

- `all`: Build and run all tests
- `<module>_tb`: Compile specific testbench
- `run_<module>_tb`: Run specific test simulation
- `wave_<module>_tb`: Open waveform in GTKWave
- `clean`: Remove all build artifacts

## File Organization

```
sv/
├── README.md           - This file
├── Makefile            - Build system
├── rtl/                - RTL source files
│   ├── core_top.sv
│   ├── fetch_unit.sv
│   ├── decode_unit.sv
│   ├── issue_unit.sv
│   ├── register_file.sv
│   ├── alu.sv
│   ├── multiply_unit.sv
│   ├── load_store_unit.sv
│   ├── branch_unit.sv
│   ├── pipeline_regs.sv
│   └── mmu_stub.sv
├── tb/                 - Testbenches
│   ├── alu_tb.sv
│   ├── register_file_tb.sv
│   ├── multiply_unit_tb.sv
│   ├── fetch_unit_tb.sv
│   ├── decode_unit_tb.sv
│   ├── issue_unit_tb.sv
│   ├── branch_unit_tb.sv
│   └── core_smoke_tb.sv
├── mem/                - Memory images and test programs
│   ├── test_alu.hex
│   ├── test_branch.hex
│   └── test_subroutine.hex
└── scripts/            - Helper scripts
    ├── bin2hex.py
    └── run_test.sh
```

## Current Implementation Status

### Completed Modules

The following RTL modules are **fully implemented and tested**:

1. **neocore_pkg.sv** - Package definitions
   - Complete opcode enumeration
   - Instruction type classification
   - ALU operation encoding
   - Pipeline stage structures (if_id_t, id_ex_t, ex_mem_t, mem_wb_t)
   - Helper functions for instruction length calculation

2. **alu.sv** - Arithmetic Logic Unit
   - All ALU operations: ADD, SUB, MUL, AND, OR, XOR, LSH, RSH
   - Flag generation (Z, V)
   - Fully tested with comprehensive testbench

3. **multiply_unit.sv** - Multiply Unit
   - 16x16 → 32-bit multiplication
   - Both signed (SMULL) and unsigned (UMULL) modes
   - Results split into low/high 16-bit registers
   - Fully tested

4. **branch_unit.sv** - Branch Condition Evaluation
   - All branch types: B, BE, BNE, BLT, BGT, BRO, JSR
   - Comparisons and branch decision logic
   - Fully tested

5. **register_file.sv** - Register File
   - 16 registers × 16 bits
   - Dual-issue capable (4 read ports, 2 write ports)
   - Internal forwarding logic to minimize hazards
   - Fully tested

6. **decode_unit.sv** - Instruction Decoder
   - Complete decoder for all NeoCore instruction formats
   - Extracts opcodes, register addresses, immediates, addresses
   - Generates control signals for pipeline stages
   - Fully tested on major instruction types

7. **fetch_unit.sv** - Instruction Fetch
   - Variable-length instruction fetching
   - Instruction buffer with alignment handling
   - PC management with branch support
   - Pre-decode for instruction lengths
   - Dual-issue ready (can extract 2 instructions per cycle)

8. **pipeline_regs.sv** - Pipeline Stage Registers
   - IF/ID, ID/EX, EX/MEM, MEM/WB registers
   - Stall and flush capabilities
   - Type-safe using package structures

9. **simple_memory.sv** - Memory Model (for simulation)
   - Combined instruction/data memory
   - Synchronous interface
   - Supports byte, halfword, and word accesses
   - Loadable from hex files for testing

### Modules Still Needed for Complete Core

To complete a full working 5-stage pipelined core, the following modules are still required:

1. **hazard_unit.sv** - Hazard Detection and Forwarding Control
   - Data hazard detection (RAW dependencies)
   - Load-use hazard detection
   - Forwarding path selection
   - Pipeline stall generation

2. **execute_stage.sv** - Execute Stage Integration
   - Integrates ALU, multiply unit, and branch unit
   - Operand selection and forwarding MUXes
   - Branch target calculation
   - Flag management

3. **memory_stage.sv** - Memory Access Stage
   - Load/store unit implementation
   - Memory address alignment checking
   - Data formatting for different access sizes (byte/half/word)
   - Stack pointer management for PSH/POP/JSR/RTS

4. **writeback_stage.sv** - Write-Back Stage
   - Result selection (ALU result vs memory load vs PC+offset)
   - Register write port arbitration
   - Flag register updates

5. **core_top.sv** - Top-Level Core Integration
   - Instantiates all pipeline stages
   - Connects pipeline registers
   - Wires hazard unit to all stages
   - Exposes clean external memory interface

6. **issue_unit.sv** - Dual-Issue Control (for full dual-issue)
   - Determines which instruction pairs can issue together
   - Structural hazard detection (memory port conflicts, register write conflicts)
   - Ensures dual-issue restrictions are met

### Testing Infrastructure

**Unit Tests (All Passing)**
- ✅ alu_tb.sv - Tests all ALU operations and flag generation
- ✅ register_file_tb.sv - Tests register reads/writes and forwarding
- ✅ multiply_unit_tb.sv - Tests UMULL and SMULL with various inputs
- ✅ branch_unit_tb.sv - Tests all branch conditions
- ✅ decode_unit_tb.sv - Tests instruction decoding for main opcodes

**Integration Tests (To Be Implemented)**
- [ ] core_smoke_tb.sv - Simple program execution end-to-end
- [ ] hazard_tb.sv - Test hazard detection and forwarding
- [ ] branch_flush_tb.sv - Test pipeline flush on branch
- [ ] stack_ops_tb.sv - Test PSH/POP/JSR/RTS sequences

### Path to Completion

**Phase 1: Single-Issue Pipeline (Recommended First Step)**

For a working prototype, implement a **single-issue, in-order, 5-stage pipeline**:

1. Create `hazard_unit.sv` for data hazard detection and forwarding
2. Create `execute_stage.sv` integrating ALU, branch, and multiply units
3. Create `memory_stage.sv` for loads/stores (initially without full stack support)
4. Create `writeback_stage.sv` for result selection and register writes
5. Create `core_top.sv` integrating all stages
6. Test with simple programs (arithmetic, branches, memory access)

**Phase 2: Stack and Subroutine Support**

1. Extend `memory_stage.sv` with stack pointer management
2. Implement PSH/POP operations
3. Implement JSR/RTS (push/pop return address on stack)
4. Test with recursive subroutine calls

**Phase 3: Dual-Issue Extension**

1. Create `issue_unit.sv` with dual-issue decision logic
2. Extend `hazard_unit.sv` to handle dual-issue dependencies
3. Add resource arbitration (memory ports, write ports)
4. Enforce dual-issue restrictions:
   - At most one memory operation per cycle
   - At most one branch per cycle
   - No write port conflicts
5. Test with programs that can exploit instruction-level parallelism

### Design Philosophy and Trade-offs

The current implementation prioritizes:

- **Correctness**: Each module is thoroughly tested individually
- **Clarity**: Code is heavily commented and structured for educational value
- **Modularity**: Clean interfaces between modules for easy integration
- **Synthesizability**: Uses only synthesizable SystemVerilog constructs in RTL

Trade-offs made:

- **Performance vs. Simplicity**: A simpler single-issue pipeline is easier to verify than dual-issue
- **Completeness vs. Time**: Implementing a fully verified dual-issue pipeline requires extensive integration testing
- **Features**: Advanced features (branch prediction, caching, out-of-order) are deferred

### Dual-Issue Design Notes

For dual-issue implementation, the key challenges are:

**1. Structural Hazards**
   - Need to detect when both instructions need the same resource
   - Memory: Only one memory access allowed per cycle
   - Write ports: Both instructions can't write to same register

**2. Data Hazards in Dual-Issue**
   - Instruction 1 writes register that instruction 0 needs → stall instruction 1
   - Instruction 1 reads register that instruction 0 writes → can forward, but complicates timing
   - Need to check dependencies between the two issuing instructions AND with in-flight instructions

**3. Control Hazards**
   - Branches must issue alone (flush is simpler with single branch)
   - Branch in instruction 0 → don't issue instruction 1

**4. Implementation Strategy**
   - Pre-decode stage: Identify instruction boundaries and types
   - Issue logic: Apply dual-issue rules before ID stage
   - If can't issue both: Issue only instruction 0, buffer instruction 1
   - Hazard unit: Extended to consider both instructions

### Recommended Next Steps for Developer

1. **Implement hazard_unit.sv** with data hazard detection:
   ```systemverilog
   // Inputs: ID/EX, EX/MEM, MEM/WB register addresses and write enables
   // Outputs: Forwarding MUX selects, stall signal
   ```

2. **Integrate ALU into execute_stage.sv**:
   ```systemverilog
   // MUX operand_a: rs1_data, forwarded from EX, forwarded from MEM, forwarded from WB
   // MUX operand_b: rs2_data, immediate, forwarded data
   ```

3. **Create simple core_top.sv** connecting the pipeline:
   ```systemverilog
   // Instantiate fetch, decode, execute, memory, writeback
   // Connect pipeline registers
   // Wire hazard unit
   ```

4. **Write core_smoke_tb.sv**:
   ```systemverilog
   // Load a simple program: add some numbers, store to memory, branch, halt
   // Run core, verify final register state and memory contents
   ```

5. **Iterate**: Fix bugs found in integration testing, add missing features

## Future Enhancements

1. **Branch Prediction**: Add simple 2-bit saturating counter predictor
2. **Caching**: Add instruction and data caches
3. **MMU**: Full virtual memory support with TLB
4. **Performance Counters**: Cycle count, instruction count, stalls, etc.
5. **Interrupt Controller**: Full interrupt support (WFI, vector table)
6. **Superscalar Issue**: More aggressive dual-issue (out-of-order)
7. **FPGA Synthesis**: Target specific FPGA platform (Xilinx/Intel)

## References

- Parent directory C emulator: `../emulator.c`, `../execute_instructions.c`
- ISA documentation: `../Instructions.md`
- Architecture details: `../README.md`, `../common.h`, `../constants.h`

## License

Same as parent project (GNU GPL v3 or later)
