# NeoCore 16x32 Architecture Deep Dive

## Von Neumann Memory System

### Overview

The NeoCore 16x32 CPU implements a **Von Neumann architecture** where instructions and data share a single unified memory space. This is in contrast to a Harvard architecture with separate instruction and data memories.

### Memory Organization

```
┌────────────────────────────────────────┐ 0x00000000
│                                        │
│     Unified Instruction/Data Space     │
│                                        │
│  - Instructions stored big-endian      │
│  - Data stored big-endian              │
│  - No separation between code/data     │
│                                        │
├────────────────────────────────────────┤ 0x0000FFFF
│                                        │ (64KB default)
│          Unmapped Region               │
│                                        │
└────────────────────────────────────────┘ 0xFFFFFFFF
```

###Big-Endian Byte Ordering

All data in memory follows **big-endian** byte ordering:
- Most significant byte at lowest address
- Least significant byte at highest address

**Examples:**

16-bit value 0x1234 at address 0x1000:
```
Address  | Value
---------|------
0x1000   | 0x12  (MSB)
0x1001   | 0x34  (LSB)
```

32-bit value 0x12345678 at address 0x2000:
```
Address  | Value
---------|------
0x2000   | 0x12  (MSB)
0x2001   | 0x34
0x2002   | 0x56
0x2003   | 0x78  (LSB)
```

Instruction example - MOV R1, #0xABCD (5 bytes) at address 0x100:
```
Address  | Value  | Description
---------|--------|-------------
0x0100   | 0x00   | Specifier
0x0101   | 0x09   | Opcode (MOV)
0x0102   | 0x01   | Destination register (R1)
0x0103   | 0xAB   | Immediate high byte
0x0104   | 0xCD   | Immediate low byte
```

### Dual-Port Memory Configuration

The unified memory provides two access ports to support simultaneous instruction fetch and data access:

**Port A - Instruction Fetch:**
- Width: 128 bits (16 bytes)
- Access: Read-only
- Purpose: Fetch variable-length instructions
- Latency: 1 cycle

**Port B - Data Access:**
- Width: 32 bits (4 bytes)
- Access: Read/Write
- Purpose: Load/store operations
- Sizes: Byte (8-bit), Halfword (16-bit), Word (32-bit)
- Latency: 1 cycle

### Memory Access Patterns

**Instruction Fetch (128-bit):**

Fetch from address 0x1000 returns 16 bytes:
```
Returned data[127:0]:
[127:120] = mem[0x1000]  // Byte 0
[119:112] = mem[0x1001]  // Byte 1
...
[7:0]     = mem[0x100F]  // Byte 15
```

**Data Read (Byte):**

Read byte from address 0x2000:
```
Returned data[31:0]:
[31:24] = mem[0x2000]    // Requested byte
[23:0]  = 0              // Zero-extended
```

**Data Read (Halfword):**

Read halfword from address 0x2000:
```
Returned data[31:0]:
[31:24] = mem[0x2000]    // High byte
[23:16] = mem[0x2001]    // Low byte
[15:0]  = 0              // Zero-extended
```

**Data Read (Word):**

Read word from address 0x2000:
```
Returned data[31:0]:
[31:24] = mem[0x2000]    // MSB
[23:16] = mem[0x2001]
[15:8]  = mem[0x2002]
[7:0]   = mem[0x2003]    // LSB
```

**Data Write Operations:**

Follow the same big-endian pattern:

Write halfword 0x1234 to address 0x3000:
```
mem[0x3000] ← 0x12
mem[0x3001] ← 0x34
```

## Variable-Length Instruction Handling

### Instruction Format

All NeoCore instructions begin with:
```
Byte 0: Specifier (addressing mode/variant)
Byte 1: Opcode
Bytes 2+: Operands (variable)
```

### Instruction Lengths

| Length | Instructions |
|--------|--------------|
| 2 bytes | NOP, HLT, RTS, WFI, ENI, DSI |
| 3 bytes | PSH, POP |
| 4 bytes | ALU ops (register mode), MOV (some variants) |
| 5 bytes | ALU ops (immediate mode), UMULL, SMULL, MOV (immediate) |
| 6 bytes | B, BRO, JSR |
| 7 bytes | ALU ops (memory mode), MOV (some memory variants) |
| 8 bytes | BE, BNE, BLT, BGT, MOV (some indexed variants) |
| 9 bytes | MOV (complex indexed variants) |

Maximum instruction length with all operands: **13 bytes** (theoretical worst case for future extensions)

### Fetch Buffer Strategy

To handle variable-length instructions efficiently:

1. **Large Buffer:** 256-byte instruction buffer
2. **Wide Fetch:** 128-bit (16-byte) memory reads
3. **Look-Ahead:** Pre-decode instruction length
4. **Dual-Issue:** Extract up to 2 complete instructions per cycle

**Buffer Operation:**

