### Instruction: nop
**Opcode:** 0x00

**General Description:** No operation.

---

### Instruction: add
**Opcode:** 0x01

**General Description:** Add two 16-bit values.

**Specifiers:**
- **00**: mode 00, add immediate 16-bit value (operand 2) to rd. Leave rn empty. 3-word length.
- **01**: mode 01, add value from rn to rd. 2-word length.
- **02**: mode 02, add value at address of normAddressing to rd. 3-word length.

**Operands:**
- **rd** (8-bit): In specifiers 00, 01, 02, it is the destination for the add operation and the second addend.
- **rn** (8-bit): In specifier 00 not used, in 01 is the source addend, in 02 not used.
- **operand 2** (16-bit): In specifier 00, it is the immediate value to be added to rd.
- **normAddressing** (32-bit): In specifier 02, it is the address from which the value is read and added to rd.

---

### Instruction: sub
**Opcode:** 0x02

**General Description:** Subtract two 16-bit values.

**Specifiers:**
- **00**: mode 00, subtract immediate 16-bit value (operand 2) from rd. Leave rn empty. 3-word length.
- **01**: mode 01, subtract value from rn from rd. 2-word length.
- **02**: mode 02, subtract value at address of normAddressing from rd. 3-word length.

**Operands:**
- **rd** (8-bit): In specifiers 00, 01, 02, it is the destination for the subtract operation and the first operand.
- **rn** (8-bit): In specifier 00 not used, in 01 is the source subtrahend, in 02 not used.
- **operand 2** (16-bit): In specifier 00, it is the immediate value to be subtracted from rd.
- **normAddressing** (32-bit): In specifier 02, it is the address from which the value is read and subtracted from rd.

---

### Instruction: mul
**Opcode:** 0x03

**General Description:** Multiply two 16-bit values. Warning: output truncated 16 bit number.

**Specifiers:**
- **00**: mode 00, multiply rd by immediate 16-bit value (operand 2). Leave rn empty. 3-word length.
- **01**: mode 01, multiply rd by value from rn. 2-word length.
- **02**: mode 02, multiply rd by value at address of normAddressing. 3-word length.

**Operands:**
- **rd** (8-bit): In specifiers 00, 01, 02, it is the destination for the multiply operation and the multiplicand.
- **rn** (8-bit): In specifier 00 not used, in 01 is the source multiplier, in 02 not used. in 03, is both a multiplier and product.
- **operand 2** (16-bit): In specifier 00, it is the immediate value to multiply with rd.
- **normAddressing** (32-bit): In specifier 02, it is the address from which the value is read and multiplied with rd.

---

### Instruction: and
**Opcode:** 0x04

**General Description:** Bitwise AND of two 16-bit values.

**Specifiers:**
- **00**: mode 00, bitwise AND rd with immediate 16-bit value (operand 2). Leave rn empty. 3-word length.
- **01**: mode 01, bitwise AND rd with value from rn. 2-word length.
- **02**: mode 02, bitwise AND rd with value at address of normAddressing. 3-word length.

**Operands:**
- **rd** (8-bit): In specifiers 00, 01, 02, it is the destination for the AND operation and one of the operands.
- **rn** (8-bit): In specifier 00 not used, in 01 is the second operand, in 02 not used.
- **operand 2** (16-bit): In specifier 00, it is the immediate value to AND with rd.
- **normAddressing** (32-bit): In specifier 02, it is the address from which the value is read and ANDed with rd.

---

### Instruction: or
**Opcode:** 0x05

**General Description:** Bitwise OR of two 16-bit values.

**Specifiers:**
- **00**: mode 00, bitwise OR rd with immediate 16-bit value (operand 2). Leave rn empty. 3-word length.
- **01**: mode 01, bitwise OR rd with value from rn. 2-word length.
- **02**: mode 02, bitwise OR rd with value at address of normAddressing. 3-word length.

