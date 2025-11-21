# NeoCore16x32 Implementation Notes

## Overview

This document explains the design decisions, trade-offs, and rationale behind the NeoCore16x32 CPU implementation. It provides insight into why certain choices were made and documents alternative approaches that were considered.

## Design Philosophy

### Core Principles

1. **Big-Endian Consistency**: Use big-endian byte ordering throughout for clarity and human readability
2. **FPGA-First**: Optimize for FPGA synthesis rather than ASIC
3. **Verifiability**: Keep design simple enough to verify thoroughly
4. **Performance Balance**: Dual-issue for performance without excessive complexity
5. **Code Density**: Variable-length instructions for compact code

---

## Big-Endian Design Decision

### Rationale

**Why big-endian instead of little-endian?**

1. **Instruction Decode Simplicity**: MSB-first matches natural instruction decoding order
   - Specifier and opcode at beginning (lower addresses)
   - Operands follow in order
   - No need to reverse bytes during decode

2. **Human Readability**: Memory dumps and waveforms are more intuitive
   - Address 0x1000 contains MSB of 0x1234
   - Hexadecimal values read left-to-right match memory order
   - Debugging is easier when addresses increase naturally

3. **Network Byte Order**: Big-endian is standard for network protocols
   - Potential future use in network applications
   - Compatibility with Ethernet, IP, TCP/UDP

4. **Historical Precedent**: Many embedded systems use big-endian
   - Motorola 68k, PowerPC (optional), SPARC
   - Proven in real-world applications

### Trade-offs

**Disadvantages**:
- Less common than little-endian in modern x86/ARM ecosystems
- Some software libraries assume little-endian
- Byte-swapping needed when interfacing with little-endian systems

**Mitigation**:
- Clear documentation of byte order
- Consistent enforcement in all RTL modules
- Helper functions for byte swapping if needed

### Implementation Details

All multi-byte values use big-endian encoding:

```verilog
// 16-bit immediate from instruction bytes
immediate = {byte_msb, byte_lsb};  // NOT {byte_lsb, byte_msb}

// 32-bit address from instruction bytes
address = {byte0, byte1, byte2, byte3};  // byte0 is MSB
```

Memory storage is big-endian:
```verilog
// Store halfword
mem[addr + 0] <= data[15:8];  // MSB at lower address
mem[addr + 1] <= data[7:0];   // LSB at higher address
```

---

## Variable-Length Instructions

### Rationale

**Why variable-length instead of fixed-length?**

1. **Code Density**: Common operations are compact
   - NOP, HLT: 2 bytes
   - Register-to-register ALU: 4 bytes
   - Average instruction: ~5 bytes vs. 8-16 bytes for fixed-length

2. **Flexible Addressing**: Large immediates and addresses fit in single instruction
   - 32-bit address embedded directly: 6-8 bytes
   - No need for address synthesis across multiple instructions

3. **Rich ISA**: Diverse addressing modes without instruction explosion
   - MOV has 19 variants with single opcode
   - Different lengths for different complexities

### Trade-offs

**Complexity**:
- Variable-length fetch requires instruction buffer
- Pre-decode logic to determine boundaries
- Alignment challenges across memory fetch boundaries

**Solutions**:
- 32-byte circular buffer in fetch_unit
- Pre-decode specifier+opcode to calculate length
- Wide 128-bit fetch (16 bytes) to span boundaries
- get_inst_length() function centralizes logic

### Implementation Details

**Instruction Buffer**:
```verilog
logic [255:0] fetch_buffer;  // 32 bytes
logic [5:0]   buffer_valid;  // Valid byte count
```

**Length Calculation** (in neocore_pkg.sv):
```verilog
function automatic logic [3:0] get_inst_length(
  logic [7:0] opcode,
  logic [7:0] specifier
);
  case (opcode)
    8'h00: return 4'd2;  // NOP
    8'h01: begin  // ADD
      case (specifier)
        8'h00: return 4'd5;  // Immediate
        8'h01: return 4'd4;  // Register
        8'h02: return 4'd7;  // Memory
      endcase
    end
    // ...
  endcase
endfunction
```