```
┌──────────────────────────────────────┐
│     256-Byte Circular Buffer         │
│                                      │
│  [Valid Bytes Counter: 0-256]        │
│                                      │
│  ┌────────────────────────────────┐  │
│  │  Inst 1 (2-13 bytes)           │  │ ← Extract from top
│  ├────────────────────────────────┤  │
│  │  Inst 2 (2-13 bytes)           │  │ ← Extract if dual-issue
│  ├────────────────────────────────┤  │
│  │  Remaining valid bytes         │  │
│  │  ...                           │  │
│  └────────────────────────────────┘  │
│                                      │
│  Refill from bottom when < 128 bytes │
└──────────────────────────────────────┘
```

**Extraction Process:**

1. Read specifier and opcode from buffer top
2. Determine instruction length
3. If enough bytes available, extract complete instruction
4. Try extracting second instruction for dual-issue
5. Shift consumed bytes out of buffer
6. Request memory refill if buffer < 128 bytes

## Dual-Issue Pipeline Architecture

### Pipeline Diagram

```
Cycle:     1      2      3      4      5      6
         ┌────┬────┬────┬────┬────┬────┐
Inst 0:  │ IF │ ID │ EX │ MEM│ WB │    │
         └────┴────┴────┴────┴────┴────┘
         ┌────┬────┬────┬────┬────┬────┐
Inst 1:  │ IF │ ID │ EX │ MEM│ WB │    │ (Dual-issue with Inst 0)
         └────┴────┴────┴────┴────┴────┘
         ┌────┬────┬────┬────┬────┬────┐
Inst 2:  │    │    │ IF │ ID │ EX │ MEM│
         └────┴────┴────┴────┴────┴────┘
```

### Dual-Issue Decision Logic

Two instructions can issue together IF:

✅ **Allowed Combinations:**
```
NOP     + NOP       ✓
ADD     + SUB       ✓ (different destinations, no dependencies)
MOV imm + AND       ✓ (different destinations)
```

❌ **Forbidden Combinations:**
```
MOV [mem] + MOV [mem]  ✗ (two memory ops)
B target  + any         ✗ (branch issues alone)
ADD R1, R2 + SUB R3, R1 ✗ (RAW dependency)
UMULL     + any         ✗ (multiply issues alone)
```

### Issue Rules Summary

| Rule | Description | Rationale |
|------|-------------|-----------|
| Max 1 Memory Op | At most one load/store per cycle | Single data memory port |
| No Branches | Branches issue alone | Simplifies control flow |
| No Multiplies | UMULL/SMULL issue alone | Long latency operation |
| No Structural Hazards | Can't write same register | Register file has 2 write ports |
| No RAW Between Pair | Inst 1 can't read what Inst 0 writes | No intra-cycle forwarding between pair |

### Hazard Detection Matrix

For instruction pair (I0, I1):

|Hazard Type | Check | Action |
|------------|-------|--------|
| RAW I0→I1 | I1.src == I0.dest | Single-issue I0 only |
| WAW | I0.dest == I1.dest | Single-issue I0 only |
| Memory | Both use memory | Single-issue I0 only |
| Branch | Either is branch | Single-issue branch only |
| Multiply | Either is multiply | Single-issue multiply only |

### Forwarding Paths

**6-Source Forwarding Network:**

For each operand read in ID stage:

```
   ID Stage
      │
      │  Need operand
      ↓
   ┌─────────────┐
   │ Forwarding  │
   │    MUX      │
   └─────────────┘
      ↑ ↑ ↑ ↑ ↑ ↑
      │ │ │ │ │ │
      │ │ │ │ │ └── WB Slot 1 (3 cycles ago)
      │ │ │ │ └──── WB Slot 0 (3 cycles ago)
      │ │ │ └────── MEM Slot 1 (2 cycles ago)
      │ │ └──────── MEM Slot 0 (2 cycles ago)
      │ └────────── EX Slot 1 (1 cycle ago)
      └──────────── EX Slot 0 (1 cycle ago)
```

**Priority:** Closer stages have priority (most recent write wins)

## Pipeline Hazards and Stalls

### Load-Use Hazard

**Problem:**
```
Cycle 1: MOV R1, [0x1000]  (load)  - in EX stage
Cycle 2: ADD R2, R1, R3    (use)   - in ID stage
```

Data from load not available until MEM stage, but ADD needs it in EX stage.

**Solution:** Stall for 1 cycle
```
Cycle 1: MOV R1, [0x1000]  │ EX │ MEM│ WB │
Cycle 2: ADD R2, R1, R3    │ ID │ stall │ EX │ (gets R1 via forwarding)
```

### Branch Hazards

**Branch Resolution:** EX stage

**Pipeline Flush on Taken Branch:**
```
Before:
Cycle N:   B target     │ ID │ EX │ ← Branch resolves here
Cycle N+1: Inst (wrong) │ IF │ ← Must be flushed
Cycle N+2: Inst (wrong) │    │ ← Not fetched yet

After flush:
Cycle N+2: Target inst  │ IF │
```

**Branch Penalty:** 2 cycles for taken branches

### Stall Conditions Summary

| Condition | Stall Duration | Stages Stalled |
|-----------|----------------|----------------|
| Load-use | 1 cycle | IF, ID |
| Dual-issue conflict | 1 cycle | IF, ID |
| Memory port busy | 1 cycle | IF, ID |
| Branch (taken) | 2 cycles (flush) | IF, ID |

