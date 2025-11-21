# NeoCore 16x32 RTL Module Documentation

This document provides comprehensive documentation for all RTL modules in the NeoCore 16x32 dual-issue, 5-stage pipelined CPU core.

## Table of Contents

1. [Package Definition (neocore_pkg.sv)](#package-definition)
2. [Core Top (core_top.sv)](#core-top)
3. [Memory Subsystem (unified_memory.sv)](#memory-subsystem)
4. [Pipeline Stages](#pipeline-stages)
   - [Fetch Unit (fetch_unit.sv)](#fetch-unit)
   - [Decode Unit (decode_unit.sv)](#decode-unit)
   - [Execute Stage (execute_stage.sv)](#execute-stage)
   - [Memory Stage (memory_stage.sv)](#memory-stage)
   - [Writeback Stage (writeback_stage.sv)](#writeback-stage)
5. [Functional Units](#functional-units)
   - [ALU (alu.sv)](#alu)
   - [Multiply Unit (multiply_unit.sv)](#multiply-unit)
   - [Branch Unit (branch_unit.sv)](#branch-unit)
   - [Register File (register_file.sv)](#register-file)
6. [Control Logic](#control-logic)
   - [Issue Unit (issue_unit.sv)](#issue-unit)
   - [Hazard Unit (hazard_unit.sv)](#hazard-unit)
7. [Pipeline Registers (pipeline_regs.sv)](#pipeline-registers)

---

## Package Definition

### neocore_pkg.sv

**Purpose:** Defines all types, structures, enumerations, and constants used throughout the NeoCore CPU.

**Key Components:**

#### Enumerations

- **`opcode_e`**: All 26 instruction opcodes (NOP, ADD, SUB, MUL, AND, OR, XOR, LSH, RSH, MOV, B, BE, BNE, BLT, BGT, BRO, UMULL, SMULL, HLT, PSH, POP, JSR, RTS, WFI, ENI, DSI)
- **`alu_op_e`**: ALU operations (ALU_ADD, ALU_SUB, ALU_MUL, ALU_AND, ALU_OR, ALU_XOR, ALU_LSH, ALU_RSH, ALU_NOP)
- **`mem_size_e`**: Memory access sizes (MEM_BYTE, MEM_HALF, MEM_WORD)

#### Pipeline Stage Structures

**`if_id_t`** - Instruction Fetch to Instruction Decode:
- `valid`: Instruction valid flag
- `pc`: Program counter (32-bit)
- `inst_data`: Instruction bytes (104-bit = 13 bytes for longest instructions)
- `inst_len`: Instruction length (4-bit, 0-13)

**`id_ex_t`** - Instruction Decode to Execute:
- `valid`: Valid instruction flag
- `pc`: Program counter
- `opcode`: Decoded opcode
- `rd_addr`: Destination register address (4-bit)
- `rs1_addr`, `rs2_addr`: Source register addresses
- `rs1_data`, `rs2_data`: Source register data (16-bit)
- `immediate`: Immediate value (32-bit)
- `alu_op`: ALU operation
- `mem_read`, `mem_write`: Memory operation flags
- `mem_size`: Memory access size
- `mem_addr`: Memory address (32-bit)
- `branch_target`: Branch target address
- `rd_we`: Register write enable
- `is_branch`, `is_mul`, `is_halt`: Instruction type flags

**`ex_mem_t`** - Execute to Memory:
- All `id_ex_t` fields plus:
- `alu_result`: ALU result (32-bit)
- `mul_result_lo`, `mul_result_hi`: Multiply results (16-bit each)
- `branch_taken`: Branch taken flag
- `branch_pc`: Resolved branch PC
- `z_flag`, `v_flag`: Arithmetic flags
- `is_halt`: Halt instruction flag

**`mem_wb_t`** - Memory to Writeback:
- `valid`: Valid flag
- `pc`: Program counter
- `rd_addr`: Destination register
- `rd_data`: Result data (16-bit)
- `rd_we`: Write enable
- `is_halt`: Halt flag

#### Helper Functions

- **`get_inst_length(opcode, specifier)`**: Returns instruction length based on opcode and specifier
  - NOP, HLT, RTS, WFI, ENI, DSI: 2 bytes
  - PSH, POP: 3 bytes
  - ADD/SUB/MUL/AND/OR/XOR/LSH/RSH (reg mode): 4 bytes
  - ADD/SUB/MUL/AND/OR/XOR/LSH/RSH (imm mode): 5 bytes
  - UMULL, SMULL: 5 bytes
  - B, BRO, JSR: 6 bytes
  - ADD/SUB/MUL/AND/OR/XOR/LSH/RSH (mem mode): 7 bytes
  - BE, BNE, BLT, BGT: 8 bytes
  - MOV: 4-9 bytes depending on specifier

**Lines of Code:** 267

**Dependencies:** None (base package)

**Testing:** Implicitly tested through all other modules

---

## Core Top

### core_top.sv

**Purpose:** Top-level integration module that instantiates and connects all pipeline stages, functional units, and control logic. Implements the complete dual-issue, 5-stage pipeline.

**Architecture:**

```
┌─────────────┐
│   Memory    │◄──────────────────────────────┐
│  Interface  │                               │
└──────┬──────┘                               │
       │                                      │
       ├──────────┐                          │
       ↓          ↓                          │
  ┌─────────┐ ┌─────────┐                   │
  │  Fetch  │ │  Data   │                   │
  │  (128b) │ │  (32b)  │                   │
  └────┬────┘ └────┬────┘                   │
       │           │                         │
   ┌───▼────┐      │                        │
   │  IF/ID │      │                        │
   └───┬────┘      │                        │
       │           │                         │
   ┌───▼────────┐  │                        │
   │  Decode    │  │                        │
   │  (Dual)    │  │                        │
   └───┬────────┘  │                        │
       │           │                         │
   ┌───▼────────┐  │                        │
   │  Issue     │  │                        │
   │  Unit      │  │                        │
   └───┬────────┘  │                        │
       │           │                         │
   ┌───▼────┐      │                        │
   │ ID/EX  │      │                        │
   │ (Dual) │      │                        │
   └───┬────┘      │                        │
       │           │                         │
   ┌───▼────────┐  │                        │
   │  Execute   │  │                        │
   │  (Dual)    │  │                        │
   └───┬────────┘  │                        │
       │           │                         │
   ┌───▼────┐      │                        │
   │ EX/MEM │      │                        │
   │ (Dual) │      │                        │
   └───┬────┘      │                        │
       │           │                         │
   ┌───▼────────┐  │                        │
   │  Memory    │◄─┘                        │
   │  (Dual)    │───────────────────────────┘
   └───┬────────┘
       │
   ┌───▼────┐
   │ MEM/WB │
   │ (Dual) │
   └───┬────┘
       │
   ┌───▼────────┐
   │ Writeback  │
   │  (Dual)    │
   └────────────┘
       │
       ↓
  (Register File)
```

**Key Features:**

1. **Von Neumann Memory Interface:**
   - Instruction fetch: 128-bit (16 bytes) for variable-length instructions
   - Data access: 32-bit with byte/halfword/word granularity
   - Big-endian byte ordering throughout
   - Single unified memory address space

2. **Dual-Issue Pipeline:**
   - Up to 2 instructions per cycle
   - Issue restrictions enforced by issue unit
   - Separate execution paths for parallel operation
   - Unified writeback with arbitration

3. **Hazard Handling:**
   - Data hazard detection via hazard unit
   - Forwarding from EX, MEM, WB stages (6 sources per operand)
   - Load-use stall detection
   - Branch flush on misprediction

4. **Control Flow:**
   - Branch resolution in EX stage
   - Pipeline flush on taken branches
   - Halt propagation from decode through all stages
   - Core stalls when halted flag is set

**Inputs:**
- `clk`: System clock
- `rst`: Synchronous active-high reset
- `mem_if_rdata[127:0]`: Instruction data from memory (16 bytes)
- `mem_if_ack`: Instruction fetch acknowledgment
- `mem_data_rdata[31:0]`: Data read from memory
- `mem_data_ack`: Data access acknowledgment

**Outputs:**
- `mem_if_addr[31:0]`: Instruction fetch address
- `mem_if_req`: Instruction fetch request
- `mem_data_addr[31:0]`: Data access address
- `mem_data_wdata[31:0]`: Data to write
- `mem_data_we`: Data write enable
- `mem_data_size[1:0]`: Data access size
- `mem_data_req`: Data access request
- `dual_issue_active`: High when 2 instructions issue
- `halted`: High when core is halted

**Internal Modules:**
- fetch_unit: Instruction fetch logic
- decode_unit (×2): Dual decoders
- issue_unit: Dual-issue decision logic
- hazard_unit: Hazard detection and forwarding control
- execute_stage: ALU, multiply, branch execution
- memory_stage: Load/store operations
- writeback_stage: Register writeback
- register_file: 16×16-bit register file
- pipeline_regs: All inter-stage registers

**Reset Behavior:**
- PC initialized to 0x00000000
- All pipeline registers cleared
- All valid flags cleared
- Halt flag cleared

**Lines of Code:** 549

**Dependencies:** All other RTL modules

**Testing:** core_simple_tb.sv, core_unified_tb.sv

---

## Memory Subsystem

### unified_memory.sv

**Purpose:** Unified Von Neumann memory module with dual-port configuration for simultaneous instruction fetch and data access. Implements big-endian byte ordering and is synthesizable for FPGA.

**Memory Organization:**

```
Address Space (32-bit addressing):
┌──────────────────────────────┐ 0x00000000
│                              │
│      Instruction/Data        │ (Mixed, Von Neumann)
│          Region              │
│                              │
├──────────────────────────────┤ 0x0000FFFF (64KB)
│         Unmapped             │
│                              │
└──────────────────────────────┘ 0xFFFFFFFF
```

**Dual-Port Configuration:**

**Port A - Instruction Fetch:**
- Width: 128 bits (16 bytes)
- Purpose: Fetch up to 16 bytes for variable-length instructions
- Read-only
- Big-endian: Address N returns bytes [N, N+1, ..., N+15]
- Byte N at bits [127:120], Byte N+15 at bits [7:0]

**Port B - Data Access:**
- Width: 32 bits
- Purpose: Load/store operations
- Read/write capable
- Supports byte, halfword, word accesses
- Big-endian: 
  - Byte at address N stored in bits [31:24]
  - Halfword at N stored in bits [31:16]
  - Word at N stored in bits [31:0]

**Big-Endian Memory Layout:**

```
Physical Memory (byte array):
mem[0] = MSB of word at address 0
mem[1] = ...
mem[2] = ...
mem[3] = LSB of word at address 0

For 16-bit value 0x1234 at address 0x100:
mem[0x100] = 0x12
mem[0x101] = 0x34

For 32-bit value 0x12345678 at address 0x200:
mem[0x200] = 0x12
mem[0x201] = 0x34
mem[0x202] = 0x56
mem[0x203] = 0x78
```

**Interface:**

Instruction Port:
- `if_addr[31:0]`: Address to fetch (must be within range)
- `if_req`: Request signal
- `if_rdata[127:0]`: 16 bytes of instruction data
- `if_ack`: Acknowledgment (always 1 cycle delay)

Data Port:
- `data_addr[31:0]`: Address for load/store
- `data_wdata[31:0]`: Data to write
- `data_size[1:0]`: Access size (00=byte, 01=halfword, 10=word)
- `data_we`: Write enable
- `data_req`: Request signal
- `data_rdata[31:0]`: Data read
- `data_ack`: Acknowledgment (always 1 cycle delay)

**Memory Initialization:**

Uses `$readmemh` to load hex files in big-endian format:
```systemverilog
initial begin
  if (INIT_FILE != "") begin
    $readmemh(INIT_FILE, mem);
  end
end
```

**Write Behavior:**

Byte Write (size = 00):
- Only writes to mem[addr]
- Other bytes unchanged

Halfword Write (size = 01):
- Writes to mem[addr] and mem[addr+1]
- Big-endian: high byte to addr, low byte to addr+1

Word Write (size = 10):
- Writes to mem[addr] through mem[addr+3]
- Big-endian: MSB to addr, LSB to addr+3

**Read Behavior:**

Byte Read:
- Returns mem[addr] in bits [31:24]
- Other bits zero

Halfword Read:
- Returns {mem[addr], mem[addr+1]} in bits [31:16]
- Other bits zero

Word Read:
- Returns {mem[addr], mem[addr+1], mem[addr+2], mem[addr+3]}

**Parameters:**
- `MEM_SIZE`: Memory size in bytes (default 65536 = 64KB)
- `INIT_FILE`: Optional hex file for initialization

**Lines of Code:** 183

**Dependencies:** None

**Testing:** Tested through core integration tests

**Synthesis Notes:**
- Uses single-dimensional byte array for FPGA BRAM inference
- Registered outputs for timing
- Dual-port configuration maps to FPGA dual-port BRAM
- Total resource: 64KB = 512Kb BRAM

---

## Pipeline Stages

### Fetch Unit

#### fetch_unit.sv

**Purpose:** Fetches variable-length instructions from unified memory, maintains an instruction buffer for dual-issue, and manages the program counter.

**Key Challenges Addressed:**

1. **Variable-Length Instructions:** NeoCore instructions range from 2 to 13 bytes
2. **Dual-Issue:** Need to fetch and buffer up to 2 complete instructions per cycle
3. **Big-Endian:** Must correctly extract bytes in MSB-first order
4. **Memory Bandwidth:** 128-bit (16-byte) memory interface

**Architecture:**

```
Memory (128-bit) ──┐
                   │
                   ↓
            ┌──────────────┐
            │   256-byte   │
            │    Buffer    │
            └──────────────┘
                   │
                   ├─────► Instruction 0 (104 bits = 13 bytes)
                   │
                   └─────► Instruction 1 (104 bits = 13 bytes)
```

**Buffer Management:**

- **Size:** 256 bytes (large enough for worst case)
- **Organization:** Circular buffer with valid byte counter
- **Extraction:** Pull from top (highest address in buffer)
- **Refill:** Add to bottom as bytes are consumed

**Operation Flow:**

1. **Memory Request:**
   - Request 16 bytes when buffer has < 128 bytes
   - Address = current PC aligned to 16-byte boundary

2. **Buffer Update:**
   - On memory ack: append 16 bytes to buffer
   - On instruction consumption: shift out consumed bytes

3. **Instruction Extraction:**
   - Extract byte 0 (specifier) and byte 1 (opcode) from buffer top
   - Determine instruction length using `get_inst_length()`
   - Extract full instruction if enough bytes available

4. **Dual-Issue:**
   - Try to extract second instruction after first
   - Only if both complete instructions fit in buffer

5. **PC Update:**
   - Sequential: PC += length of consumed instructions
   - Branch: PC = branch target (when branch_taken)
   - Flush: Clear buffer, start fetch from new PC

**State Machine:**

```
IDLE ──► FETCH_REQ ──► WAIT_ACK ──► IDLE
  ↑                                   │
  └───────────────────────────────────┘
```

**Inputs:**
- `clk`, `rst`: Clock and reset
- `mem_rdata[127:0]`: 16 bytes from memory
- `mem_ack`: Memory acknowledge
- `branch_taken`: Branch decision from EX stage
- `branch_pc[31:0]`: Branch target
- `stall`: Pipeline stall signal

**Outputs:**
- `mem_addr[31:0]`: Memory fetch address
- `mem_req`: Memory request
- `inst0_valid`: Instruction 0 valid
- `inst0_data[103:0]`: Instruction 0 bytes
- `inst0_len[3:0]`: Instruction 0 length
- `inst0_pc[31:0]`: Instruction 0 PC
- `inst1_valid`: Instruction 1 valid (dual-issue)
- `inst1_data[103:0]`: Instruction 1 bytes
- `inst1_len[3:0]`: Instruction 1 length
- `inst1_pc[31:0]`: Instruction 1 PC

**Big-Endian Extraction Example:**

For instruction MOV R1, #0x1234 (5 bytes):
```
Buffer (big-endian):
buffer[255] = 0x00 (specifier)
buffer[254] = 0x09 (MOV opcode)
buffer[253] = 0x01 (R1)
buffer[252] = 0x12 (immediate high)
buffer[251] = 0x34 (immediate low)

Extracted inst_data[103:0]:
inst_data[103:96] = 0x00
inst_data[95:88]  = 0x09
inst_data[87:80]  = 0x01
inst_data[79:72]  = 0x12
inst_data[71:64]  = 0x34
inst_data[63:0]   = don't care
```

**Lines of Code:** 264

**Dependencies:** neocore_pkg.sv

**Testing:** Tested through core integration

---

### Decode Unit

#### decode_unit.sv

**Purpose:** Decodes variable-length instructions and extracts all fields including opcode, registers, immediates, and addresses. Handles all 26 opcodes and 19 MOV variants with big-endian byte extraction.

**Input Format:**

Big-endian instruction data (104 bits = 13 bytes):
```
inst_data[103:96] = Byte 0 (Specifier)
inst_data[95:88]  = Byte 1 (Opcode)
inst_data[87:80]  = Byte 2 (rd or operand)
inst_data[79:72]  = Byte 3
...
inst_data[7:0]    = Byte 12
```

**Decoding Process:**

1. **Extract Opcode and Specifier:**
   ```systemverilog
   specifier = inst_data[103:96];
   opcode    = inst_data[95:88];
   ```

2. **Determine Instruction Type:**
   - ALU ops: ADD, SUB, MUL, AND, OR, XOR, LSH, RSH
   - MOV ops: 19 different addressing modes
   - Branch ops: B, BE, BNE, BLT, BGT, BRO
   - Multiply ops: UMULL, SMULL
   - Stack ops: PSH, POP
   - Subroutine: JSR, RTS
   - Control: NOP, HLT, WFI, ENI, DSI

3. **Extract Operands Based on Type:**

**ALU Instructions (ADD/SUB/MUL/AND/OR/XOR/LSH/RSH):**

Specifier 00 - Immediate mode (5 bytes):
```
rd    = inst_data[87:84]  // destination register
imm   = inst_data[79:64]  // 16-bit immediate (big-endian)
```

Specifier 01 - Register mode (4 bytes):
```
rd    = inst_data[87:84]  // destination register  
rn    = inst_data[79:76]  // source register
```

Specifier 02 - Memory mode (7 bytes):
```
rd    = inst_data[87:84]  // destination register
addr  = inst_data[79:48]  // 32-bit address (big-endian)
```

**MOV Instruction (19 specifiers):**

Specifier 00 - MOV rd, #imm (5 bytes):
```
rd    = inst_data[87:84]
imm   = inst_data[79:64]
```

Specifier 05 - MOV rd, [addr] (7 bytes):
```
rd    = inst_data[87:84]
addr  = inst_data[79:48]
```

Specifier 0D - MOV rd, [rn + #offset] (8 bytes):
```
rd     = inst_data[87:84]
rn     = inst_data[79:76]
offset = inst_data[71:40]
```

(See complete MOV decoding in decode_unit.sv for all 19 variants)

**Branch Instructions:**

B, BRO, JSR (6 bytes):
```
target = inst_data[79:48]  // 32-bit target address
```

BE, BNE, BLT, BGT (8 bytes):
```
rd     = inst_data[87:84]  // compare operand 1
rn     = inst_data[79:76]  // compare operand 2
target = inst_data[71:40]  // branch target
```

**Multiply Instructions:**

UMULL, SMULL (5 bytes):
```
rd  = inst_data[87:84]  // low result register
rn  = inst_data[79:76]  // operand 1
rn1 = inst_data[71:68]  // operand 2
// High result goes to rd+1
```

**Outputs:**
- `opcode`: Decoded opcode
- `rd_addr`, `rs1_addr`, `rs2_addr`: Register addresses
- `immediate`: Immediate value (32-bit)
- `mem_addr`: Memory address for loads/stores
- `branch_target`: Branch target address
- `alu_op`: ALU operation to perform
- `mem_read`, `mem_write`: Memory operation flags
- `mem_size`: Byte/halfword/word access size
- `rd_we`: Register write enable
- Instruction type flags: `is_branch`, `is_mul`, `is_halt`

**Big-Endian Multi-Byte Extraction:**

16-bit value:
```systemverilog
value = {inst_data[79:72], inst_data[71:64]};  // high byte, low byte
```

32-bit value:
```systemverilog
value = {inst_data[79:72], inst_data[71:64], 
         inst_data[63:56], inst_data[55:48]};  // MSB to LSB
```

**Lines of Code:** 428

**Dependencies:** neocore_pkg.sv

**Testing:** decode_unit_tb.sv

---

### Execute Stage

#### execute_stage.sv

**Purpose:** Integrates ALU, multiply unit, and branch unit. Handles operand forwarding and supports dual-issue execution with two parallel datapaths.

**Architecture:**

```
                    Forwarding MUXes
                          │
        ┌─────────────────┼─────────────────┐
        │                 │                 │
   ┌────▼────┐       ┌────▼────┐       ┌───▼────┐
   │   ALU   │       │Multiply │       │ Branch │
   │  Path 0 │       │ Path 0  │       │ Path 0 │
   └────┬────┘       └────┬────┘       └───┬────┘
        │                 │                 │
        └─────────────────┼─────────────────┘
                          │
                    Results Slot 0
                          
                          
                    Forwarding MUXes
                          │
        ┌─────────────────┼─────────────────┐
        │                 │                 │
   ┌────▼────┐       ┌────▼────┐       ┌───▼────┐
   │   ALU   │       │Multiply │       │ Branch │
   │  Path 1 │       │ Path 1  │       │ Path 1 │
   └────┬────┘       └────┬────┘       └───┬────┘
        │                 │                 │
        └─────────────────┼─────────────────┘
                          │
                    Results Slot 1
```

**Forwarding Network:**

Each operand (rs1, rs2) can be forwarded from 6 sources:
- EX stage slot 0 result
- EX stage slot 1 result
- MEM stage slot 0 result
- MEM stage slot 1 result
- WB stage slot 0 result
- WB stage slot 1 result

**Forwarding MUX Selection:**

```systemverilog
// Example for operand A of instruction 0
always_comb begin
  case (fwd_rs1_sel_0)
    3'b000: operand_a_0 = id_ex_0.rs1_data;  // No forwarding
    3'b001: operand_a_0 = ex_fwd_data_0;     // From EX slot 0
    3'b010: operand_a_0 = ex_fwd_data_1;     // From EX slot 1
    3'b011: operand_a_0 = mem_fwd_data_0;    // From MEM slot 0
    3'b100: operand_a_0 = mem_fwd_data_1;    // From MEM slot 1
    3'b101: operand_a_0 = wb_fwd_data_0;     // From WB slot 0
    3'b110: operand_a_0 = wb_fwd_data_1;     // From WB slot 1
    default: operand_a_0 = id_ex_0.rs1_data;
  endcase
end
```

**Operation:**

For each instruction slot (0 and 1):

1. **Operand Selection:** Apply forwarding based on hazard unit signals
2. **ALU Execution:** If ALU operation, compute result and flags
3. **Multiply Execution:** If multiply, compute 32-bit result
4. **Branch Evaluation:** If branch, determine taken/not-taken
5. **Result Propagation:** Pass results to EX/MEM register

**Dual-Issue Interaction:**

- Both slots execute in parallel
- Results from slot 0 can forward to slot 1 in same cycle (EX-to-EX forwarding)
- Branch in either slot flushes pipeline

**Inputs:**
- `id_ex_0`, `id_ex_1`: Pipeline data for both instruction slots
- `fwd_rs1_sel_0`, `fwd_rs2_sel_0`: Forwarding selects for slot 0
- `fwd_rs1_sel_1`, `fwd_rs2_sel_1`: Forwarding selects for slot 1
- Forwarding data from EX, MEM, WB stages (6 sources)

**Outputs:**
- `ex_mem_0`, `ex_mem_1`: Results to EX/MEM register
- `branch_taken_0`, `branch_taken_1`: Branch decisions
- `branch_pc_0`, `branch_pc_1`: Branch targets

**Lines of Code:** 346

**Dependencies:** alu.sv, multiply_unit.sv, branch_unit.sv, neocore_pkg.sv

**Testing:** Tested through core integration

---

### Memory Stage

#### memory_stage.sv

**Purpose:** Handles load and store operations through the unified memory interface. Manages stack operations (push/pop) and subroutine calls (JSR/RTS).

**Memory Access Types:**

1. **Load Operations:**
   - Byte load: Read 8 bits, zero-extend to 16 bits
   - Halfword load: Read 16 bits
   - Word load: Read 32 bits (for JSR/RTS return address)

2. **Store Operations:**
   - Byte store: Write 8 bits
   - Halfword store: Write 16 bits
   - Word store: Write 32 bits (for stack operations)

3. **Stack Operations:**
   - PSH: Write 16-bit value to [SP], SP -= 2
   - POP: Read 16-bit value from [SP], SP += 2

4. **Subroutine Operations:**
   - JSR: Push 32-bit return address, jump to target
   - RTS: Pop 32-bit return address, jump to it

**Big-Endian Data Access:**

All data operations respect big-endian byte ordering:

Byte write at address A:
```
mem[A] = data[7:0]
```

Halfword write at address A:
```
mem[A]   = data[15:8]   // high byte
mem[A+1] = data[7:0]    // low byte
```

Word write at address A:
```
mem[A]   = data[31:24]  // MSB
mem[A+1] = data[23:16]
mem[A+2] = data[15:8]
mem[A+3] = data[7:0]    // LSB
```

**Read Data Extraction:**

From 32-bit memory read data:

Byte read:
```systemverilog
result = {8'h00, mem_rdata[31:24]};  // Zero-extend
```

Halfword read:
```systemverilog
result = mem_rdata[31:16];  // Upper 16 bits
```

**Dual-Issue Arbitration:**

When both instructions need memory:
- Slot 0 accesses memory first
- Slot 1 waits (serialized access)
- Total: 2 cycles for both accesses

**Interface:**

Inputs:
- `ex_mem_0`, `ex_mem_1`: Data from execute stage
- `mem_data_rdata[31:0]`: Read data from memory
- `mem_data_ack`: Memory acknowledge

Outputs:
- `mem_data_addr[31:0]`: Address for memory access
- `mem_data_wdata[31:0]`: Write data
- `mem_data_we`: Write enable
- `mem_data_size[1:0]`: Access size
- `mem_data_req`: Request signal
- `mem_wb_0`, `mem_wb_1`: Results to MEM/WB register

**Lines of Code:** 264

**Dependencies:** neocore_pkg.sv

**Testing:** Tested through core integration

---

### Writeback Stage

#### writeback_stage.sv

**Purpose:** Final pipeline stage that writes results back to the register file. Handles dual-issue writeback with register write port arbitration.

**Operation:**

For each instruction slot (0 and 1):

1. **Check Valid:** Only writeback if valid and rd_we asserted
2. **Select Result:** 
   - From ALU result
   - From multiply result
   - From memory load
3. **Write to Register File:** If rd != 0 (R0 is hardwired to 0)

**Dual-Issue Writeback:**

Register file has 2 write ports:
- Write port 0: For instruction slot 0
- Write port 1: For instruction slot 1

**Register Write Conflict:**

If both instructions write to same register:
- Priority to slot 0 (earlier instruction)
- Slot 1 write is suppressed

**Inputs:**
- `mem_wb_0`, `mem_wb_1`: Data from memory stage

**Outputs:**
- `wb_rd_addr_0`, `wb_rd_addr_1`: Destination registers
- `wb_rd_data_0`, `wb_rd_data_1`: Data to write
- `wb_rd_we_0`, `wb_rd_we_1`: Write enables
- `halt_0`, `halt_1`: Halt flags

**Lines of Code:** 105

**Dependencies:** neocore_pkg.sv

**Testing:** Tested through core integration

---

## Functional Units

### ALU

#### alu.sv

**Purpose:** Performs 16-bit arithmetic and logical operations. Generates zero (Z) and overflow (V) flags.

**Supported Operations:**

| Operation | ALU_OP | Description | Flags Updated |
|-----------|--------|-------------|---------------|
| ADD | ALU_ADD | Signed addition | Z, V |
| SUB | ALU_SUB | Signed subtraction | Z, V |
| MUL | ALU_MUL | Truncated 16-bit multiply | Z, V |
| AND | ALU_AND | Bitwise AND | Z |
| OR | ALU_OR | Bitwise OR | Z |
| XOR | ALU_XOR | Bitwise XOR | Z |
| LSH | ALU_LSH | Logical shift left | Z |
| RSH | ALU_RSH | Logical shift right | Z |
| NOP | ALU_NOP | No operation | - |

**Flag Generation:**

**Zero Flag (Z):**
```systemverilog
z_flag = (result[15:0] == 16'h0000);
```

**Overflow Flag (V):**

For ADD:
```systemverilog
// Overflow if signs match but result sign differs
v_flag = (operand_a[15] == operand_b[15]) && 
         (result[15] != operand_a[15]);
```

For SUB:
```systemverilog
// Overflow if subtracting negative from positive gives negative
// or subtracting positive from negative gives positive
v_flag = (operand_a[15] != operand_b[15]) && 
         (result[15] != operand_a[15]);
```

For MUL:
```systemverilog
// Overflow if result doesn't fit in 16 bits
signed_result = $signed(operand_a) * $signed(operand_b);
v_flag = (signed_result > 32767) || (signed_result < -32768);
```

**Implementation:**

Pure combinational logic:
```systemverilog
always_comb begin
  case (alu_op)
    ALU_ADD: result = operand_a + operand_b;
    ALU_SUB: result = operand_a - operand_b;
    ALU_MUL: result = operand_a * operand_b;
    ALU_AND: result = operand_a & operand_b;
    ALU_OR:  result = operand_a | operand_b;
    ALU_XOR: result = operand_a ^ operand_b;
    ALU_LSH: result = operand_a << operand_b[3:0];
    ALU_RSH: result = operand_a >> operand_b[3:0];
    default: result = 32'h0;
  endcase
end
```

**Inputs:**
- `operand_a[15:0]`: First operand
- `operand_b[15:0]`: Second operand
- `alu_op`: Operation to perform

**Outputs:**
- `result[31:0]`: Result (lower 16 bits for most ops)
- `z_flag`: Zero flag
- `v_flag`: Overflow flag

**Lines of Code:** 122

**Dependencies:** neocore_pkg.sv

**Testing:** alu_tb.sv (comprehensive tests for all operations)

---

### Multiply Unit

#### multiply_unit.sv

**Purpose:** Performs 16×16 → 32-bit multiplication, both unsigned (UMULL) and signed (SMULL).

**Operations:**

**UMULL (Unsigned Multiply Long):**
```systemverilog
{result_hi, result_lo} = operand_a * operand_b;
```
- Both operands treated as unsigned
- 32-bit result split into two 16-bit registers

**SMULL (Signed Multiply Long):**
```systemverilog
{result_hi, result_lo} = $signed(operand_a) * $signed(operand_b);
```
- Both operands treated as signed (two's complement)
- 32-bit result split into two 16-bit registers

**Result Storage:**

Given `UMULL Rd, Rn, Rm`:
- `Rd` receives result_lo (bits [15:0])
- `Rd+1` receives result_hi (bits [31:16])

**Implementation:**

```systemverilog
always_comb begin
  if (is_signed) begin
    // Signed multiplication
    signed_result = $signed(operand_a) * $signed(operand_b);
    mul_result = signed_result[31:0];
  end else begin
    // Unsigned multiplication
    mul_result = operand_a * operand_b;
  end
  
  result_lo = mul_result[15:0];
  result_hi = mul_result[31:16];
end
```

**Inputs:**
- `operand_a[15:0]`: First operand
- `operand_b[15:0]`: Second operand
- `is_signed`: 1 for SMULL, 0 for UMULL

**Outputs:**
- `result_lo[15:0]`: Lower 16 bits of result
- `result_hi[15:0]`: Upper 16 bits of result

**Examples:**

UMULL R0, R1, R2 where R1=0xFFFF, R2=0xFFFF:
```
Result = 0xFFFF × 0xFFFF = 0xFFFE0001
R0 = 0x0001 (low)
R1 = 0xFFFE (high)
```

SMULL R0, R1, R2 where R1=0xFFFF (-1), R2=0xFFFF (-1):
```
Result = (-1) × (-1) = 0x00000001
R0 = 0x0001 (low)
R1 = 0x0000 (high)
```

**Lines of Code:** 61

**Dependencies:** neocore_pkg.sv

**Testing:** multiply_unit_tb.sv

---

### Branch Unit

#### branch_unit.sv

**Purpose:** Evaluates branch conditions and determines whether a branch should be taken.

**Branch Instructions:**

| Opcode | Mnemonic | Condition | Description |
|--------|----------|-----------|-------------|
| 0x0A | B | Always | Unconditional branch |
| 0x0B | BE | rs1 == rs2 | Branch if equal |
| 0x0C | BNE | rs1 != rs2 | Branch if not equal |
| 0x0D | BLT | rs1 < rs2 (signed) | Branch if less than |
| 0x0E | BGT | rs1 > rs2 (signed) | Branch if greater than |
| 0x0F | BRO | V == 1 | Branch if overflow |
| 0x15 | JSR | Always | Jump to subroutine |

**Implementation:**

```systemverilog
always_comb begin
  branch_taken = 1'b0;
  branch_pc = branch_target;
  
  case (opcode)
    OP_B:   branch_taken = 1'b1;  // Unconditional
    OP_BE:  branch_taken = (operand_a == operand_b);
    OP_BNE: branch_taken = (operand_a != operand_b);
    OP_BLT: branch_taken = ($signed(operand_a) < $signed(operand_b));
    OP_BGT: branch_taken = ($signed(operand_a) > $signed(operand_b));
    OP_BRO: branch_taken = v_flag_in;
    OP_JSR: branch_taken = 1'b1;  // Unconditional
    default: branch_taken = 1'b0;
  endcase
end
```

**Signed Comparison:**

Uses `$signed()` cast for BLT and BGT to treat operands as two's complement signed integers.

Examples:
- 0x0001 > 0xFFFF (unsigned), but 1 > -1 (signed) = false
- 0xFFFE < 0x0001 (unsigned comparison), but -2 < 1 (signed) = true

**Branch Taken Behavior:**

When branch is taken:
1. `branch_taken` asserts
2. `branch_pc` provides target address
3. Core flushes IF and ID stages
4. Fetch resumes from `branch_pc`

**Inputs:**
- `opcode`: Branch instruction opcode
- `operand_a[15:0]`: First comparison operand
- `operand_b[15:0]`: Second comparison operand
- `v_flag_in`: Current overflow flag (for BRO)
- `branch_target[31:0]`: Target address

**Outputs:**
- `branch_taken`: 1 if branch should be taken
- `branch_pc[31:0]`: PC to branch to

**Lines of Code:** 83

**Dependencies:** neocore_pkg.sv

**Testing:** branch_unit_tb.sv

---

### Register File

#### register_file.sv

**Purpose:** Implements 16 general-purpose 16-bit registers with dual-port read/write capability and internal forwarding.

**Register Organization:**

```
R0  : Always 0 (hardwired)
R1  : General purpose
R2  : General purpose
...
R14 : General purpose
R15 : General purpose (often used as stack pointer)
```

**Dual-Port Configuration:**

**Read Ports (4 total):**
- Port A1: Read address rs1_addr_a → rs1_data_a
- Port A2: Read address rs2_addr_a → rs2_data_a
- Port B1: Read address rs1_addr_b → rs1_data_b
- Port B2: Read address rs2_addr_b → rs2_data_b

**Write Ports (2 total):**
- Port A: Write to rd_addr_a if rd_we_a asserted
- Port B: Write to rd_addr_b if rd_we_b asserted

**Internal Forwarding:**

If read and write happen to same register in same cycle, forward write data directly to read port:

```systemverilog
// Read port A1 with forwarding
if (rd_we_a && (rd_addr_a == rs1_addr_a) && (rs1_addr_a != 4'h0)) begin
  rs1_data_a = rd_data_a;  // Forward from write port A
end else if (rd_we_b && (rd_addr_b == rs1_addr_a) && (rs1_addr_a != 4'h0)) begin
  rs1_data_a = rd_data_b;  // Forward from write port B
end else begin
  rs1_data_a = registers[rs1_addr_a];  // Normal read
end
```

**R0 Hardwiring:**

Register 0 is always 0:
```systemverilog
// Writes to R0 are ignored
if (rd_we_a && (rd_addr_a != 4'h0)) begin
  registers[rd_addr_a] <= rd_data_a;
end

// Reads from R0 always return 0
if (rs1_addr_a == 4'h0) begin
  rs1_data_a = 16'h0000;
end
```

**Reset Behavior:**

All registers cleared to 0 on reset:
```systemverilog
if (rst) begin
  for (int i = 0; i < 16; i++) begin
    registers[i] <= 16'h0000;
  end
end
```

**Inputs:**
- `clk`, `rst`
- Read addresses: `rs1_addr_a`, `rs2_addr_a`, `rs1_addr_b`, `rs2_addr_b`
- Write ports: `rd_addr_a`, `rd_data_a`, `rd_we_a`, `rd_addr_b`, `rd_data_b`, `rd_we_b`

**Outputs:**
- Read data: `rs1_data_a`, `rs2_data_a`, `rs1_data_b`, `rs2_data_b`

**Lines of Code:** 120

**Dependencies:** neocore_pkg.sv

**Testing:** register_file_tb.sv

---

## Control Logic

### Issue Unit

#### issue_unit.sv

**Purpose:** Determines whether one or two instructions can be issued in a given cycle based on dual-issue restrictions and dependencies.

**Dual-Issue Rules:**

Two instructions can issue together ONLY if ALL conditions are met:

1. **Both instructions are valid**
2. **At most one memory operation** (load or store)
3. **No branches** - branches always issue alone
4. **No multiply operations** - UMULL/SMULL issue alone
5. **No structural hazards** - both instructions don't write to same register
6. **No RAW dependencies** - instruction 1 doesn't depend on result of instruction 0

**Implementation:**

```systemverilog
always_comb begin
  // Default: single issue
  issue_inst0 = inst0_valid;
  issue_inst1 = 1'b0;
  dual_issue = 1'b0;
  
  if (inst0_valid && inst1_valid) begin
    // Check all restrictions
    logic can_dual_issue;
    
    can_dual_issue = 1'b1;
    
    // Rule 1: At most one memory operation
    if ((inst0_mem_read || inst0_mem_write) && 
        (inst1_mem_read || inst1_mem_write)) begin
      can_dual_issue = 1'b0;
    end
    
    // Rule 2: Branches issue alone
    if (inst0_is_branch || inst1_is_branch) begin
      can_dual_issue = 1'b0;
    end
    
    // Rule 3: Multiply operations issue alone
    if (inst0_is_mul || inst1_is_mul) begin
      can_dual_issue = 1'b0;
    end
    
    // Rule 4: Structural hazard - same destination register
    if (inst0_rd_we && inst1_rd_we && 
        (inst0_rd_addr == inst1_rd_addr) && 
        (inst0_rd_addr != 4'h0)) begin
      can_dual_issue = 1'b0;
    end
    
    // Rule 5: RAW dependency - inst1 reads what inst0 writes
    if (inst0_rd_we && (inst0_rd_addr != 4'h0)) begin
      if ((inst1_rs1_valid && (inst1_rs1_addr == inst0_rd_addr)) ||
          (inst1_rs2_valid && (inst1_rs2_addr == inst0_rd_addr))) begin
        can_dual_issue = 1'b0;
      end
    end
    
    if (can_dual_issue) begin
      issue_inst0 = 1'b1;
      issue_inst1 = 1'b1;
      dual_issue = 1'b1;
    end
  end
end
```

**Examples:**

✅ Can dual-issue:
```
NOP
NOP
```

✅ Can dual-issue:
```
ADD R1, R2, R3
ADD R4, R5, R6
```
(Different destination registers, no dependencies)

❌ Cannot dual-issue:
```
ADD R1, R2, R3
ADD R4, R1, R5
```
(Instruction 1 reads R1 which instruction 0 writes)

❌ Cannot dual-issue:
```
MOV R1, [0x1000]
MOV R2, [0x2000]
```
(Two memory operations)

❌ Cannot dual-issue:
```
B target
ADD R1, R2, R3
```
(Branch must issue alone)

**Inputs:**
- `inst0_valid`, `inst1_valid`: Instruction validity
- Instruction 0 info: `inst0_opcode`, `inst0_rd_addr`, `inst0_rs1_addr`, etc.
- Instruction 1 info: `inst1_opcode`, `inst1_rd_addr`, `inst1_rs1_addr`, etc.

**Outputs:**
- `issue_inst0`: Issue instruction 0
- `issue_inst1`: Issue instruction 1
- `dual_issue`: Both instructions issuing

**Lines of Code:** 154

**Dependencies:** neocore_pkg.sv

**Testing:** Tested through core integration

---

### Hazard Unit

#### hazard_unit.sv

**Purpose:** Detects data hazards (RAW dependencies) and generates forwarding control signals. Also detects load-use hazards that require pipeline stalls.

**Hazard Types:**

1. **Read-After-Write (RAW):**
   - Instruction reads a register that a previous instruction will write
   - Solved by forwarding or stalling

2. **Load-Use:**
   - Instruction uses result of a load in the immediately following cycle
   - Cannot forward (load data not available yet)
   - Requires 1-cycle stall

**Forwarding Sources:**

For dual-issue, each operand can be forwarded from 6 sources:

1. **EX stage slot 0** - Result from first execution unit
2. **EX stage slot 1** - Result from second execution unit
3. **MEM stage slot 0** - Result from first memory access
4. **MEM stage slot 1** - Result from second memory access
5. **WB stage slot 0** - Result from first writeback
6. **WB stage slot 1** - Result from second writeback

**Forwarding Priority:**

Closer stages have priority (most recent producer):
1. EX stage (1 cycle ago)
2. MEM stage (2 cycles ago)
3. WB stage (3 cycles ago)

**Forwarding Logic:**

For instruction in ID stage reading register Rs:

```systemverilog
// Check EX stage slot 0
if (ex_rd_we_0 && (ex_rd_addr_0 == rs_addr) && (rs_addr != 4'h0)) begin
  forward_sel = 3'b001;  // Forward from EX slot 0
end
// Check EX stage slot 1
else if (ex_rd_we_1 && (ex_rd_addr_1 == rs_addr) && (rs_addr != 4'h0)) begin
  forward_sel = 3'b010;  // Forward from EX slot 1
end
// Check MEM stage slot 0
else if (mem_rd_we_0 && (mem_rd_addr_0 == rs_addr) && (rs_addr != 4'h0)) begin
  forward_sel = 3'b011;  // Forward from MEM slot 0
end
// ... and so on for other sources
else begin
  forward_sel = 3'b000;  // No forwarding
end
```

**Load-Use Stall Detection:**

```systemverilog
// Instruction in EX is a load
if ((ex_mem_read_0 || ex_mem_read_1) && 
    // Instruction in ID uses the load result
    ((id_rs1_addr == ex_rd_addr_0) || (id_rs2_addr == ex_rd_addr_0))) begin
  stall_id = 1'b1;  // Stall decode stage
  stall_if = 1'b1;  // Stall fetch stage
  flush_ex = 1'b1;  // Insert bubble in execute stage
end
```

**Control Hazards:**

Branch instructions cause control hazards:
- Branch resolved in EX stage
- If taken, flush IF and ID stages
- 2-cycle branch penalty

**Inputs:**
- ID stage: `id_rs1_addr_0`, `id_rs2_addr_0`, `id_rs1_addr_1`, `id_rs2_addr_1`
- EX stage: `ex_rd_addr_0`, `ex_rd_we_0`, `ex_mem_read_0`, etc.
- MEM stage: `mem_rd_addr_0`, `mem_rd_we_0`, etc.
- WB stage: `wb_rd_addr_0`, `wb_rd_we_0`, etc.

**Outputs:**
- Forwarding selects: `fwd_rs1_sel_0`, `fwd_rs2_sel_0`, `fwd_rs1_sel_1`, `fwd_rs2_sel_1`
- Stall signals: `stall_if`, `stall_id`
- Flush signal: `flush_ex`

**Lines of Code:** 235

**Dependencies:** neocore_pkg.sv

**Testing:** Tested through core integration

---

## Pipeline Registers

### pipeline_regs.sv

**Purpose:** Implements all inter-stage pipeline registers with support for stall, flush, and dual-issue.

**Pipeline Register Types:**

1. **IF/ID Register** - Between Fetch and Decode
2. **ID/EX Register** - Between Decode and Execute (dual)
3. **EX/MEM Register** - Between Execute and Memory (dual)
4. **MEM/WB Register** - Between Memory and Writeback (dual)

**Features:**

- **Stall:** Hold current value when stall signal asserted
- **Flush:** Clear to invalid state when flush signal asserted
- **Dual-Issue:** ID/EX, EX/MEM, MEM/WB have two slots (0 and 1)

**Implementation:**

IF/ID Register:
```systemverilog
always_ff @(posedge clk) begin
  if (rst || flush) begin
    if_id_reg.valid <= 1'b0;
    if_id_reg.pc <= 32'h0;
    if_id_reg.inst_data <= 104'h0;
    if_id_reg.inst_len <= 4'h0;
  end else if (!stall) begin
    if_id_reg <= if_id_next;
  end
  // else hold current value
end
```

ID/EX Register (dual):
```systemverilog
always_ff @(posedge clk) begin
  if (rst || flush) begin
    id_ex_reg_0.valid <= 1'b0;
    id_ex_reg_1.valid <= 1'b0;
    // Clear all fields
  end else if (!stall) begin
    id_ex_reg_0 <= id_ex_next_0;
    id_ex_reg_1 <= id_ex_next_1;
  end
end
```

**Flush Behavior:**

- IF/ID: Clears when branch taken or stall
- ID/EX: Clears when branch taken
- EX/MEM: Typically not flushed
- MEM/WB: Never flushed

**Stall Behavior:**

When stalled:
- IF/ID: Holds current instruction
- ID/EX: Holds current decoded state
- Fetch unit stops requesting new instructions

**Reset Behavior:**

All pipeline registers cleared:
- valid flags set to 0
- All data fields cleared
- Ensures clean startup

**Lines of Code:** 142

**Dependencies:** neocore_pkg.sv

**Testing:** Tested through core integration

---

## Summary Statistics

| Module | Lines of Code | Purpose | Testing |
|--------|---------------|---------|---------|
| neocore_pkg.sv | 267 | Type definitions | Implicit |
| core_top.sv | 549 | Top-level integration | core_simple_tb.sv |
| unified_memory.sv | 183 | Von Neumann memory | Integration tests |
| fetch_unit.sv | 264 | Instruction fetch | Integration tests |
| decode_unit.sv | 428 | Instruction decode | decode_unit_tb.sv |
| execute_stage.sv | 346 | Execution integration | Integration tests |
| memory_stage.sv | 264 | Memory operations | Integration tests |
| writeback_stage.sv | 105 | Result writeback | Integration tests |
| alu.sv | 122 | Arithmetic/logic | alu_tb.sv |
| multiply_unit.sv | 61 | Multiply operations | multiply_unit_tb.sv |
| branch_unit.sv | 83 | Branch evaluation | branch_unit_tb.sv |
| register_file.sv | 120 | Register storage | register_file_tb.sv |
| issue_unit.sv | 154 | Dual-issue control | Integration tests |
| hazard_unit.sv | 235 | Hazard detection | Integration tests |
| pipeline_regs.sv | 142 | Pipeline registers | Integration tests |
| **Total** | **3,323** | | |

---

## Design Principles

Throughout the implementation, these principles were followed:

1. **Educational Quality:** Code is written to be clear and understandable, with extensive comments
2. **Synthesizable:** All RTL uses synthesizable constructs suitable for FPGA
3. **Big-Endian:** Consistent big-endian byte ordering throughout
4. **Snake_Case:** All signals and modules use snake_case naming
5. **Synchronous Reset:** All sequential logic uses synchronous active-high reset
6. **Modular:** Clear separation of concerns with well-defined interfaces
7. **Testable:** Unit tests for all major functional units

---

## References

- Machine description file in repository root
- README.md for ISA and architecture overview
- DEVELOPER_GUIDE.md for implementation guide
- Individual testbenches in `tb/` directory