**Dual-Issue Pre-Decode**:
- Extract first instruction (bytes 0-12)
- Calculate first_length
- Extract second instruction (bytes first_length to first_length+12)
- Calculate second_length
- Both available in same cycle for issue decision

---

## Dual-Issue Implementation

### Rationale

**Why dual-issue instead of single-issue or out-of-order?**

1. **Performance**: Up to 2x throughput without excessive complexity
2. **FPGA-Friendly**: Static issue logic, no reorder buffer or register renaming
3. **Power Efficiency**: Better IPC without complex dynamic scheduling
4. **Deterministic**: In-order execution simplifies debugging and verification

**Why not out-of-order?**
- Too complex for FPGA (large content-addressable memories, reorder buffers)
- Higher power consumption
- Non-deterministic timing problematic for real-time applications
- Verification burden much higher

### Design Choices

**Issue Restrictions**:

We chose to allow dual-issue only when:
1. No structural hazards (memory port, write ports)
2. No data dependencies within issue pair
3. No branches
4. No multiply-long operations

**Alternative considered**: Allow dependent instructions to dual-issue with intra-pair forwarding
- Rejected because: Adds forwarding paths and MUX complexity
- Not worth the benefit given other restrictions

**Branch Restriction**:
- **Current**: Branches issue alone
- **Alternative**: Allow instruction before branch to dual-issue
- **Rejected**: Complicates branch flush logic; not worth marginal gain

**Memory Restriction**:
- **Current**: Only one memory operation per cycle
- **Alternative**: Dual-port data memory
- **Rejected**: Doubles BRAM usage; fetch port already uses one port

**Multiply Restriction**:
- **Current**: UMULL/SMULL cannot dual-issue
- **Rationale**: They write two destination registers, consuming both write ports
- **Alternative**: Allow with careful scheduling
- **Rejected**: Added complexity not worth it for infrequent instructions

### Implementation Details

**Dual ALU**:
```verilog
alu alu_0 (.operand_a(operand_a_0), .operand_b(operand_b_0), ...);
alu alu_1 (.operand_a(operand_a_1), .operand_b(operand_b_1), ...);
```

**Dual Multiply**:
```verilog
multiply_unit mul_0 (...);
multiply_unit mul_1 (...);
```

**Issue Unit Logic**:
```verilog
issue_inst0 = inst0_valid;
issue_inst1 = inst0_valid && inst1_valid &&
              !mem_port_conflict &&
              !write_port_conflict &&
              !branch_restriction &&
              !data_dependency &&
              !mul_restriction;
```

---

## Hazard Handling

### Forwarding Paths

**Rationale**: Minimize stalls by forwarding results as soon as available

**Forwarding Sources**:
1. **EX/MEM register**: ALU result available immediately after EX stage
2. **MEM/WB register**: Loaded data or ALU result after MEM stage
3. **WB outputs**: Final write-back data

**Priority**: EX > MEM > WB (most recent wins)

### Load-Use Hazards

**Problem**: Load data not available until after MEM stage, but next instruction's EX needs it

**Solution**: Stall pipeline for 1 cycle

**Alternative considered**: Move loads earlier or add bypass
- Rejected: Would complicate memory stage or add forwarding path from memory output

**Implementation**:
```verilog
if (ex_mem_read && ((ex_rd_addr == id_rs1_addr) || (ex_rd_addr == id_rs2_addr))) begin
  stall = 1'b1;
  flush_ex = 1'b1;  // Insert bubble
end
```

### Branch Hazards

**Strategy**: Predict not-taken, resolve in EX, flush if taken