## Performance Characteristics

### Ideal Throughput

- **Single-Issue:** 1 instruction per cycle (IPC = 1.0)
- **Dual-Issue:** Up to 2 instructions per cycle (IPC ≤ 2.0)

### Typical Performance

Assuming:
- 30% of instructions are memory ops → limit dual-issue
- 10% are branches → always single-issue
- 10% data dependencies → force single-issue
- 50% can dual-issue successfully

**Effective IPC ≈ 1.5** (50% improvement over single-issue)

### Cycle Counts

| Instruction Type | Best Case | With Hazards |
|------------------|-----------|--------------|
| ALU (register) | 1 cycle | 2-3 cycles (if dependencies) |
| ALU (immediate) | 1 cycle | 2-3 cycles |
| Load | 1 cycle | 2 cycles (load-use stall) |
| Store | 1 cycle | 1 cycle |
| Branch (not taken) | 1 cycle | 1 cycle |
| Branch (taken) | 3 cycles | 3 cycles (flush penalty) |
| Multiply | 1 cycle | 1 cycle (issues alone) |

## Memory Performance

### Bandwidth

**Instruction Fetch:**
- 128 bits (16 bytes) per cycle
- Sufficient for worst case: 2 × 9-byte instructions = 18 bytes (need 2 fetches)
- Typical: 2 × 4-byte instructions = 8 bytes (1 fetch)

**Data Access:**
- 32 bits (4 bytes) per cycle
- Single port shared between dual-issue slots
- Sequential access if both slots need memory

### Latency

All memory operations: **1 cycle latency**
- Instruction fetch: Request cycle → Data available next cycle
- Data load: Request cycle → Data available next cycle
- Data store: Write completes in 1 cycle

### Memory Conflicts

When both instructions need memory:
```
Cycle 1: Slot 0 memory access  │ MEM │
Cycle 2: Slot 1 memory access  │ MEM │ (serialized)
```

Total: 2 cycles for both operations

## FPGA Synthesis Characteristics

### Resource Usage (Estimated for ULX3S 85F)

| Resource | Usage | Notes |
|----------|-------|-------|
| Logic Cells | ~8,000 | Pipeline stages, control logic |
| BRAM | 512 Kb | 64KB unified memory |
| Multipliers | 4 | Two 16×16 multiply units |
| Registers | ~2,000 | Pipeline registers, state |

### Timing Characteristics

**Critical Paths:**
1. Forwarding MUX → ALU → Result (combinational)
2. Hazard detection logic (combinational)
3. Instruction decode logic (combinational)

**Target Frequency:** 50-100 MHz (conservative, no optimization)

### Synthesizability Features

✅ All synchronous logic
✅ Single clock domain
✅ Synchronous active-high reset
✅ Registered outputs from memory
✅ Inferable BRAM structures
✅ No latches
✅ No combinational loops

## Design Trade-offs

### Chosen Approaches

| Choice | Alternative | Rationale |
|--------|-------------|-----------|
| Von Neumann | Harvard | Simplicity, flexibility |
| Big-endian | Little-endian | Match network byte order |
| In-order | Out-of-order | Simplicity, determinism |
| Dual-issue | Single/Quad | Balance complexity vs performance |
| EX branch resolution | ID/MEM | Balance penalty vs complexity |
| 128-bit instruction fetch | 64-bit | Support long instructions |

### Performance vs Complexity

**Complexity Added for Dual-Issue:**
- Issue unit (154 lines)
- Dual forwarding paths (6 sources × 2 slots)
- Dual execution datapaths
- Second write port on register file

**Performance Gained:**
- Up to 2× throughput (1.5× typical)
- Better instruction-level parallelism

**Conclusion:** Dual-issue provides good performance gain for moderate complexity increase.

## Verification Strategy

### Unit Testing

Each module tested independently:
- ALU: All operations, flag generation
- Register file: Read/write, forwarding, dual-port
- Multiply: Signed/unsigned, edge cases
- Branch: All conditions
- Decode: All instruction formats

### Integration Testing

Full pipeline tested:
- Simple programs (NOP, HLT)
- ALU operations
- Memory operations
- Branch behavior
- Dual-issue scenarios

### Validation Against Emulator

Compare CPU behavior to C emulator:
- Same program
- Same initial state
- Final register/memory state must match

## Future Enhancements

### Short-Term

- More comprehensive test programs
- Branch prediction (static/dynamic)
- Performance counters
- Debug interface (JTAG)

### Long-Term

- Cache hierarchy (L1 I$ and D$)
- Deeper pipeline (7-8 stages for higher frequency)
- More aggressive dual-issue (3-4 wide superscalar)
- Out-of-order execution
- Vector extensions

## References

- **Machine Description File:** Complete ISA specification
- **C Emulator:** Reference implementation (../src/emulator.c)
- **MODULE_DOCUMENTATION.md:** Detailed module documentation
- **README.md:** Quick reference and overview

