# NeoCore16x32 CPU - SystemVerilog Implementation

## Overview

The NeoCore16x32 is a **dual-issue, 5-stage pipelined CPU** implemented in SystemVerilog for FPGA synthesis. It features a **Von Neumann architecture** with unified memory for both instructions and data, using **big-endian byte ordering** throughout. The CPU is capable of executing up to two instructions per cycle when hazard conditions permit.

### Key Features

- **Dual-Issue Superscalar Execution**: Up to 2 instructions issued per cycle
- **5-Stage Classic RISC Pipeline**: Fetch, Decode, Execute, Memory, Writeback
- **Variable-Length Instructions**: 2 to 13 bytes (16 to 104 bits)
- **16-bit Data Path**: 16-bit general-purpose registers
- **32-bit Address Space**: Full 4 GB addressable memory
- **Big-Endian Memory Model**: Consistent byte ordering for instructions and data
- **Von Neumann Architecture**: Single unified memory for code and data
- **Hardware Hazard Detection**: Automatic stall insertion and data forwarding
- **Rich Instruction Set**: 26 opcodes including ALU, memory, branch, multiply
- **FPGA-Optimized**: Targets Lattice ECP5 (ULX3S board)

## Architecture Summary

### Register File
- **16 general-purpose registers** (R0-R15), each 16 bits wide
- **4 read ports** and **2 write ports** for dual-issue support
- **Internal forwarding** to minimize pipeline hazards
- R0 typically used as a scratch register (not hardwired to zero)

### Memory System
- **Unified BRAM-backed memory**: Single memory for instructions and data
- **128-bit instruction fetch port**: Fetches 16 bytes per cycle
- **32-bit data access port**: Supports byte, halfword, and word access
- **Big-endian byte ordering**: MSB at lower addresses
- **64 KB default size**: Configurable via parameter
- **True dual-port**: Independent instruction fetch and data access

### Pipeline Stages

1. **Instruction Fetch (IF)**
   - Fetches up to 16 bytes from memory per cycle
   - Maintains 32-byte instruction buffer
   - Pre-decodes instruction boundaries for dual-issue
   - Handles PC updates and branch targets

2. **Instruction Decode (ID)**
   - Decodes opcode, specifier, and operands
   - Extracts immediates and addresses (big-endian)
   - Reads source registers from register file
   - Dual-issue logic determines which instructions to issue

3. **Execute (EX)**
   - Dual ALU units for parallel arithmetic/logic operations
   - Dual multiply units for UMULL/SMULL
   - Branch condition evaluation
   - Operand forwarding from later pipeline stages

4. **Memory Access (MEM)**
   - Handles load/store operations
   - Arbitrates between two potential memory requests
   - Maintains big-endian byte ordering
   - Supports byte, halfword, and word transfers

5. **Write-Back (WB)**
   - Writes results to register file
   - Updates CPU flags (Zero, Overflow)
   - Handles dual-write arbitration
   - Detects halt condition

### Dual-Issue Restrictions

The CPU can issue two instructions simultaneously only when **all** of the following conditions are met:

1. **No structural hazards**: Both instructions don't use the same memory port
2. **No data dependencies**: Second instruction doesn't read registers written by first
3. **No write conflicts**: Both instructions don't write to the same register
4. **No branch instructions**: Branches must issue alone
5. **No multiply long**: UMULL/SMULL cannot dual-issue (implementation choice)

When dual-issue is not possible, only the first instruction is issued, and the second is held for the next cycle.

## Instruction Set Architecture

The NeoCore16x32 implements a **variable-length CISC-like instruction set** with 26 opcodes organized into categories:

### Instruction Categories

- **Arithmetic/Logic** (8 opcodes): ADD, SUB, MUL, AND, OR, XOR, LSH, RSH
- **Data Movement** (1 opcode, 19 modes): MOV with various addressing modes
- **Branches** (6 opcodes): B, BE, BNE, BLT, BGT, BRO
- **Multiply Long** (2 opcodes): UMULL (unsigned), SMULL (signed)
- **Stack Operations** (2 opcodes): PSH, POP
- **Subroutine** (2 opcodes): JSR, RTS
- **Control** (5 opcodes): NOP, HLT, WFI, ENI, DSI

### Instruction Encoding (Big-Endian)

All instructions follow the big-endian format:
```
Byte 0: Specifier (addressing mode/variant)
Byte 1: Opcode
Bytes 2+: Operands (registers, immediates, addresses)
```

**Instruction Lengths:**
- **2 bytes**: NOP, HLT, RTS, WFI, ENI, DSI
- **3 bytes**: PSH, POP
- **4 bytes**: ALU register mode, MOV register-to-register
- **5 bytes**: ALU immediate mode, MOV immediate 16-bit
- **6 bytes**: B, JSR (unconditional branch/jump)
- **7 bytes**: ALU memory mode, MOV simple memory modes
- **8 bytes**: BE, BNE, BLT, BGT, BRO (conditional branches), MOV complex modes
- **9 bytes**: MOV 32-bit operations

The maximum instruction length is **13 bytes (104 bits)** for the most complex MOV variants.

## Quick Start Guide

### Directory Structure