**Rationale**:
- Simple hardware (no branch prediction table)
- Early resolution (EX vs. MEM or WB)
- Acceptable 2-cycle penalty

**Alternative considered**: Branch prediction (BTB, BHT)
- Rejected: Added complexity, limited benefit for small programs
- Future enhancement possible

---

## Pipeline Organization

### 5-Stage Pipeline

**Why 5 stages instead of 3, 7, or more?**

1. **Classic RISC balance**: IF, ID, EX, MEM, WB are natural divisions
2. **Timing**: Each stage ~8-9 ns at 100 MHz, achievable on FPGA
3. **Hazard complexity**: More stages = more forwarding paths
4. **Verification**: 5 stages well-understood, easy to verify

**Stage Breakdown**:
- **IF**: Instruction fetch (memory access latency)
- **ID**: Decode + register read
- **EX**: ALU/multiply/branch (arithmetic latency)
- **MEM**: Data memory access (memory access latency)
- **WB**: Register write (setup time)

### Pipeline Registers

**Design Choice**: Separate register modules for each pipeline boundary

**Rationale**:
- Clear stage separation
- Easier to add stall/flush logic
- Better for synthesis (register inference)

**Alternative**: Single large pipeline register
- Rejected: Harder to manage, less modular

**Implementation** (pipeline_regs.sv):
```verilog
module if_id_reg ...
module id_ex_reg ...
module ex_mem_reg ...
module mem_wb_reg ...
```

Each has:
- `stall` input: Freeze current data
- `flush` input: Insert NOP
- Registered outputs

---

## Memory System

### Von Neumann vs. Harvard

**Choice**: Von Neumann (unified memory)

**Rationale**:
1. **Simpler**: Single address space, single memory block
2. **Flexible**: No fixed partition between code and data
3. **BRAM efficient**: One dual-port BRAM instead of two separate memories

**Trade-off**: Potential structural hazard if both ports needed

**Mitigation**: Dual-port BRAM allows simultaneous instruction fetch and data access

### Dual-Port Configuration

**Instruction Fetch Port**:
- Wide (128 bits / 16 bytes)
- Read-only
- Big-endian

**Data Access Port**:
- Narrow (32 bits / 4 bytes)
- Read-write
- Byte, halfword, word granularity
- Big-endian

**Rationale**:
- Wide instruction port amortizes fetch latency for variable-length instructions
- Narrow data port sufficient for 16-bit registers
- Separate ports avoid contention

### Memory Size

**Default**: 64 KB (configurable parameter)

**Rationale**:
- Fits target FPGA (Lattice ECP5 has 208 KB BRAM)
- Sufficient for moderate programs
- Leaves BRAM for future peripherals

**Scaling**:
- 16 KB: Minimal (tight loops, small programs)
- 64 KB: Good balance (current)
- 128 KB: Larger programs (uses 62% BRAM)
- 256 KB+: Requires external SRAM

---

## Register File

### Number of Registers

**Choice**: 16 registers (R0-R15)

**Rationale**:
1. **Balance**: Enough for most computations, not excessive
2. **Addressing**: 4-bit register fields in instructions
3. **FPGA Resources**: 16x16-bit = 256 bits total (small)

**Alternative considered**: 32 registers
- Rejected: Instruction encoding would need 5-bit fields, increasing instruction length
- Not worth the benefit given variable-length instructions can embed immediates

### Register File Ports

**Choice**: 4 read ports, 2 write ports

**Rationale**:
- **4 reads**: Dual-issue needs up to 4 source operands (2 per instruction)
- **2 writes**: Dual-issue needs up to 2 destinations

**Implementation**:
- Combinational read (no latency)
- Sequential write (on clock edge)
- Internal forwarding for same-cycle write-read

**Alternative**: 2 read, 1 write with staging
- Rejected: Would require extra pipeline stage or restrict dual-issue further

---

## Instruction Set Decisions

### Opcode Encoding

