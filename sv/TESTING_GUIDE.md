# NeoCore 16x32 Testing Guide

This document provides comprehensive information about testing the NeoCore 16x32 dual-issue CPU core.

## Table of Contents

1. [Test Infrastructure](#test-infrastructure)
2. [Unit Tests](#unit-tests)
3. [Integration Tests](#integration-tests)
4. [Test Program Development](#test-program-development)
5. [Debugging Failed Tests](#debugging-failed-tests)
6. [Performance Benchmarking](#performance-benchmarking)

---

## Test Infrastructure

### Build System

The project uses a Makefile-based build system with Icarus Verilog (iverilog) and VVP for simulation.

**Location:** `sv/Makefile`

**Key Targets:**

```bash
make all                    # Run all unit tests
make run_alu_tb            # Run ALU testbench
make run_register_file_tb  # Run register file testbench
make run_multiply_unit_tb  # Run multiply unit testbench
make run_branch_unit_tb    # Run branch unit testbench
make run_decode_unit_tb    # Run decode unit testbench
make run_core_simple_tb    # Run simple core integration test
make run_core_unified_tb   # Run full core integration test
make clean                 # Clean all build artifacts
```

### Directory Structure

```
sv/
├── rtl/            # RTL source files
├── tb/             # Testbenches
├── mem/            # Memory initialization files
├── scripts/        # Helper scripts
└── build/          # Build artifacts (generated)
```

### Simulation Tool

**Icarus Verilog (iverilog):**
- Open-source Verilog simulator
- Supports SystemVerilog subset
- Fast compilation and simulation
- Good for functional verification

**Installation:**
```bash
sudo apt-get install iverilog  # Ubuntu/Debian
brew install icarus-verilog    # macOS
```

---

## Unit Tests

Unit tests verify individual modules in isolation.

### ALU Testbench

**File:** `tb/alu_tb.sv`

**Purpose:** Tests all ALU operations and flag generation.

**Test Cases:**

| Test | Operation | Operands | Expected Result | Flags |
|------|-----------|----------|-----------------|-------|
| 1 | ADD | 10 + 20 | 30 | Z=0, V=0 |
| 2 | ADD | 0xFFFF + 1 | 0 | Z=1, V=0 |
| 3 | ADD | 0x7FFF + 1 | 0x8000 | Z=0, V=1 (overflow) |
| 4 | SUB | 20 - 10 | 10 | Z=0, V=0 |
| 5 | SUB | 10 - 10 | 0 | Z=1, V=0 |
| 6 | MUL | 10 × 20 | 200 | Z=0, V=0 |
| 7 | AND | 0xF0F0 & 0xFF00 | 0xF000 | Z=0 |
| 8 | OR | 0xF0F0 | 0x0F0F | 0xFFFF | Z=0 |
| 9 | XOR | 0xFFFF ^ 0xFFFF | 0 | Z=1 |
| 10 | LSH | 1 << 4 | 16 | Z=0 |
| 11 | RSH | 16 >> 2 | 4 | Z=0 |

**Running:**
```bash
make run_alu_tb
```

**Expected Output:**
```
ALU Testbench
Test 1: ADD - PASS
Test 2: ADD (zero) - PASS
Test 3: ADD (overflow) - PASS
...
Test 11: RSH - PASS
All tests PASSED!
```

---

### Register File Testbench

**File:** `tb/register_file_tb.sv`

**Purpose:** Tests register read/write, dual-port operation, and forwarding.

**Test Cases:**

1. **Basic Write/Read:**
   - Write 0x1234 to R1
   - Read R1, verify 0x1234

2. **Dual-Port Read:**
   - Write different values to R1, R2
   - Read both simultaneously
   - Verify both correct

3. **Dual-Port Write:**
   - Write to R1 and R2 simultaneously
   - Read both, verify

4. **Internal Forwarding:**
   - Write to R1 and read R1 in same cycle
   - Verify forwarded data

5. **R0 Hardwiring:**
   - Write to R0
   - Read R0, verify always 0

6. **Write Collision:**
   - Write different values to R1 from both ports
   - Verify arbitration

**Running:**
```bash
make run_register_file_tb
```

---

### Multiply Unit Testbench

**File:** `tb/multiply_unit_tb.sv`

**Purpose:** Tests 16×16 → 32-bit multiplication (unsigned and signed).

**Test Cases:**

| Test | Type | Operand A | Operand B | Expected Result |
|------|------|-----------|-----------|-----------------|
| 1 | UMULL | 0x0010 | 0x0020 | 0x00000200 |
| 2 | UMULL | 0xFFFF | 0x0002 | 0x0001FFFE |
| 3 | UMULL | 0xFFFF | 0xFFFF | 0xFFFE0001 |
| 4 | SMULL | 0xFFFF (-1) | 0xFFFF (-1) | 0x00000001 |
| 5 | SMULL | 0xFFFF (-1) | 0x0002 (2) | 0xFFFFFFFE (-2) |
| 6 | SMULL | 0x8000 | 0x0002 | 0xFFFF0000 |

**Verification:**
- `result_lo` = bits [15:0]
- `result_hi` = bits [31:16]

**Running:**
```bash
make run_multiply_unit_tb
```

---

### Branch Unit Testbench

**File:** `tb/branch_unit_tb.sv`

**Purpose:** Tests all branch conditions.

**Test Cases:**

| Test | Instruction | Operand A | Operand B | V Flag | Expected Taken |
|------|-------------|-----------|-----------|--------|----------------|
| 1 | B | - | - | - | Yes (unconditional) |
| 2 | BE | 0x1234 | 0x1234 | - | Yes (equal) |
| 3 | BE | 0x1234 | 0x5678 | - | No (not equal) |
| 4 | BNE | 0x1234 | 0x5678 | - | Yes (not equal) |
| 5 | BNE | 0x1234 | 0x1234 | - | No (equal) |
| 6 | BLT | 0xFFFF | 0x0001 | - | Yes (-1 < 1 signed) |
| 7 | BLT | 0x0001 | 0xFFFF | - | No (1 > -1 signed) |
| 8 | BGT | 0x0001 | 0xFFFF | - | Yes (1 > -1 signed) |
| 9 | BGT | 0xFFFF | 0x0001 | - | No (-1 < 1 signed) |
| 10 | BRO | - | - | 1 | Yes (overflow set) |
| 11 | BRO | - | - | 0 | No (overflow clear) |
| 12 | JSR | - | - | - | Yes (unconditional) |

**Running:**
```bash
make run_branch_unit_tb
```

---

### Decode Unit Testbench

**File:** `tb/decode_unit_tb.sv`

**Purpose:** Tests instruction decoding for all opcodes with big-endian format.

**Test Cases:**

1. **NOP:** `inst_data[103:96]=0x00, [95:88]=0x00`
2. **ADD immediate:** `sp=0x00, op=0x01, rd=R1, imm=0x1234`
3. **ADD register:** `sp=0x01, op=0x01, rd=R1, rn=R2`
4. **MOV immediate:** `sp=0x00, op=0x09, rd=R1, imm=0xABCD`
5. **B (branch):** `sp=0x00, op=0x0A, target=0x12345678`
6. **BNE:** `sp=0x00, op=0x0C, rd=R1, rn=R2, target=0x00001000`
7. **HLT:** `sp=0x00, op=0x12`
8. **UMULL:** `sp=0x00, op=0x10, rd=R0, rn=R1, rn1=R2`

**Big-Endian Encoding Example:**

For MOV R1, #0x1234:
```systemverilog
inst_data = 104'h00_09_01_12_34_00_00_00_00_00_00_00_00;
//           ^^ ^^ ^^ ^^ ^^
//           │  │  │  └──└─ Immediate (big-endian)
//           │  │  └──────── Destination register
//           │  └──────────── Opcode (MOV)
//           └──────────────── Specifier
```

**Verification:**
- Correct opcode extraction
- Correct register addresses
- Correct immediate/address extraction (big-endian)
- Correct ALU operation determination
- Correct memory operation flags

**Running:**
```bash
make run_decode_unit_tb
```

---

## Integration Tests

Integration tests verify the complete CPU core with all modules connected.

### Simple Core Test

**File:** `tb/core_simple_tb.sv`

**Purpose:** Basic sanity test of the integrated core.

**Test Program:**
```assembly
NOP          ; 0x00: No operation
NOP          ; 0x02: No operation
HLT          ; 0x04: Halt execution
```

**Memory Format (Big-Endian):**
```
Address | Value | Description
--------|-------|------------
0x0000  | 0x00  | NOP specifier
0x0001  | 0x00  | NOP opcode
0x0002  | 0x00  | NOP specifier
0x0003  | 0x00  | NOP opcode
0x0004  | 0x00  | HLT specifier
0x0005  | 0x12  | HLT opcode
```

**Expected Behavior:**
1. PC starts at 0x00000000
2. First NOP executes
3. Second NOP executes (dual-issue if enabled)
4. HLT executes
5. Core halts (halted signal asserts)
6. PC stops advancing

**Verification Points:**
- Dual-issue indicator active when 2 NOPs issue
- PC advances by correct amounts
- Core halts on HLT instruction
- Pipeline properly stalls

**Running:**
```bash
make run_core_simple_tb
```

---

### Unified Core Test

**File:** `tb/core_unified_tb.sv`

**Purpose:** Comprehensive test with Von Neumann memory model.

**Test Program:**
```assembly
        ; Test ALU operations
MOV R1, #10      ; R1 = 10
MOV R2, #20      ; R2 = 20
ADD R3, R1, R2   ; R3 = R1 + R2 = 30
SUB R4, R2, R1   ; R4 = R2 - R1 = 10

        ; Test memory operations
MOV R5, [0x1000] ; Load from memory
MOV [0x2000], R3 ; Store to memory

        ; Test branch
B done           ; Unconditional branch

skip:
MOV R6, #0xFF    ; Should be skipped

done:
HLT              ; Halt
```

**Verification:**
- All register values match expected
- Memory stores complete correctly
- Branch skips correct instruction
- Dual-issue occurs where possible
- Pipeline hazards handled correctly

**Running:**
```bash
make run_core_unified_tb
```

---

## Test Program Development

### Creating Test Programs

#### Step 1: Write Assembly

Create file `mem/test_example.asm`:
```assembly
; Example test program
MOV R1, #100     ; R1 = 100
MOV R2, #50      ; R2 = 50
ADD R3, R1, R2   ; R3 = 150
HLT              ; Stop
```

#### Step 2: Assemble (if assembler available)

```bash
./assembler test_example.asm -o test_example.bin
```

#### Step 3: Convert to Big-Endian Hex

Use the provided script:
```bash
python3 scripts/bin2hex.py test_example.bin > mem/test_example.hex
```

**Note:** The script must output big-endian format matching memory layout.

#### Step 4: Create Hex File Manually (if no assembler)

For manual creation, use big-endian byte ordering:

**Example:** MOV R1, #100 (0x0064)
```
00 09 01 00 64  ; sp=0x00, op=0x09 (MOV), rd=0x01, imm=0x0064
```

Hex file format:
```hex
00
09
01
00
64
00
09
02
00
32
...
```

### Big-Endian Memory Initialization

**Important:** All hex files must be in big-endian format!

**Example Program Encoding:**

```
Instruction: ADD R1, #0x1234
Length: 5 bytes
Big-endian hex:
00  ; Specifier (immediate mode)
01  ; Opcode (ADD)
01  ; Destination register (R1)
12  ; Immediate high byte
34  ; Immediate low byte
```

### Test Program Template

```systemverilog
module my_test_tb;
  // Signals
  logic clk = 0;
  logic rst = 1;
  
  // Unified memory
  unified_memory #(
    .MEM_SIZE(65536),
    .INIT_FILE("mem/my_test.hex")
  ) mem (
    // Connections...
  );
  
  // Core
  core_top core (
    .clk(clk),
    .rst(rst),
    // Memory connections...
  );
  
  // Clock generation
  always #5 clk = ~clk;  // 100 MHz
  
  // Test sequence
  initial begin
    rst = 1;
    #20 rst = 0;
    
    // Wait for halt
    wait(core.halted);
    
    #100;
    
    // Check results
    if (core.reg_file.registers[1] == 16'd100) begin
      $display("PASS: R1 = 100");
    end else begin
      $display("FAIL: R1 = %d, expected 100", 
               core.reg_file.registers[1]);
    end
    
    $finish;
  end
endmodule
```

---

## Debugging Failed Tests

### Common Issues

#### 1. Big-Endian vs Little-Endian Mismatch

**Symptom:** Instructions decode incorrectly, wrong opcodes, garbage data

**Solution:**
- Verify hex file uses big-endian byte ordering
- Check instruction extraction in fetch_unit.sv
- Verify decode_unit.sv reads bytes from correct bit positions

**Example Check:**
```systemverilog
// Correct big-endian extraction
byte0 = inst_data[103:96];  // Specifier (MSB)
byte1 = inst_data[95:88];   // Opcode
byte2 = inst_data[87:80];   // First operand
```

#### 2. Instruction Length Calculation

**Symptom:** PC doesn't advance correctly, instructions misaligned

**Solution:**
- Verify `get_inst_length()` function in neocore_pkg.sv
- Check all opcode/specifier combinations
- Ensure fetch_unit consumes correct number of bytes

**Debug:**
```systemverilog
$display("Opcode: 0x%02h, Specifier: 0x%02h, Length: %0d",
         opcode, specifier, inst_len);
```

#### 3. Hazard Detection False Positives

**Symptom:** Unnecessary stalls, poor performance

**Solution:**
- Check register address comparisons in hazard_unit.sv
- Verify R0 is excluded from hazard checks
- Confirm forwarding priority is correct

**Debug:**
```systemverilog
$display("Hazard: rs1=%h rd_ex=%h rd_mem=%h fwd_sel=%b",
         rs1_addr, ex_rd_addr, mem_rd_addr, fwd_rs1_sel);
```

#### 4. Dual-Issue Not Occurring

**Symptom:** dual_issue_active never asserts

**Solution:**
- Check issue_unit.sv restrictions
- Verify both instructions are valid
- Check for false RAW dependencies
- Confirm no structural hazards

**Debug:**
```systemverilog
$display("Issue: inst0_valid=%b inst1_valid=%b can_dual=%b",
         inst0_valid, inst1_valid, can_dual_issue);
```

#### 5. Memory Access Errors

**Symptom:** Incorrect load/store behavior

**Solution:**
- Verify address calculation
- Check big-endian data extraction in memory_stage.sv
- Confirm mem_size is correct (byte/halfword/word)
- Check unified_memory.sv byte lane selection

**Debug:**
```systemverilog
$display("MEM: addr=0x%08h wdata=0x%08h we=%b size=%b",
         mem_data_addr, mem_data_wdata, mem_data_we, mem_data_size);
```

### Waveform Viewing

Generate VCD file for waveform viewing:

```systemverilog
initial begin
  $dumpfile("core_tb.vcd");
  $dumpvars(0, core_tb);
end
```

View with GTKWave:
```bash
gtkwave core_tb.vcd
```

**Key Signals to Monitor:**
- PC progression
- Pipeline valid flags
- Instruction data through stages
- Hazard detection signals
- Forwarding select signals
- Memory interface transactions

---

## Performance Benchmarking

### Measuring IPC (Instructions Per Cycle)

```systemverilog
int total_cycles = 0;
int total_instructions = 0;

always @(posedge clk) begin
  if (!rst) begin
    total_cycles++;
    
    if (id_ex_reg_0.valid) total_instructions++;
    if (id_ex_reg_1.valid) total_instructions++;  // Dual-issue
  end
end

initial begin
  wait(core.halted);
  real ipc = real'(total_instructions) / real'(total_cycles);
  $display("Total Cycles: %0d", total_cycles);
  $display("Total Instructions: %0d", total_instructions);
  $display("IPC: %0.3f", ipc);
end
```

### Dual-Issue Rate

```systemverilog
int dual_issue_cycles = 0;
int total_issue_cycles = 0;

always @(posedge clk) begin
  if (!rst && (id_ex_reg_0.valid || id_ex_reg_1.valid)) begin
    total_issue_cycles++;
    if (id_ex_reg_0.valid && id_ex_reg_1.valid) begin
      dual_issue_cycles++;
    end
  end
end

initial begin
  wait(core.halted);
  real dual_rate = 100.0 * real'(dual_issue_cycles) / real'(total_issue_cycles);
  $display("Dual-Issue Rate: %0.1f%%", dual_rate);
end
```

### Branch Prediction Accuracy

(For future when branch prediction is added)

```systemverilog
int total_branches = 0;
int correct_predictions = 0;

always @(posedge clk) begin
  if (!rst && ex_mem_reg_0.is_branch) begin
    total_branches++;
    // Compare prediction vs actual
    if (predicted == ex_mem_reg_0.branch_taken) begin
      correct_predictions++;
    end
  end
end

initial begin
  wait(core.halted);
  real accuracy = 100.0 * real'(correct_predictions) / real'(total_branches);
  $display("Branch Prediction Accuracy: %0.1f%%", accuracy);
end
```

---

## Test Coverage

### Functional Coverage

| Feature | Test Coverage | Status |
|---------|---------------|--------|
| ALU Operations | All 9 operations | ✅ Complete |
| Branch Conditions | All 7 types | ✅ Complete |
| Memory Operations | Byte/halfword/word | ✅ Complete |
| Dual-Issue | Various combinations | ✅ Complete |
| Hazard Detection | RAW, WAW, load-use | ✅ Complete |
| Forwarding | All 6 sources | ✅ Complete |
| Pipeline Stalls | Load-use, conflicts | ✅ Complete |
| Branch Flush | Taken branches | ✅ Complete |

### Code Coverage

Use Verilator for code coverage analysis (future enhancement):

```bash
verilator --coverage --binary core_top.sv
./obj_dir/Vcore_top
verilator_coverage --annotate coverage_report coverage.dat
```

---

## Continuous Integration

### Automated Testing

Example CI script (.github/workflows/test.yml):

```yaml
name: RTL Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install Icarus Verilog
        run: sudo apt-get install -y iverilog
      - name: Run Tests
        run: cd sv && make all
```

---

## Conclusion

This testing infrastructure provides comprehensive verification of the NeoCore 16x32 CPU core. All unit tests pass, demonstrating correct implementation of individual components. Integration tests verify the complete system operates correctly with the Von Neumann architecture and big-endian memory model.

For further information, see:
- MODULE_DOCUMENTATION.md - Detailed module specifications
- ARCHITECTURE.md - Architecture deep dive
- README.md - Quick reference guide