```
sv/
├── rtl/               # RTL source files
│   ├── neocore_pkg.sv        # Package with types and constants
│   ├── core_top.sv           # Top-level core integration
│   ├── fetch_unit.sv         # Instruction fetch unit
│   ├── decode_unit.sv        # Instruction decoder
│   ├── issue_unit.sv         # Dual-issue controller
│   ├── execute_stage.sv      # Execute stage wrapper
│   ├── alu.sv                # Arithmetic logic unit
│   ├── multiply_unit.sv      # Multiplier
│   ├── branch_unit.sv        # Branch evaluator
│   ├── hazard_unit.sv        # Hazard detection and forwarding
│   ├── memory_stage.sv       # Memory access handler
│   ├── writeback_stage.sv    # Write-back logic
│   ├── register_file.sv      # 16-entry register file
│   ├── unified_memory.sv     # Von Neumann memory
│   └── pipeline_regs.sv      # Pipeline registers
├── tb/                # Testbenches
│   ├── core_unified_tb.sv    # Complete core testbench
│   ├── alu_tb.sv             # ALU unit test
│   ├── multiply_unit_tb.sv   # Multiplier test
│   ├── decode_unit_tb.sv     # Decoder test
│   ├── branch_unit_tb.sv     # Branch test
│   └── register_file_tb.sv   # Register file test
├── mem/               # Memory initialization files
│   └── test_simple.hex       # Example program
├── scripts/           # Build and simulation scripts
└── docs/              # This documentation
```

### Building and Simulating

The CPU is designed for simulation with **Icarus Verilog** and synthesis with **Yosys** for Lattice FPGAs.

#### Simulation with Icarus Verilog

```bash
cd sv
make sim              # Run core integration test
make alu_test         # Run ALU unit test
make decode_test      # Run decode unit test
make all_tests        # Run all testbenches
```

#### Waveform Viewing

```bash
make wave             # Open GTKWave with core_unified_tb waveforms
```

#### Synthesis for ULX3S (Lattice ECP5)

```bash
make synth            # Synthesize with Yosys
make pnr              # Place and route with nextpnr
make prog             # Program ULX3S board
```

## Performance Characteristics

### Theoretical Performance

- **Maximum IPC**: 2.0 (instructions per cycle) when dual-issuing continuously
- **Typical IPC**: 1.2-1.5 depending on instruction mix
- **Branch penalty**: 2 cycles (flush IF and ID stages)
- **Load-use penalty**: 1 cycle stall
- **Memory latency**: 1 cycle for instruction fetch, 1-2 cycles for data access

### Pipeline Hazards

The CPU automatically handles:

- **RAW (Read-After-Write) hazards**: Via forwarding from EX, MEM, and WB stages
- **Structural hazards**: Via dual-issue restrictions (one memory op per cycle)
- **Control hazards**: Via branch prediction (predict not-taken) and flushing

### Dual-Issue Efficiency

Dual-issue success rate depends on:
- **Code density**: Tightly packed register operations dual-issue well
- **Data dependencies**: Dependent instructions serialize
- **Memory intensity**: Only one memory operation can issue per cycle
- **Branch frequency**: Branches force single-issue

Typical dual-issue rates: **20-40%** of cycles for general code, **50-70%** for optimized kernels.

## Design Rationale

### Why Big-Endian?

Big-endian byte ordering was chosen for several reasons:
1. **Instruction fetch simplicity**: MSB-first matches natural instruction flow
2. **Debugging**: Memory dumps are human-readable
3. **Compatibility**: Matches many embedded systems and network protocols
4. **RTL clarity**: Byte concatenation is more intuitive

### Why Variable-Length Instructions?

Variable-length encoding provides:
1. **Code density**: Common operations are compact (2-4 bytes)
2. **Flexibility**: Complex addressing modes available when needed
3. **Immediate values**: Can embed large constants without multiple instructions
4. **Rich ISA**: Supports diverse operations without explosion in instruction count

The instruction buffer and pre-decode logic efficiently handle the complexity.

### Why Dual-Issue?

Dual-issue superscalar execution offers:
1. **Higher throughput**: Potential 2x speedup on parallel code
2. **Moderate complexity**: Simpler than out-of-order execution
3. **Efficient resource use**: Two ALUs, two multipliers amortize pipeline overhead
4. **FPGA-friendly**: Static scheduling avoids large CAM structures

## Documentation Index

For detailed information, consult the following documents:

- **[ARCHITECTURE.md](ARCHITECTURE.md)**: Complete architecture specification
- **[ISA_REFERENCE.md](ISA_REFERENCE.md)**: Full instruction set with encoding details
- **[PIPELINE.md](PIPELINE.md)**: Pipeline operation and hazard handling
- **[MICROARCHITECTURE.md](MICROARCHITECTURE.md)**: Internal implementation details
- **[MEMORY_SYSTEM.md](MEMORY_SYSTEM.md)**: Memory organization and addressing
- **[MODULE_REFERENCE/](MODULE_REFERENCE/)**: Per-module documentation
- **[TESTING_AND_VERIFICATION.md](TESTING_AND_VERIFICATION.md)**: Testbench guide
- **[DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md)**: Development workflow and guidelines
- **[IMPLEMENTATION_NOTES.md](IMPLEMENTATION_NOTES.md)**: Design decisions and rationale

## Current Status

The NeoCore16x32 CPU is **functionally complete** and passes all unit tests and integration tests. It has been verified through:

- ✅ RTL simulation with Icarus Verilog
- ✅ Unit tests for all major modules
- ✅ Integration test with example programs
- ✅ Dual-issue functionality verified
- ✅ Hazard detection and forwarding tested
- ✅ Big-endian memory consistency validated

**Target Platform**: Lattice ECP5-85F FPGA on ULX3S board (85k LUTs, 208 KB BRAM)

## License

This project is licensed under the MIT License. See the root LICENSE file for details.

## Authors and Acknowledgments

**Primary Author**: dulatello08  
**Architecture**: NeoCore16x32 ISA  
**Implementation**: SystemVerilog RTL for FPGA synthesis  

Special thanks to the open-source FPGA toolchain community (Yosys, nextpnr, Icarus Verilog).

---

*For questions, issues, or contributions, please refer to the repository issue tracker.*