**Choice**: 8-bit opcode field

**Rationale**:
- 256 possible opcodes (plenty of room for future expansion)
- Current ISA uses 26 opcodes (10%)
- Simple decode (direct case statement)

**Specifier Field**:
- Also 8 bits
- Allows 256 addressing modes per opcode
- MOV uses 19 specifiers (flexible addressing)

### Stack Operations

**PSH/POP Design**:
- Implicit use of R14 as stack pointer
- No explicit stack pointer register in hardware

**Rationale**:
- Simple hardware (just register file operations + memory access)
- Software convention (R14 = SP)
- Flexibility (any register could be used as SP if needed)

**Alternative**: Dedicated stack pointer register
- Rejected: Adds hardware complexity, reduces flexibility

**Stack Direction**:
- Grows downward (high address to low)
- Common convention (matches x86, ARM)

### Multiply Instructions

**MUL vs. UMULL/SMULL**:

**MUL** (truncated):
- 16x16 → 16-bit result (truncated)
- Simple, fast
- Use when overflow not a concern

**UMULL/SMULL** (long):
- 16x16 → 32-bit result (two registers)
- Unsigned or signed
- Use when full precision needed

**Rationale**: Provide both for flexibility
- Simple MUL for common case
- Long multiply for when overflow matters (crypto, fixed-point math)

---

## Control Flow

### Branch Resolution Timing

**Choice**: Resolve branches in EX stage

**Rationale**:
- Early enough to minimize penalty (2 cycles)
- Late enough to have all operands (register values, flags)

**Alternative**: Resolve in ID
- Rejected: Would need forwarding or extra stalls for operand dependencies

**Alternative**: Resolve in MEM/WB
- Rejected: Larger branch penalty (3-4 cycles)

### No Branch Delay Slots

**Choice**: No delay slots (flush pipeline on taken branch)

**Rationale**:
- Simpler for programmers (no need to fill delay slots)
- Simpler hardware (no special delay slot handling)
- 2-cycle penalty acceptable

**Alternative**: MIPS-style delay slot
- Rejected: Complicates dual-issue (does delay slot instruction dual-issue?)
- Programmer burden not worth 1-cycle savings

---

## Future Enhancements

### Planned Features

1. **Interrupt Support**: Complete WFI, ENI, DSI implementation
   - Interrupt vector table
   - Automatic context save
   - Nested interrupt support

2. **Cache**: Instruction cache for larger programs
   - 4-way set-associative
   - 1-2 KB per way
   - Only if memory becomes bottleneck

3. **Hardware Multiply-Accumulate**: MAC instruction for DSP
   - `MAC RD, RA, RB` → RD += RA * RB
   - Useful for filters, dot products

4. **Conditional Execution**: ARM-style conditional bits
   - Reduce branch frequency
   - Improve dual-issue rate

### Rejected Features

1. **Floating-Point**: Too complex for target applications
   - Software floating-point library sufficient
   - FPGA resource intensive

2. **Virtual Memory**: Not needed for embedded system
   - Fixed address space simpler
   - No MMU overhead

3. **Out-of-Order Execution**: As discussed, too complex
   - In-order dual-issue sufficient for target performance

---

## Lessons Learned

### What Worked Well

1. **Big-Endian**: Consistent byte order simplified development and debugging
2. **Package File**: Centralizing types and functions (neocore_pkg.sv) avoided duplication
3. **Modular Pipeline Stages**: Easy to modify individual stages
4. **Variable-Length Buffer**: Fetch buffer effectively hides instruction length complexity
5. **Dual-Port Memory**: True dual-port BRAM eliminated structural hazards

### What Could Be Improved

1. **Instruction Encoding**: Some instructions have complex specifier mappings (MOV especially)
   - Consider orthogonal encoding for future ISAs

2. **Hazard Unit Complexity**: Checking all forwarding combinations is verbose
   - Could use generate blocks or functions to reduce code duplication

