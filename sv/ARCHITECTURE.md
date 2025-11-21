# NeoCore16x32 Architecture Specification

## Table of Contents

1. [Introduction](#introduction)
2. [Data Types and Byte Ordering](#data-types-and-byte-ordering)
3. [Register Architecture](#register-architecture)
4. [Memory Organization](#memory-organization)
5. [Instruction Format](#instruction-format)
6. [Addressing Modes](#addressing-modes)
7. [Processor Flags](#processor-flags)
8. [Pipeline Architecture](#pipeline-architecture)
9. [Dual-Issue Execution Model](#dual-issue-execution-model)
10. [Exception and Interrupt Model](#exception-and-interrupt-model)

---

## Introduction

The NeoCore16x32 is a modern 16-bit CPU architecture with 32-bit addressing capability, designed for FPGA implementation. The architecture combines RISC-like pipeline simplicity with CISC-like variable-length instruction encoding to achieve high code density while maintaining straightforward hardware implementation.

### Design Philosophy

1. **Performance through Parallelism**: Dual-issue superscalar execution
2. **Code Density**: Variable-length instructions minimize program size
3. **Simplicity**: Classic 5-stage RISC pipeline with in-order execution
4. **FPGA-Friendly**: Block RAM utilization, no complex CAM structures
5. **Deterministic**: Predictable timing for real-time applications

### Architectural Family

The NeoCore16x32 is a **load-store architecture** with **Von Neumann** memory organization:
- Data operations occur only between registers
- Explicit load/store instructions for memory access
- Single unified address space for code and data
- Big-endian byte ordering throughout

---

## Data Types and Byte Ordering

### Native Data Types

The architecture supports three fundamental data types:

| Type | Size | Alignment | Description |
|------|------|-----------|-------------|
| **Byte** | 8 bits | None | Unsigned byte value (0-255) |
| **Halfword** | 16 bits | Even address | Primary data type, registers are halfwords |
| **Word** | 32 bits | 4-byte boundary | For addresses and 32-bit operations |

### Big-Endian Byte Ordering

**All data and instructions use big-endian byte ordering** where the most significant byte (MSB) resides at the lowest memory address.

#### Halfword (16-bit) in Memory

```
Address:     N      N+1
Content:    [MSB]  [LSB]
           bits    bits
           15-8    7-0
```

**Example**: The halfword value `0x1234` at address `0x1000`:
- Address `0x1000`: `0x12` (MSB)
- Address `0x1001`: `0x34` (LSB)

#### Word (32-bit) in Memory

```
Address:     N      N+1    N+2    N+3
Content:    [MSB]  [....]  [....]  [LSB]
           bits    bits    bits    bits
           31-24   23-16   15-8    7-0
```

**Example**: The word value `0xDEADBEEF` at address `0x2000`:
- Address `0x2000`: `0xDE` (bits 31-24)
- Address `0x2001`: `0xAD` (bits 23-16)
- Address `0x2002`: `0xBE` (bits 15-8)
- Address `0x2003`: `0xEF` (bits 7-0)

### Endianness in Instructions

Instructions are also big-endian. The instruction format always places:
- **Byte 0** (specifier) at the lowest address
- **Byte 1** (opcode) at the next address
- **Operands** following in big-endian order

For a 5-byte instruction at address `0x100`:
```
Address  Content      Field
0x100    0x00         Specifier
0x101    0x01         Opcode (ADD)
0x102    0x05         rd (register 5)
0x103    0x12         Immediate MSB
0x104    0x34         Immediate LSB (immediate = 0x1234)
```

---

## Register Architecture

### General-Purpose Registers

The NeoCore16x32 provides **16 general-purpose registers** numbered R0 through R15. Each register is **16 bits wide**.

```
Register Number:  0    1    2    3    4    5    6    7    8    9    10   11   12   13   14   15
Name:            R0   R1   R2   R3   R4   R5   R6   R7   R8   R9   R10  R11  R12  R13  R14  R15
Width:           16   16   16   16   16   16   16   16   16   16   16   16   16   16   16   16
```

#### Register Properties

- **No special register**: R0 is NOT hardwired to zero (unlike MIPS). It's a fully general-purpose register.
- **Register addressing**: 4-bit fields in instructions (0x0 to 0xF)
- **Dual-port read**: Up to 4 simultaneous reads for dual-issue
- **Dual-port write**: Up to 2 simultaneous writes for dual-issue

#### Register Conventions (Software)

While hardware treats all registers equally, software conventions may establish:
- **R0-R7**: Temporary/scratch registers
- **R8-R11**: Callee-saved registers
- **R12-R13**: Argument/return value registers  
- **R14**: Stack pointer (SP) - used by PSH/POP
- **R15**: Link register (LR) - used by JSR/RTS

**Note**: These are **conventions**, not hardware enforcements. The RTL makes no assumptions about register usage.

### Byte Access to Registers

Individual bytes of registers can be accessed using MOV instruction variants:

- **R#.L**: Lower byte (bits 7-0) of register #
- **R#.H**: Higher byte (bits 15-8) of register #

Example:
```
Register R3 = 0x1234
R3.H = 0x12
R3.L = 0x34
```

MOV instructions can load/store individual bytes:
- `MOV R1.L, [0x2000]` - Load byte from memory into R1 bits 7-0
- `MOV R1.H, [0x2001]` - Load byte from memory into R1 bits 15-8
- `MOV [0x3000], R2.L` - Store R2 bits 7-0 to memory
- `MOV [0x3001], R2.H` - Store R2 bits 15-8 to memory

### Special State Registers

In addition to the 16 general-purpose registers, the CPU maintains special state:

#### Program Counter (PC)
- **Width**: 32 bits
- **Function**: Points to the next instruction to fetch
- **Access**: Indirect via branch instructions (B, BE, JSR, etc.)
- **Increment**: Variable (2 to 13 bytes) based on instruction length

#### Status Flags
- **Z (Zero) Flag**: Set if result is zero
- **V (Overflow) Flag**: Set if result exceeds 16 bits

These flags are updated by arithmetic/logic operations and tested by conditional branches.

---

## Memory Organization

### Address Space

The NeoCore16x32 uses a **32-bit address space** providing 4 GB of addressable memory.

```
Address Range:         0x00000000 to 0xFFFFFFFF (4 GB)
Physically Implemented: Typically 64 KB (0x0000 to 0xFFFF)
```

The RTL implementation defaults to **64 KB of unified memory**, but this is parameterized and can be adjusted.

### Von Neumann Architecture

Unlike Harvard architectures with separate instruction and data memories, the NeoCore16x32 uses a **unified memory** that holds both code and data.

#### Memory Access Ports

The unified_memory module provides two independent access ports:

1. **Instruction Fetch Port (IF)**
   - **Width**: 128 bits (16 bytes)
   - **Purpose**: Fetch variable-length instructions
   - **Latency**: 1 cycle
   - **Bandwidth**: Up to 16 bytes/cycle

2. **Data Access Port (DATA)**
   - **Width**: 32 bits (4 bytes maximum)
   - **Granularity**: Byte, halfword, or word
   - **Latency**: 1 cycle
   - **Operations**: Load and store

Both ports access the same underlying memory array but operate independently, allowing simultaneous instruction fetch and data access.

### Memory Map

While the architecture doesn't enforce a specific memory map, typical usage follows:

```
0x00000000 +------------------+
           |   Boot Sector    |  Code entry point
           |    (32 pages)    |
0x00008000 +------------------+
           |   MMIO Region 1  |  UART device
           |    (1 page)      |
0x00010000 +------------------+
           |   MMIO Region 2  |  PIC (interrupt controller)
           |    (1 page)      |
0x00020000 +------------------+
           |   Stack Memory   |  Stack grows downward
           |    (2 pages)     |
0x00030000 +------------------+
           |   Flash Memory   |  Non-volatile storage
           |    (8 pages)     |
0x00050000 +------------------+
           |   (Unused)       |
           ~                  ~
0xFFFFFFFF +------------------+
```

**Note**: This is a **suggested** layout from config.ini. The CPU architecture itself is agnostic to memory mapping.

### Memory Alignment

- **Byte access**: No alignment required (any address)
- **Halfword access**: **Should** be even-aligned (address bit 0 = 0)
- **Word access**: **Should** be 4-byte aligned (address bits 1-0 = 00)

**Important**: The current RTL does NOT enforce alignment. Unaligned halfword/word accesses will succeed but may produce unexpected results in real hardware. Software should ensure proper alignment.

### Big-Endian Memory Operations

All memory reads and writes respect big-endian byte ordering:

#### Byte Read/Write (MEM_BYTE)
- Address N reads/writes a single byte
- No endianness issue (single byte)

#### Halfword Read/Write (MEM_HALF)
- Address N: MSB (bits 15-8)
- Address N+1: LSB (bits 7-0)
- Reading address 0x1000 with value {0x12, 0x34} returns 0x1234

#### Word Read/Write (MEM_WORD)
- Address N: MSB (bits 31-24)
- Address N+1: (bits 23-16)
- Address N+2: (bits 15-8)
- Address N+3: LSB (bits 7-0)
- Reading address 0x2000 with value {0xDE, 0xAD, 0xBE, 0xEF} returns 0xDEADBEEF

---

## Instruction Format

All NeoCore16x32 instructions follow a common format with variable length.

### Basic Instruction Structure

```
+--------+--------+--------+--------+--------+-----+--------+
| Byte 0 | Byte 1 | Byte 2 | Byte 3 | Byte 4 | ... | Byte N |
+--------+--------+--------+--------+--------+-----+--------+
|  Spec  | Opcode |   rd   |   rn   |   rn1  | ... | Oper   |
+--------+--------+--------+--------+--------+-----+--------+
```

**Byte 0: Specifier**
- 8-bit field indicating addressing mode or instruction variant
- Determines instruction length and operand interpretation

**Byte 1: Opcode**
- 8-bit field identifying the operation (ADD, SUB, MOV, B, etc.)
- Maps to enum opcode_e in neocore_pkg.sv

**Bytes 2+: Operands**
- Registers (rd, rn, rn1)
- Immediate values
- Memory addresses
- Branch targets
- Length varies by instruction and specifier

### Instruction Length Determination

The CPU determines instruction length by examining the **opcode and specifier** fields. The get_inst_length() function in neocore_pkg.sv implements this logic.

#### Fixed-Length Instructions (2 bytes)

These instructions have no operands or implicit operands:
- NOP (0x00), HLT (0x12), RTS (0x16)
- WFI (0x17), ENI (0x18), DSI (0x19)

Format:
```
+--------+--------+
|  Spec  | Opcode |
+--------+--------+
```

#### Short Instructions (3 bytes)

Stack operations with single register operand:
- PSH (0x13), POP (0x14)

Format:
```
+--------+--------+--------+
|  Spec  | Opcode |   rd   |
+--------+--------+--------+
```

#### Medium Instructions (4-5 bytes)

ALU register mode (specifier 0x01):
```
+--------+--------+--------+--------+
|  0x01  | ALU_OP |   rd   |   rn   |
+--------+--------+--------+--------+
```

ALU immediate mode (specifier 0x00):
```
+--------+--------+--------+--------+--------+
|  0x00  | ALU_OP |   rd   |  Imm H |  Imm L |
+--------+--------+--------+--------+--------+
```

#### Long Instructions (6-9 bytes)

Unconditional branch:
```
+--------+--------+--------+--------+--------+--------+
|  0x00  |  0x0A  | Addr3  | Addr2  | Addr1  | Addr0  |
+--------+--------+--------+--------+--------+--------+
           (B)       MSB                        LSB
```

Conditional branch (e.g., BE):
```
+--------+--------+--------+--------+--------+--------+--------+--------+
|  0x00  |  0x0B  |   rd   |   rn   | Addr3  | Addr2  | Addr1  | Addr0  |
+--------+--------+--------+--------+--------+--------+--------+--------+
           (BE)                        MSB                        LSB
```

#### Maximum Length Instructions (13 bytes)

Some MOV variants can reach 13 bytes, though the most common maximum is 9 bytes.

### Instruction Fetch and Buffering

The fetch_unit maintains a **32-byte instruction buffer** to handle variable-length instructions:

1. Fetch 16 bytes from memory each cycle (when buffer needs refilling)
2. Extract specifier and opcode from top of buffer
3. Calculate instruction length using get_inst_length()
4. Extract full instruction bytes (up to 13)
5. Pre-decode second instruction for dual-issue
6. Consume valid instructions (shift buffer)

This design allows the CPU to:
- Handle instructions spanning memory fetch boundaries
- Pre-decode two instructions simultaneously
- Maintain high instruction bandwidth despite variable length

---

## Addressing Modes

The NeoCore16x32 supports multiple addressing modes primarily through the MOV instruction's specifier variants.

### Register Direct

Operand is in a register.

**Example**: `ADD R1, R2` (R1 = R1 + R2)
- Encoding: Specifier=0x01, Opcode=0x01, rd=1, rn=2

### Immediate

Operand is a constant in the instruction.

**Example**: `ADD R5, #0x1234`
- Encoding: Specifier=0x00, Opcode=0x01, rd=5, immediate=0x1234
- Length: 5 bytes

### Absolute (Direct)

Operand is at a fixed memory address.

**Example**: `MOV R3, [0x12345678]`
- Encoding: Specifier=0x05, Opcode=0x09, rd=3, address=0x12345678
- Length: 7 bytes
- Loads halfword from address 0x12345678 into R3

### Register Indirect with Offset

Operand is at address = register + offset.

**Example**: `MOV R2, [R4 + 0x100]`
- Encoding: Specifier=0x0D, Opcode=0x09, rd=2, rn=4, offset=0x100
- Length: 8 bytes
- Address = R4 + 0x100

### Specifier-Based Mode Selection

Different ALU and MOV instruction specifiers select modes:

**ALU Instructions**:
- Specifier 0x00: Immediate mode (rd, #imm16)
- Specifier 0x01: Register mode (rd, rn)
- Specifier 0x02: Memory mode (rd, [addr32])

**MOV Instructions** (19 specifiers):
- 0x00: Immediate to register
- 0x01: 32-bit immediate to register pair
- 0x02: Register to register
- 0x03-0x06: Memory to register (byte, byte-high, halfword, word)
- 0x07-0x0A: Register to memory (byte, byte-high, halfword, word)
- 0x0B-0x0E: Memory with offset to register
- 0x0F-0x12: Register to memory with offset

---

## Processor Flags

The CPU maintains two condition flags updated by arithmetic/logic operations.

### Zero Flag (Z)

**Set when**: The lower 16 bits of an operation result are zero  
**Cleared when**: The result is non-zero

**Example**:
```
R1 = 0x0005
R2 = 0x0005
SUB R1, R2      ; R1 = R1 - R2 = 0, Z flag SET
```

**Used by**: Not directly tested by conditional branches in current ISA. Reserved for future use.

### Overflow Flag (V)

**Set when**: An arithmetic operation result exceeds 16 bits (bits beyond bit 15 are non-zero)  
**Cleared when**: The result fits in 16 bits

**Example**:
```
R1 = 0xFFFF
R2 = 0x0002
ADD R1, R2      ; R1 = 0x0001 (truncated), V flag SET (0x10001 truncated)
```

**Used by**: BRO (Branch if Overflow) instruction tests this flag.

### Flag Update Rules

Flags are updated by:
- ALU operations: ADD, SUB, MUL, AND, OR, XOR, LSH, RSH
- Multiply long: UMULL, SMULL (based on 32-bit result)

Flags are NOT updated by:
- MOV instructions
- Load/store operations
- Branch instructions
- Stack operations (PSH, POP)

### Flag Timing

Flags are updated in the **Write-Back (WB) stage**, so they reflect the result of the most recently completed instruction. In dual-issue scenarios, if both instructions update flags, the second instruction's flags take precedence.

---

## Pipeline Architecture

The NeoCore16x32 implements a classic **5-stage pipeline**:

```
IF  ->  ID  ->  EX  ->  MEM  ->  WB
```

### Pipeline Stage Descriptions

#### 1. Instruction Fetch (IF)

**Function**: Fetch instructions from memory  
**Module**: fetch_unit.sv

**Operations**:
- Request 16 bytes from unified memory (instruction fetch port)
- Maintain 32-byte circular buffer of fetched bytes
- Extract first instruction (specifier + opcode → determine length)
- Extract second instruction for dual-issue
- Update PC based on consumed instruction bytes

**Output**: Two potential instructions (inst_data_0, inst_data_1) with PC and length

**Key Feature**: Pre-decodes instruction boundaries to enable dual-issue without stalling.

#### 2. Instruction Decode (ID)

**Function**: Decode instructions and read operands  
**Modules**: decode_unit.sv, issue_unit.sv, register_file.sv

**Operations**:
- Parse big-endian instruction fields (opcode, specifier, registers, immediates)
- Generate control signals (ALU op, memory size, branch type)
- Read source registers from register file (4 read ports)
- Determine dual-issue feasibility (issue_unit)
- Select which instructions to issue this cycle

**Output**: Decoded instructions with operands and control signals

**Dual-Issue Logic**: The issue_unit checks for hazards between the two instructions and decides if both can proceed.

#### 3. Execute (EX)

**Function**: Perform arithmetic, logic, and branch evaluation  
**Modules**: execute_stage.sv, alu.sv, multiply_unit.sv, branch_unit.sv

**Operations**:
- Select operands (with forwarding from later stages)
- Execute ALU operations (dual ALU instances for dual-issue)
- Perform 16x16→32 multiplication (dual multiply units)
- Evaluate branch conditions
- Calculate memory addresses for load/store
- Determine if branch is taken

**Output**: ALU results, memory addresses, branch decision

**Forwarding**: Operand forwarding MUXes select data from EX, MEM, or WB stages to resolve RAW hazards.

#### 4. Memory Access (MEM)

**Function**: Access unified memory for load/store  
**Module**: memory_stage.sv

**Operations**:
- Arbitrate between two potential memory requests (dual-issue)
- Issue load/store to unified memory (data access port)
- Handle byte, halfword, and word accesses with big-endian ordering
- Forward loaded data for forwarding paths
- Sequence multiple memory accesses if both instructions need memory

**Output**: Memory read data or ALU result pass-through

**Note**: Only one memory operation can complete per cycle. If both instructions need memory, the second stalls.

#### 5. Write-Back (WB)

**Function**: Write results to register file and update flags  
**Module**: writeback_stage.sv

**Operations**:
- Write results to register file (2 write ports)
- Update Z and V flags based on results
- Detect HALT instruction
- Handle dual-write arbitration for 32-bit operations (UMULL/SMULL)

**Output**: Register write enables and data, flag updates

### Pipeline Registers

Data flows between stages through pipeline registers:

- **IF/ID**: Holds fetched instructions
- **ID/EX**: Holds decoded control signals and register operands
- **EX/MEM**: Holds ALU results and memory operation info
- **MEM/WB**: Holds final results ready for write-back

Each register has:
- **Stall capability**: Freeze current data when pipeline stalls
- **Flush capability**: Insert pipeline bubble (NOP) on branch mispredict

### Pipeline Timing Diagram

Example execution of three independent ADD instructions:

```
Cycle:    1      2      3      4      5      6      7      8
         +------+------+------+------+------+------+------+------+
ADD R1   | IF   | ID   | EX   | MEM  | WB   |      |      |      |
         +------+------+------+------+------+------+------+------+
ADD R2   |      | IF   | ID   | EX   | MEM  | WB   |      |      |
         +------+------+------+------+------+------+------+------+
ADD R3   |      |      | IF   | ID   | EX   | MEM  | WB   |      |
         +------+------+------+------+------+------+------+------+
```

Under ideal dual-issue:

```
Cycle:    1      2      3      4      5      6
         +------+------+------+------+------+------+
ADD R1   | IF   | ID   | EX   | MEM  | WB   |      |
         +------+------+------+------+------+------+
ADD R2   | IF   | ID   | EX   | MEM  | WB   |      |
         +------+------+------+------+------+------+
ADD R3   |      | IF   | ID   | EX   | MEM  | WB   |
         +------+------+------+------+------+------+
ADD R4   |      | IF   | ID   | EX   | MEM  | WB   |
         +------+------+------+------+------+------+
```

Instructions 1&2 issue together, then 3&4 issue together, achieving 2 instructions/cycle.

---

## Dual-Issue Execution Model

The NeoCore16x32 can execute **up to two instructions simultaneously** using dual-issue in-order superscalar execution.

### Dual-Issue Concept

In each cycle, the pipeline attempts to:
1. Fetch and decode two instructions
2. Check for hazards and dependencies
3. Issue both if safe, otherwise issue only the first

### Issue Restrictions

Two instructions can issue together ONLY if all these conditions hold:

#### 1. No Structural Hazards

**Memory Port Conflict**: Only ONE instruction may access memory (load/store) per cycle.
- ❌ `MOV R1, [0x1000]` + `MOV R2, [0x2000]` (both load)
- ❌ `MOV [0x1000], R1` + `MOV [0x2000], R2` (both store)
- ❌ `ADD R1, [0x1000]` + `MOV R2, [0x2000]` (both memory)
- ✅ `ADD R1, R2` + `MOV R3, [0x1000]` (only second uses memory)

**Register Write Port Conflict**: Both instructions cannot write to the same register.
- ❌ `ADD R1, R2` + `SUB R1, R3` (both write R1)
- ✅ `ADD R1, R2` + `SUB R3, R4` (different destinations)

#### 2. No Data Dependencies

If instruction 1 writes a register that instruction 2 reads, they cannot dual-issue (RAW hazard within issue).
- ❌ `ADD R1, R2` + `SUB R3, R1` (inst2 reads R1 written by inst1)
- ✅ `ADD R1, R2` + `SUB R3, R4` (no dependency)

**Note**: Dependencies with earlier pipeline stages are handled by forwarding, not by preventing dual-issue.

#### 3. No Branch Instructions

Branches must issue alone to simplify control flow.
- ❌ `ADD R1, R2` + `B 0x1000` (second is branch)
- ❌ `BE R1, R2, 0x1000` + `ADD R3, R4` (first is branch)
- ✅ Branch issues alone

This includes: B, BE, BNE, BLT, BGT, BRO, JSR, RTS

#### 4. No Multiply Long

UMULL and SMULL are complex (32-bit result, two register writes) and cannot dual-issue.
- ❌ `UMULL R1, R2, R3` + `ADD R4, R5` (first is UMULL)
- ✅ UMULL issues alone

### Dual-Issue Success Conditions

Examples of instructions that CAN dual-issue:

```
ADD R1, R2  +  SUB R3, R4     ✅ (both ALU, no dependencies)
ADD R1, R2  +  AND R3, R4     ✅
MOV R1, #5  +  MOV R2, #10    ✅ (both immediate MOV)
ADD R1, R2  +  MOV [0x1000], R3  ✅ (first ALU, second memory)
XOR R1, R2  +  LSH R3, R4     ✅
```

Examples that CANNOT dual-issue:

```
ADD R1, R2  +  SUB R3, R1     ❌ (data dependency: R1)
ADD R1, R2  +  ADD R1, R3     ❌ (write conflict: R1)
MOV R1, [0x1000]  +  MOV R2, [0x2000]  ❌ (both use memory)
ADD R1, R2  +  B 0x1000       ❌ (branch restriction)
UMULL R1, R2, R3  +  ADD R4, R5  ❌ (UMULL restriction)
```

### Dual-Issue Implementation

The issue_unit.sv module examines both decoded instructions and outputs:
- **issue_inst0**: Always TRUE if inst0 is valid
- **issue_inst1**: TRUE only if both can dual-issue
- **dual_issue**: TRUE if both are being issued this cycle

If dual-issue fails, instruction 0 proceeds and instruction 1 waits in the IF/ID register for the next cycle.

### Performance Impact

Dual-issue effectiveness depends on code characteristics:

| Code Type | Dual-Issue Rate | IPC |
|-----------|-----------------|-----|
| Pure ALU (independent) | 60-70% | 1.6-1.7 |
| Mixed ALU+Memory | 20-30% | 1.2-1.3 |
| Heavy branching | 10-15% | 1.1-1.15 |
| Tight loops (data dependencies) | 15-25% | 1.15-1.25 |

IPC = Instructions Per Cycle (average)

Theoretical maximum: **2.0 IPC**  
Realistic sustained: **1.2-1.5 IPC**

---

## Exception and Interrupt Model

The current RTL implementation includes **placeholders** for interrupt and exception handling but does not fully implement them.

### Reserved Instructions

Three opcodes are reserved for future interrupt/exception support:
- **WFI (0x17)**: Wait For Interrupt
- **ENI (0x18)**: Enable Interrupts
- **DSI (0x19)**: Disable Interrupts

These decode correctly but have minimal functionality (treated as NOPs currently).

### Future Interrupt Model (Planned)

The intended interrupt model (based on opcodes and emulator config) is:

1. **Interrupt Sources**: External devices (UART, PIC)
2. **Interrupt Enable**: Global interrupt enable flag (via ENI/DSI)
3. **Interrupt Vector**: Fixed address for ISR entry point
4. **Context Save**: Software saves registers to stack
5. **Return**: RTS returns from interrupt handler

This is **not implemented in the current RTL** but the instruction set is designed to support it.

---

## Summary

The NeoCore16x32 architecture combines:
- **16-bit data path** with **32-bit addresses** for compact yet capable processing
- **Variable-length instructions** (2-13 bytes) for code density
- **Big-endian byte ordering** consistently throughout
- **Von Neumann memory** with unified code/data space
- **Dual-issue superscalar** execution for performance
- **5-stage pipeline** with hardware hazard detection

The architecture is designed for FPGA implementation with emphasis on:
- Synthesizable RTL
- Block RAM utilization
- Deterministic timing
- Straightforward microarchitecture

This document defines the **architectural specification** - what the programmer sees. For details on how this is implemented in RTL, see [MICROARCHITECTURE.md](MICROARCHITECTURE.md).