**Operands:**
- **rd** (8-bit): In specifiers 00, 01, 02, it is the destination for the OR operation and one of the operands.
- **rn** (8-bit): In specifier 00 not used, in 01 is the second operand, in 02 not used.
- **operand 2** (16-bit): In specifier 00, it is the immediate value to OR with rd.
- **normAddressing** (32-bit): In specifier 02, it is the address from which the value is read and ORed with rd.

---

### Instruction: xor
**Opcode:** 0x06

**General Description:** Bitwise XOR of two 16-bit values.

**Specifiers:**
- **00**: mode 00, bitwise XOR rd with immediate 16-bit value (operand 2). Leave rn empty. 3-word length.
- **01**: mode 01, bitwise XOR rd with value from rn. 2-word length.
- **02**: mode 02, bitwise XOR rd with value at address of normAddressing. 3-word length.

**Operands:**
- **rd** (8-bit): In specifiers 00, 01, 02, it is the destination for the XOR operation and one of the operands.
- **rn** (8-bit): In specifier 00 not used, in 01 is the second operand, in 02 not used.
- **operand 2** (16-bit): In specifier 00, it is the immediate value to XOR with rd.
- **normAddressing** (32-bit): In specifier 02, it is the address from which the value is read and XORed with rd.

---

### Instruction: lsh
**Opcode:** 0x07

**General Description:** Left shift a 16-bit value.

**Specifiers:**
- **00**: mode 00, left shift rd by immediate 16-bit value (operand 2). Leave rn empty. 3-word length.
- **01**: mode 01, left shift rd by value from rn. 2-word length.
- **02**: mode 02, left shift rd by value at address of normAddressing. 3-word length.

**Operands:**
- **rd** (8-bit): In specifiers 00, 01, 02, it is the destination for the left shift operation and the value to be shifted.
- **rn** (8-bit): In specifier 00 not used, in 01 is the source of the shift amount, in 02 not used.
- **operand 2** (16-bit): In specifier 00, it is the immediate value for the shift amount.
- **normAddressing** (32-bit): In specifier 02, it is the address from which the value is read and used as the shift amount.

---

### Instruction: rsh
**Opcode:** 0x08

**General Description:** Right shift a 16-bit value.

**Specifiers:**
- **00**: mode 00, right shift rd by immediate 16-bit value (operand 2). Leave rn empty. 3-word length.
- **01**: mode 01, right shift rd by value from rn. 2-word length.
- **02**: mode 02, right shift rd by value at address of normAddressing. 3-word length.

**Operands:**
- **rd** (8-bit): In specifiers 00, 01, 02, it is the destination for the right shift operation and the value to be shifted.
- **rn** (8-bit): In specifier 00 not used, in 01 is the source of the shift amount, in 02 not used.
- **operand 2** (16-bit): In specifier 00, it is the immediate value for the shift amount.
- **normAddressing** (32-bit): In specifier 02, it is the address from which the value is read and used as the shift amount.

---

### Instruction: mov
**Opcode:** 0x09

**General Description:** Move data between registers and memory with various widths and addressing modes, as determined by the specifier.