3. **Memory Alignment**: Not enforcing alignment means potential bugs
   - Consider adding alignment checks or restrictions

4. **Test Coverage**: More automated coverage would catch edge cases
   - Integrate with coverage tools in future

---

## Performance Analysis

### Achieved Performance

**Simulation Results** (typical code):
- **IPC**: 1.2-1.5 instructions per cycle
- **Dual-Issue Rate**: 20-40% of cycles
- **Clock Frequency**: 100 MHz target (achievable on ECP5)

**Theoretical Maximum**:
- **IPC**: 2.0 (perfect dual-issue)
- **Clock Frequency**: ~120 MHz (limited by ALU critical path)

### Bottlenecks

1. **Memory Access**: Only one data access per cycle
   - Limits dual-issue of memory-intensive code
   - Could add second data port (doubles BRAM usage)

2. **Branch Frequency**: Branches must issue alone
   - 10-20% of instructions are branches in typical code
   - Limits dual-issue rate

3. **Data Dependencies**: Register dependencies prevent dual-issue
   - 30-50% of adjacent instruction pairs have dependencies
   - Compiler optimization could help (instruction scheduling)

### Optimization Opportunities

**Software**:
- Instruction scheduling to maximize dual-issue
- Loop unrolling to reduce branches
- Minimize load-use hazards (insert independent instructions)

**Hardware**:
- Branch prediction to reduce branch penalty
- Wider issue (3-way?) if more execution units added
- Out-of-order execution (major change)

---

## Comparison to Other Architectures

### NeoCore16x32 vs. MIPS R3000

| Feature | NeoCore16x32 | MIPS R3000 |
|---------|--------------|------------|
| Pipeline Stages | 5 | 5 |
| Issue Width | 2 (dual) | 1 (single) |
| Instruction Length | Variable (2-13 bytes) | Fixed (32 bits) |
| Byte Order | Big-endian | Big-endian (configurable) |
| Register Count | 16 x 16-bit | 32 x 32-bit |
| Branch Delay | None | 1 slot |
| Multiply | Dedicated unit | Separate coprocessor |

**Advantages of NeoCore16x32**:
- Dual-issue for higher IPC
- Variable-length for code density
- Simpler (no coprocessor interface)

**Advantages of MIPS**:
- Fixed-length simplifies fetch
- Branch delay slot saves cycle
- More registers

### NeoCore16x32 vs. ARM7TDMI

| Feature | NeoCore16x32 | ARM7TDMI |
|---------|--------------|----------|
| Pipeline Stages | 5 | 3 |
| Issue Width | 2 | 1 |
| Instruction Length | Variable | Dual (ARM 32-bit, Thumb 16-bit) |
| Byte Order | Big-endian | Bi-endian (configurable) |
| Data Width | 16-bit | 32-bit |
| Conditional Execution | No | Yes (all instructions) |

**Advantages of NeoCore16x32**:
- Dual-issue for higher throughput
- Simpler pipeline (5 vs. 3 but more straightforward)

**Advantages of ARM7TDMI**:
- Conditional execution reduces branches
- Thumb mode for code density
- 32-bit data path

---

## Conclusion

The NeoCore16x32 design balances performance, complexity, and verifiability. Key decisions include:

1. **Big-endian** byte ordering for clarity
2. **Variable-length** instructions for code density
3. **Dual-issue** for performance without excessive complexity
4. **Von Neumann** memory for simplicity
5. **5-stage pipeline** for standard hazard handling

These choices result in a CPU that is:
- FPGA-friendly (efficient BRAM usage, achievable timing)
- Verifiable (moderate complexity, clear pipeline structure)
- Performant (1.2-1.5 IPC typical)
- Extensible (room for future enhancements)

The implementation successfully demonstrates a dual-issue processor suitable for embedded FPGA applications with performance competitive with commercial CPUs of similar complexity.

