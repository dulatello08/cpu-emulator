# NeoCore16x32 Instruction Set Architecture Reference

## Table of Contents

1. [Instruction Set Overview](#instruction-set-overview)
2. [Instruction Encoding](#instruction-encoding)
3. [Arithmetic and Logic Instructions](#arithmetic-and-logic-instructions)
4. [Data Movement Instructions](#data-movement-instructions)
5. [Branch and Control Flow Instructions](#branch-and-control-flow-instructions)
6. [Multiply Instructions](#multiply-instructions)
7. [Stack Operations](#stack-operations)
8. [Subroutine Instructions](#subroutine-instructions)
9. [System Control Instructions](#system-control-instructions)
10. [Instruction Summary Table](#instruction-summary-table)

---

## Instruction Set Overview

The NeoCore16x32 instruction set consists of **26 opcodes** organized into 7 functional categories. All instructions use **big-endian byte ordering** and have **variable lengths** from 2 to 13 bytes.

### Instruction Categories

| Category | Opcodes | Count | Description |
|----------|---------|-------|-------------|
| **ALU** | 0x01-0x08 | 8 | Arithmetic and logic operations |
| **MOV** | 0x09 | 1 (19 modes) | Data movement with multiple addressing modes |
| **Branch** | 0x0A-0x0F | 6 | Control flow and conditional branches |
| **Multiply** | 0x10-0x11 | 2 | 32-bit multiply operations |
| **Stack** | 0x13-0x14 | 2 | Push and pop operations |
| **Subroutine** | 0x15-0x16 | 2 | Call and return |
| **Control** | 0x00, 0x12, 0x17-0x19 | 5 | NOP, halt, and interrupt control |

### Common Instruction Format

All instructions follow this structure:

```
Byte 0:    Specifier (addressing mode/variant)
Byte 1:    Opcode
Bytes 2+:  Operands (registers, immediates, addresses)
```

Big-endian ordering means:
- Multi-byte immediates have MSB at lower address
- Addresses are stored MSB-first
- First byte at lowest memory address

---

## Instruction Encoding

### Register Encoding

Registers are encoded in 4-bit fields (bits 3-0 of the byte):

| Bits 3-0 | Register |
|----------|----------|
| 0x0 | R0 |
| 0x1 | R1 |
| 0x2 | R2 |
| ... | ... |
| 0xF | R15 |

### Specifier Field

The specifier (Byte 0) determines:
1. Addressing mode for the instruction
2. Which operands are used
3. Total instruction length

Different opcodes interpret specifiers differently. For example:
- ALU instructions use 3 specifiers: immediate, register, memory
- MOV uses 19 specifiers for different addressing modes
- Branches typically use specifier 0x00

### Opcode Field

The opcode (Byte 1) identifies the operation:

| Opcode | Mnemonic | Type |
|--------|----------|------|
| 0x00 | NOP | Control |
| 0x01 | ADD | ALU |
| 0x02 | SUB | ALU |
| 0x03 | MUL | ALU |
| 0x04 | AND | ALU |
| 0x05 | OR | ALU |
| 0x06 | XOR | ALU |
| 0x07 | LSH | ALU |
| 0x08 | RSH | ALU |
| 0x09 | MOV | Data Movement |
| 0x0A | B | Branch |
| 0x0B | BE | Branch |
| 0x0C | BNE | Branch |
| 0x0D | BLT | Branch |
| 0x0E | BGT | Branch |
| 0x0F | BRO | Branch |
| 0x10 | UMULL | Multiply |
| 0x11 | SMULL | Multiply |
| 0x12 | HLT | Control |
| 0x13 | PSH | Stack |
| 0x14 | POP | Stack |
| 0x15 | JSR | Subroutine |
| 0x16 | RTS | Subroutine |
| 0x17 | WFI | Control |
| 0x18 | ENI | Control |
| 0x19 | DSI | Control |

---

## Arithmetic and Logic Instructions

These instructions perform operations on 16-bit register values. The result is truncated to 16 bits and stored in the destination register.

### ADD - Addition

**Operation**: `rd = rd + operand`  
**Opcode**: 0x01  
**Flags**: Updates Z (zero) and V (overflow)

#### Variants:

**ADD Immediate Mode** (Specifier 0x00)
```
Length: 5 bytes
Encoding:
  [0x00] [0x01] [rd] [imm_high] [imm_low]

Example: ADD R5, #0x1234
  0x00, 0x01, 0x05, 0x12, 0x34

Operation: R5 = R5 + 0x1234
```

**ADD Register Mode** (Specifier 0x01)
```
Length: 4 bytes
Encoding:
  [0x01] [0x01] [rd] [rn]

Example: ADD R1, R2
  0x01, 0x01, 0x01, 0x02

Operation: R1 = R2 + R1  (Note: operand order)
```

**ADD Memory Mode** (Specifier 0x02)
```
Length: 7 bytes
Encoding:
  [0x02] [0x01] [rd] [addr3] [addr2] [addr1] [addr0]

Example: ADD R3, [0x12345678]
  0x02, 0x01, 0x03, 0x12, 0x34, 0x56, 0x78

Operation: R3 = R3 + mem16[0x12345678]
```

**Notes**:
- In register mode, rn is added to rd
- Memory mode loads a halfword (16-bit) value from the address
- Overflow occurs if result > 0xFFFF (sets V flag)

---

### SUB - Subtraction

**Operation**: `rd = rd - operand`  
**Opcode**: 0x02  
**Flags**: Updates Z and V

**Important**: The RTL implements clamping behavior - if operand > rd, result is 0 (not negative wrap).

#### Variants:

**SUB Immediate Mode** (Specifier 0x00)
```
Length: 5 bytes
Encoding: [0x00] [0x02] [rd] [imm_h] [imm_l]
Operation: rd = (rd >= imm) ? rd - imm : 0
```

**SUB Register Mode** (Specifier 0x01)
```
Length: 4 bytes
Encoding: [0x01] [0x02] [rd] [rn]
Operation: rd = (rd >= rn) ? rd - rn : 0
```

**SUB Memory Mode** (Specifier 0x02)
```
Length: 7 bytes
Encoding: [0x02] [0x02] [rd] [a3] [a2] [a1] [a0]
Operation: rd = (rd >= mem[addr]) ? rd - mem[addr] : 0
```

---

### MUL - Multiply (Truncated)

**Operation**: `rd = (rd * operand) & 0xFFFF` (lower 16 bits only)  
**Opcode**: 0x03  
**Flags**: Updates Z and V (V set if result > 0xFFFF before truncation)

**Warning**: This is a **truncated multiply**. For full 32-bit results, use UMULL or SMULL.

#### Variants:

**MUL Immediate Mode** (Specifier 0x00)
```
Length: 5 bytes
Operation: rd = (rd * immediate) & 0xFFFF
```

**MUL Register Mode** (Specifier 0x01)
```
Length: 4 bytes
Operation: rd = (rd * rn) & 0xFFFF
```

**MUL Memory Mode** (Specifier 0x02)
```
Length: 7 bytes
Operation: rd = (rd * mem[addr]) & 0xFFFF
```

**Example**:
```
R1 = 0x0100
R2 = 0x0200
MUL R1, R2      ; R1 = (0x0100 * 0x0200) & 0xFFFF = 0x20000 & 0xFFFF = 0x0000
                ; V flag SET (overflow occurred)
```

---

### AND - Bitwise AND

**Operation**: `rd = rd & operand`  
**Opcode**: 0x04  
**Flags**: Updates Z and V (V is cleared for logic operations)

#### Variants:

All three specifiers (0x00, 0x01, 0x02) same as ADD.

**Example**:
```
R1 = 0xFF00
AND R1, #0x00FF     ; R1 = 0x0000, Z flag SET
```

---

### OR - Bitwise OR

**Operation**: `rd = rd | operand`  
**Opcode**: 0x05  
**Flags**: Updates Z and V

#### Variants:

Same as AND (immediate, register, memory modes).

---

### XOR - Bitwise XOR

**Operation**: `rd = rd ^ operand`  
**Opcode**: 0x06  
**Flags**: Updates Z and V

**Example**:
```
R1 = 0xAAAA
R2 = 0xAAAA
XOR R1, R2      ; R1 = 0x0000, Z flag SET
```

---

### LSH - Left Shift

**Operation**: `rd = rd << (operand & 0x1F)` (shift amount masked to 5 bits)  
**Opcode**: 0x07  
**Flags**: Updates Z and V

**Notes**:
- Shift amount is limited to 0-31 (5-bit mask)
- Shift fills with zeros from right
- Bits shifted out are lost

#### Variants:

**LSH Immediate**:
```
Example: LSH R1, #4
  R1 = 0x0123
  Result: R1 = 0x1230
```

**LSH Register**:
```
Example: LSH R1, R2
  R1 = 0x0001, R2 = 0x0008
  Result: R1 = 0x0100
```

---

### RSH - Right Shift (Logical)

**Operation**: `rd = rd >> (operand & 0x1F)`  
**Opcode**: 0x08  
**Flags**: Updates Z and V

**Notes**:
- Logical shift (fills with zeros from left)
- NOT arithmetic shift (no sign extension)
- Shift amount masked to 5 bits

**Example**:
```
R1 = 0x8000
RSH R1, #1      ; R1 = 0x4000 (not 0xC000)
```

---

## Data Movement Instructions

The MOV instruction is the workhorse for data movement with **19 different specifiers** supporting diverse addressing modes.

### MOV - Move Data

**Opcode**: 0x09

#### MOV Specifier 0x00: Immediate to Register

```
Length: 5 bytes
Encoding: [0x00] [0x09] [rd] [imm_h] [imm_l]
Operation: rd = immediate
Flags: None

Example: MOV R1, #0x1234
  0x00, 0x09, 0x01, 0x12, 0x34
  R1 = 0x1234
```

#### MOV Specifier 0x01: 32-bit Immediate to Register Pair

```
Length: 8 bytes
Encoding: [0x01] [0x09] [rd] [rn] [a3] [a2] [a1] [a0]
Operation: rd = addr[31:16], rn = addr[15:0]
Flags: None

Example: MOV R1, R2, #0xDEADBEEF
  0x01, 0x09, 0x01, 0x02, 0xDE, 0xAD, 0xBE, 0xEF
  R1 = 0xDEAD (upper 16 bits)
  R2 = 0xBEEF (lower 16 bits)
```

**Note**: Used to load 32-bit addresses or constants into two registers.

#### MOV Specifier 0x02: Register to Register

```
Length: 4 bytes
Encoding: [0x02] [0x09] [rd] [rn]
Operation: rd = rn (copy rn to rd)
Flags: None

Example: MOV R1, R2
  0x02, 0x09, 0x01, 0x02
  R1 = R2
```

**Warning**: Syntax is "MOV rd, rn" but operation is rd = rn (reversed from some conventions).

#### MOV Specifier 0x03: Load Byte to Low Register

```
Length: 7 bytes
Encoding: [0x03] [0x09] [rd] [a3] [a2] [a1] [a0]
Operation: rd[7:0] = mem8[addr], rd[15:8] unchanged
Flags: None

Example: MOV R1.L, [0x12345678]
  Loads byte from address 0x12345678 into R1 bits 7-0
  R1 bits 15-8 are NOT modified
```

#### MOV Specifier 0x04: Load Byte to High Register

```
Length: 7 bytes
Encoding: [0x04] [0x09] [rd] [a3] [a2] [a1] [a0]
Operation: rd[15:8] = mem8[addr], rd[7:0] unchanged
Flags: None

Example: MOV R1.H, [0x12345678]
  Loads byte into R1 bits 15-8
```

#### MOV Specifier 0x05: Load Halfword

```
Length: 7 bytes
Encoding: [0x05] [0x09] [rd] [a3] [a2] [a1] [a0]
Operation: rd = mem16[addr]
Flags: None

Example: MOV R1, [0x12345678]
  R1 = {mem[0x12345678], mem[0x12345679]}  (big-endian)
```

#### MOV Specifier 0x06: Load Word to Register Pair

```
Length: 8 bytes
Encoding: [0x06] [0x09] [rd] [rn1] [a3] [a2] [a1] [a0]
Operation: rd = mem32[addr][31:16], rn1 = mem32[addr][15:0]
Flags: None

Example: MOV R1, R2, [0x12345678]
  32-bit word loaded:
    R1 = upper 16 bits
    R2 = lower 16 bits
```

#### MOV Specifier 0x07: Store Byte from Low Register

```
Length: 7 bytes
Encoding: [0x07] [0x09] [rd] [a3] [a2] [a1] [a0]
Operation: mem8[addr] = rd[7:0]
Flags: None

Example: MOV [0x12345678], R1.L
  Stores R1 bits 7-0 to address 0x12345678
```

#### MOV Specifier 0x08: Store Byte from High Register

```
Length: 7 bytes
Encoding: [0x08] [0x09] [rd] [a3] [a2] [a1] [a0]
Operation: mem8[addr] = rd[15:8]
Flags: None

Example: MOV [0x12345678], R1.H
  Stores R1 bits 15-8 to address 0x12345678
```

#### MOV Specifier 0x09: Store Halfword

```
Length: 7 bytes
Encoding: [0x09] [0x09] [rd] [a3] [a2] [a1] [a0]
Operation: mem16[addr] = rd
Flags: None

Example: MOV [0x12345678], R1
  mem[0x12345678] = R1[15:8]  (MSB)
  mem[0x12345679] = R1[7:0]   (LSB)
```

#### MOV Specifier 0x0A: Store Word from Register Pair

```
Length: 8 bytes
Encoding: [0x0A] [0x09] [rd] [rn1] [a3] [a2] [a1] [a0]
Operation: mem32[addr] = {rd, rn1}
Flags: None

Example: MOV [0x12345678], R1, R2
  mem[0x12345678+0] = R1[15:8]
  mem[0x12345678+1] = R1[7:0]
  mem[0x12345678+2] = R2[15:8]
  mem[0x12345678+3] = R2[7:0]
```

#### MOV Specifier 0x0B: Load Byte to Low with Offset

```
Length: 8 bytes
Encoding: [0x0B] [0x09] [rd] [rn] [o3] [o2] [o1] [o0]
Operation: rd[7:0] = mem8[rn + offset]
Flags: None

Example: MOV R1.L, [R3 + 0x100]
  Effective address = R3 + 0x00000100
  Loads byte to R1 low byte
```

**Note**: Offset is 32-bit unsigned, added to 16-bit register value (zero-extended).

#### MOV Specifier 0x0C: Load Byte to High with Offset

```
Length: 8 bytes
Operation: rd[15:8] = mem8[rn + offset]
```

#### MOV Specifier 0x0D: Load Halfword with Offset

```
Length: 8 bytes
Encoding: [0x0D] [0x09] [rd] [rn] [o3] [o2] [o1] [o0]
Operation: rd = mem16[rn + offset]

Example: MOV R1, [R3 + 0x20]
  Effective address = R3 + 0x20
  R1 = mem16[R3 + 0x20]
```

#### MOV Specifier 0x0E: Load Word with Offset to Register Pair

```
Length: 9 bytes
Encoding: [0x0E] [0x09] [rd] [rd1] [rn] [o3] [o2] [o1] [o0]
Operation: {rd, rd1} = mem32[rn + offset]

Example: MOV R1, R2, [R3 + 0x30]
  Effective address = R3 + 0x30
  R1 = mem32[addr][31:16]
  R2 = mem32[addr][15:0]
```

#### MOV Specifier 0x0F: Store Byte from Low with Offset

```
Length: 8 bytes
Encoding: [0x0F] [0x09] [rd] [rn] [o3] [o2] [o1] [o0]
Operation: mem8[rn + offset] = rd[7:0]
```

#### MOV Specifier 0x10: Store Byte from High with Offset

```
Length: 8 bytes
Operation: mem8[rn + offset] = rd[15:8]
```

#### MOV Specifier 0x11: Store Halfword with Offset

```
Length: 8 bytes
Encoding: [0x11] [0x09] [rd] [rn] [o3] [o2] [o1] [o0]
Operation: mem16[rn + offset] = rd

Example: MOV [R3 + 0x50], R1
  mem16[R3 + 0x50] = R1
```

#### MOV Specifier 0x12: Store Word with Offset from Register Pair

```
Length: 9 bytes
Encoding: [0x12] [0x09] [rd] [rd1] [rn] [o3] [o2] [o1] [o0]
Operation: mem32[rn + offset] = {rd, rd1}

Example: MOV [R3 + 0x60], R1, R2
  mem32[R3 + 0x60] = {R1, R2}
```

---

## Branch and Control Flow Instructions

Branch instructions modify the Program Counter (PC) based on conditions.

### B - Unconditional Branch

**Operation**: `PC = target`  
**Opcode**: 0x0A  
**Specifier**: 0x00  
**Length**: 6 bytes  
**Flags**: None

```
Encoding: [0x00] [0x0A] [t3] [t2] [t1] [t0]

Example: B 0x00001000
  0x00, 0x0A, 0x00, 0x00, 0x10, 0x00
  PC = 0x00001000
```

**Branch Behavior**:
- Takes 2 cycles (flushes IF and ID stages)
- Always issues alone (no dual-issue with branches)

---

### BE - Branch if Equal

**Operation**: `if (rd == rn) PC = target`  
**Opcode**: 0x0B  
**Length**: 8 bytes  
**Flags**: None (uses register comparison)

```
Encoding: [0x00] [0x0B] [rd] [rn] [t3] [t2] [t1] [t0]

Example: BE R1, R2, 0x00002000
  If R1 == R2, branch to 0x00002000
  Otherwise, continue to next instruction
```

---

### BNE - Branch if Not Equal

**Operation**: `if (rd != rn) PC = target`  
**Opcode**: 0x0C  
**Length**: 8 bytes

```
Example: BNE R1, R2, 0x00002000
  If R1 != R2, branch to 0x00002000
```

---

### BLT - Branch if Less Than

**Operation**: `if (rd < rn) PC = target` (unsigned comparison)  
**Opcode**: 0x0D  
**Length**: 8 bytes

```
Example: BLT R1, R2, 0x00003000
  If R1 < R2 (unsigned), branch to 0x00003000
```

**Note**: This is **unsigned comparison**. `0xFFFF < 0x0001` is FALSE.

---

### BGT - Branch if Greater Than

**Operation**: `if (rd > rn) PC = target` (unsigned comparison)  
**Opcode**: 0x0E  
**Length**: 8 bytes

```
Example: BGT R1, R2, 0x00003000
  If R1 > R2 (unsigned), branch to 0x00003000
```

---

### BRO - Branch if Overflow

**Operation**: `if (V_flag == 1) PC = target`  
**Opcode**: 0x0F  
**Length**: 6 bytes

```
Encoding: [0x00] [0x0F] [t3] [t2] [t1] [t0]

Example: BRO 0x00004000
  If overflow flag is set, branch to 0x00004000
```

**Use Case**:
```
ADD R1, #0x8000     ; May overflow
BRO error_handler   ; Branch if it did
```

---

## Multiply Instructions

Full 32-bit multiply operations that produce results in two registers.

### UMULL - Unsigned Multiply Long

**Operation**: `{rd, rn1} = rd * rn` (unsigned 16x16 → 32)  
**Opcode**: 0x10  
**Length**: 4 bytes  
**Flags**: Updates Z and V based on 32-bit result

```
Encoding: [0x00] [0x10] [rd] [rn] [rn1]

Example: UMULL R1, R2, R3
  32-bit product = R2 * R3 (unsigned)
  R1 = product[15:0]  (lower 16 bits)
  R2 = product[31:16] (upper 16 bits)

Concrete:
  R2 = 0x0100
  R3 = 0x0200
  UMULL R1, R2, R3
  Product = 0x00020000
  R1 = 0x0000
  R2 = 0x0002
```

**Notes**:
- UMULL cannot dual-issue
- Writes two destination registers (rd and rn1)
- Result is full 32-bit product, no truncation

---

### SMULL - Signed Multiply Long

**Operation**: `{rd, rn1} = signed(rd) * signed(rn)` (signed 16x16 → 32)  
**Opcode**: 0x11  
**Length**: 4 bytes  
**Flags**: Updates Z and V

```
Encoding: [0x00] [0x11] [rd] [rn] [rn1]

Example: SMULL R1, R2, R3
  Treats R2 and R3 as signed 16-bit values
  32-bit signed product → {R1 (low), R2 (high)}

Concrete:
  R2 = 0xFFFF  (interpreted as -1)
  R3 = 0x0002  (interpreted as +2)
  SMULL R1, R2, R3
  Product = -2 = 0xFFFFFFFE
  R1 = 0xFFFE
  R2 = 0xFFFF
```

---

## Stack Operations

Stack operations use R14 as the stack pointer (by convention, not enforced by hardware).

### PSH - Push

**Operation**: `mem16[R14] = rd; R14 = R14 - 2`  
**Opcode**: 0x13  
**Length**: 3 bytes  
**Flags**: None

```
Encoding: [0x00] [0x13] [rd]

Example: PSH R1
  mem16[R14] = R1
  R14 = R14 - 2  (stack grows downward)
```

**Note**: RTL assumes R14 is stack pointer but doesn't enforce it. Any register could be used if software manages it.

---

### POP - Pop

**Operation**: `R14 = R14 + 2; rd = mem16[R14]`  
**Opcode**: 0x14  
**Length**: 3 bytes  
**Flags**: None

```
Encoding: [0x00] [0x14] [rd]

Example: POP R1
  R14 = R14 + 2
  R1 = mem16[R14]
```

**Stack Usage Example**:
```
; Save registers
PSH R1
PSH R2

; ... use R1, R2 ...

; Restore registers
POP R2
POP R1
```

---

## Subroutine Instructions

### JSR - Jump to Subroutine

**Operation**: `PSH(PC+6); PC = target`  
**Opcode**: 0x15  
**Length**: 6 bytes  
**Flags**: None

```
Encoding: [0x00] [0x15] [t3] [t2] [t1] [t0]

Example: JSR 0x00005000
  Pushes return address (PC+6) onto stack
  Jumps to 0x00005000
```

**Note**: Return address points to the instruction AFTER JSR (6 bytes ahead).

---

### RTS - Return from Subroutine

**Operation**: `PC = POP()`  
**Opcode**: 0x16  
**Length**: 2 bytes  
**Flags**: None

```
Encoding: [0x00] [0x16]

Example: RTS
  Pops return address from stack
  Jumps to that address
```

**Subroutine Example**:
```
main:
  MOV R1, #5
  JSR function     ; Call function
  ; execution continues here after RTS
  HLT

function:
  ADD R1, #10      ; R1 = 15
  RTS              ; Return to caller
```

---

## System Control Instructions

### NOP - No Operation

**Operation**: None (pipeline advances)  
**Opcode**: 0x00  
**Length**: 2 bytes  
**Flags**: None

```
Encoding: [0x00] [0x00]

Example: NOP
  Does nothing for one cycle
```

---

### HLT - Halt

**Operation**: Stop execution  
**Opcode**: 0x12  
**Length**: 2 bytes  
**Flags**: None

```
Encoding: [0x00] [0x12]

Example: HLT
  Sets halted signal, stopping pipeline
```

**Note**: CPU remains halted until reset.

---

### WFI - Wait For Interrupt

**Operation**: Wait for interrupt (currently NOP)  
**Opcode**: 0x17  
**Length**: 2 bytes  
**Flags**: None

**Note**: Placeholder for future interrupt support. Currently behaves as NOP.

---

### ENI - Enable Interrupts

**Operation**: Enable interrupt handling (currently NOP)  
**Opcode**: 0x18  
**Length**: 2 bytes

**Note**: Placeholder for future interrupt support.

---

### DSI - Disable Interrupts

**Operation**: Disable interrupt handling (currently NOP)  
**Opcode**: 0x19  
**Length**: 2 bytes

**Note**: Placeholder for future interrupt support.

---

## Instruction Summary Table

| Opcode | Mnemonic | Specifiers | Length Range | Flags | Description |
|--------|----------|------------|--------------|-------|-------------|
| 0x00 | NOP | 0x00 | 2 | - | No operation |
| 0x01 | ADD | 0x00-0x02 | 4-7 | Z,V | Addition |
| 0x02 | SUB | 0x00-0x02 | 4-7 | Z,V | Subtraction (clamped) |
| 0x03 | MUL | 0x00-0x02 | 4-7 | Z,V | Multiply (truncated) |
| 0x04 | AND | 0x00-0x02 | 4-7 | Z,V | Bitwise AND |
| 0x05 | OR | 0x00-0x02 | 4-7 | Z,V | Bitwise OR |
| 0x06 | XOR | 0x00-0x02 | 4-7 | Z,V | Bitwise XOR |
| 0x07 | LSH | 0x00-0x02 | 4-7 | Z,V | Left shift |
| 0x08 | RSH | 0x00-0x02 | 4-7 | Z,V | Right shift (logical) |
| 0x09 | MOV | 0x00-0x12 | 4-9 | - | Data movement |
| 0x0A | B | 0x00 | 6 | - | Unconditional branch |
| 0x0B | BE | 0x00 | 8 | - | Branch if equal |
| 0x0C | BNE | 0x00 | 8 | - | Branch if not equal |
| 0x0D | BLT | 0x00 | 8 | - | Branch if less than |
| 0x0E | BGT | 0x00 | 8 | - | Branch if greater than |
| 0x0F | BRO | 0x00 | 6 | - | Branch if overflow |
| 0x10 | UMULL | 0x00 | 4 | Z,V | Unsigned multiply long |
| 0x11 | SMULL | 0x00 | 4 | Z,V | Signed multiply long |
| 0x12 | HLT | 0x00 | 2 | - | Halt execution |
| 0x13 | PSH | 0x00 | 3 | - | Push to stack |
| 0x14 | POP | 0x00 | 3 | - | Pop from stack |
| 0x15 | JSR | 0x00 | 6 | - | Jump to subroutine |
| 0x16 | RTS | 0x00 | 2 | - | Return from subroutine |
| 0x17 | WFI | 0x00 | 2 | - | Wait for interrupt (placeholder) |
| 0x18 | ENI | 0x00 | 2 | - | Enable interrupts (placeholder) |
| 0x19 | DSI | 0x00 | 2 | - | Disable interrupts (placeholder) |

---

## Big-Endian Encoding Examples

### Example 1: Simple ADD Immediate

```
Assembly: ADD R5, #0x1234

Binary Layout in Memory (address 0x1000):
  0x1000: 0x00  (Specifier: immediate mode)
  0x1001: 0x01  (Opcode: ADD)
  0x1002: 0x05  (rd = R5)
  0x1003: 0x12  (Immediate MSB)
  0x1004: 0x34  (Immediate LSB)

Length: 5 bytes
Next instruction at: 0x1005
```

### Example 2: Conditional Branch

```
Assembly: BNE R1, R2, 0xDEADBEEF

Binary Layout in Memory (address 0x2000):
  0x2000: 0x00  (Specifier)
  0x2001: 0x0C  (Opcode: BNE)
  0x2002: 0x01  (rd = R1)
  0x2003: 0x02  (rn = R2)
  0x2004: 0xDE  (Target address MSB)
  0x2005: 0xAD
  0x2006: 0xBE
  0x2007: 0xEF  (Target address LSB)

Length: 8 bytes
Next instruction at: 0x2008
```

### Example 3: MOV with Offset

```
Assembly: MOV R1, [R3 + 0x00000100]

Binary Layout in Memory (address 0x3000):
  0x3000: 0x0D  (Specifier: load halfword with offset)
  0x3001: 0x09  (Opcode: MOV)
  0x3002: 0x01  (rd = R1)
  0x3003: 0x03  (rn = R3, base register)
  0x3004: 0x00  (Offset MSB)
  0x3005: 0x00
  0x3006: 0x01
  0x3007: 0x00  (Offset LSB = 0x100)

Length: 8 bytes
Operation: R1 = mem16[R3 + 0x100]
```

---

## Instruction Timing

All instructions execute through the 5-stage pipeline:
- **Minimum latency**: 5 cycles (IF, ID, EX, MEM, WB)
- **Throughput**: 1-2 instructions/cycle (with dual-issue)
- **Branch penalty**: 2 cycles (flush IF, ID)
- **Load-use hazard**: 1 cycle stall

**Best-case scenario** (dual-issue throughout):
```
Cycle 1: Fetch inst 0 and 1
Cycle 2: Decode inst 0 and 1, Fetch inst 2 and 3
Cycle 3: Execute inst 0 and 1, Decode inst 2 and 3, Fetch inst 4 and 5
...
Result: 2 instructions per cycle = CPI of 0.5
```

**Typical scenario** (mix of single and dual-issue):
```
CPI ≈ 0.67 - 0.83 (1.2 - 1.5 IPC)
```

---

This ISA reference is derived entirely from the RTL implementation in `decode_unit.sv`, `neocore_pkg.sv`, and the verified behavior in testbenches. All encodings, lengths, and behaviors match the actual hardware.