**Specifiers:**
- **0x00**: Move immediate into rd(any width, 16 or 8). Syntax: `mov 1, #0x1234`
- **0x01**: Move 32 bit immediate into rd and rn. Syntax: `mov 1, 2 #0xDEADBEEF` or `mov 1, 2 label`
- **0x02**: Move (copy) rd to rn (full width, 16). Syntax: `mov 1, 2`. Warning: reversed semantics.
- **0x03**: Move 8-bit value from memory (normAddressing) to rd.L. Syntax: `mov 1.L, [0x2000]`
- **0x04**: Move 8-bit value from memory (normAddressing) to rd.H. Syntax: `mov 1.H, [0x2001]`
- **0x05**: Move 16-bit value from memory (normAddressing) to rd. Syntax: `mov 1, [0x3000]`
- **0x06**: Move 32-bit value from memory (normAddressing) into rd (upper 16 bits) and rn1 (lower 16 bits). Syntax: `mov 1, 2, [0x4000]`
- **0x07**: Move 8-bit value from rd.L to memory (normAddressing). Syntax: `mov [0x5000], 1.L`
- **0x08**: Move 8-bit value from rd.H to memory (normAddressing). Syntax: `mov [0x5001], 1.H`
- **0x09**: Move 16-bit value from rd to memory (normAddressing). Syntax: `mov [0x6000], 1`
- **0x0A**: Move 32-bit value from rd (upper 16 bits) and rn1 (lower 16 bits) to memory (normAddressing). Syntax: `mov [0x7000], 1, 2`
- **0x0B**: Move 8-bit value from memory (rn + Offset) to rd.L. Syntax: `mov 1.L, [3 + 0x10]`
- **0x0C**: Move 8-bit value from memory (rn + Offset) to rd.H. Syntax: `mov 1.H, [3 + 0x10]`
- **0x0D**: Move 16-bit value from memory (rn + Offset) to rd. Syntax: `mov 1, [3 + 0x20]`
- **0x0E**: Move 32-bit value from memory (rn + Offset) to rd and rn1. Syntax: `mov 1, 2, [3 + 0x30]`
- **0x0F**: Move 8-bit value from rd.L to memory (rn + Offset). Syntax: `mov [3 + 0x40], 1.L`
- **0x10**: Move 8-bit value from rd.H to memory (rn + Offset). Syntax: `mov [3 + 0x40], 1.H`
- **0x11**: Move 16-bit value from rd to memory (rn + Offset). Syntax: `mov [3 + 0x50], 1`
- **0x12**: Move 32-bit value from rd and rn1 to memory (rn + Offset). Syntax: `mov [3 + 0x60], 1, 2`
  **Operands:**
- **rd** (8-bit): Destination or source register depending on the specifier.
- **rn** (8-bit): Used as a base register with offset in specifiers 0x0A–0x11.
- **rn1** (8-bit): Secondary register used in 32-bit operations (specifiers 0x05, 0x09, 0x0D, 0x11).
- **normAddressing** (32-bit): Base memory address used in specifiers 0x02–0x09.

---

### Instruction: b
**Opcode:** 0x0A

**General Description:** Unconditional branch.

**Specifiers:**
- **00**: Syntax: `b label`

**Operands:**
- **label** (32-bit): Branch target address.

---

### Instruction: be
**Opcode:** 0x0B

**General Description:** Branch if equal.

**Specifiers:**
- **00**: Syntax: `be rd, rn, label`

**Operands:**
- **rd** (8-bit): First register for comparison.
- **rn** (8-bit): Second register for comparison.
- **label** (32-bit): Branch target address.

---

### Instruction: bne
**Opcode:** 0x0C

**General Description:** Branch if not equal.

**Specifiers:**
- **00**: Syntax: `bne rd, rn, label`

**Operands:**
- **rd** (8-bit): First register for comparison.
- **rn** (8-bit): Second register for comparison.
- **label** (32-bit): Branch target address.

---

### Instruction: blt
**Opcode:** 0x0D

**General Description:** Branch if less than.

**Specifiers:**
- **00**: Syntax: `blt rd, rn, label`

**Operands:**
- **rd** (8-bit): First register for comparison.
- **rn** (8-bit): Second register for comparison.
- **label** (32-bit): Branch target address.

---

### Instruction: bgt
**Opcode:** 0x0E

**General Description:** Branch if greater than.

**Specifiers:**
- **00**: Syntax: `bgt rd, rn, label`

**Operands:**
- **rd** (8-bit): First register for comparison.
- **rn** (8-bit): Second register for comparison.
- **label** (32-bit): Branch target address.

---

### Instruction: bro
**Opcode:** 0x0F

**General Description:** Branch if overflow is set.

**Specifiers:**
- **00**: Syntax: `bro label`

**Operands:**
- **label** (32-bit): Branch target address.
