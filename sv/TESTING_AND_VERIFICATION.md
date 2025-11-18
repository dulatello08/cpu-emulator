# NeoCore16x32 Testing and Verification Guide

## Overview

The NeoCore16x32 CPU is verified through a comprehensive suite of testbenches that validate individual modules and the integrated system. This document describes the test strategy, testbench structure, and verification procedures.

## Test Strategy

### Verification Levels

The verification approach follows a hierarchical strategy:

1. **Unit Tests**: Individual module verification
2. **Integration Tests**: Pipeline stage integration
3. **System Tests**: Full CPU with memory
4. **Functional Tests**: ISA compliance verification

### Coverage Goals

- **Instruction Coverage**: All 26 opcodes exercised
- **Addressing Mode Coverage**: All MOV specifiers tested
- **Pipeline Coverage**: All pipeline stages and hazards tested
- **Dual-Issue Coverage**: Various dual-issue scenarios
- **Edge Cases**: Boundary conditions, corner cases

---

## Testbench Structure

All testbenches follow a common pattern:

```verilog
module module_tb;
  // Clock and reset
  logic clk, rst;
  
  // DUT signals
  // ... (module-specific)
  
  // DUT instantiation
  module_name dut (...);
  
  // Clock generation (100 MHz)
  initial begin
    clk = 0;
    forever #5 clk = ~clk;
  end
  
  // Test stimulus
  initial begin
    // Reset sequence
    rst = 1;
    #20 rst = 0;
    
    // Test cases
    // ...
    
    // Finish
    #1000 $finish;
  end
  
  // Waveform dump
  initial begin
    $dumpfile("module_tb.vcd");
    $dumpvars(0, module_tb);
  end
  
  // Monitoring/checking
  always @(posedge clk) begin
    if (!rst) begin
      // Assertions and checks
    end
  end
endmodule
```

---

## Unit Testbenches

### ALU Testbench (`alu_tb.sv`)

**Purpose**: Verify all ALU operations

**Test Cases**:
1. **Addition**: Test various operand combinations, overflow detection
2. **Subtraction**: Test clamping behavior (result >= 0)
3. **Multiplication**: Test truncation to 16 bits
4. **Bitwise Operations**: AND, OR, XOR with various patterns
5. **Shifts**: LSH and RSH with different shift amounts
6. **Flag Generation**: Z and V flags for all operations

**Example Test**:
```verilog
// Test ADD operation
operand_a = 16'h1000;
operand_b = 16'h2000;
alu_op = ALU_ADD;
#10;
assert(result == 32'h0000_3000);
assert(z_flag == 1'b0);
assert(v_flag == 1'b0);

// Test overflow
operand_a = 16'hFFFF;
operand_b = 16'h0002;
alu_op = ALU_ADD;
#10;
assert(result == 32'h0001_0001);  // Overflow
assert(v_flag == 1'b1);
```

**Running**:
```bash
make alu_test
gtkwave alu_tb.vcd &
```

**Expected Results**:
- All ADD, SUB, MUL operations produce correct results
- Overflow flag sets correctly
- Zero flag sets when result is zero
- No X or Z values in outputs

---

### Multiply Unit Testbench (`multiply_unit_tb.sv`)

**Purpose**: Verify signed and unsigned multiplication

**Test Cases**:
1. **Unsigned Multiply**: Various positive operands
2. **Signed Multiply**: Positive, negative, and mixed signs
3. **Edge Cases**: Zero, maximum values
4. **Result Splitting**: Verify low/high result correctness

**Example Test**:
```verilog
// Unsigned multiply
operand_a = 16'h0100;
operand_b = 16'h0200;
is_signed = 1'b0;
#10;
assert(result_lo == 16'h0000);  // 0x020000 & 0xFFFF
assert(result_hi == 16'h0002);  // 0x020000 >> 16

// Signed multiply (negative * positive)
operand_a = 16'hFFFF;  // -1
operand_b = 16'h0002;  // +2
is_signed = 1'b1;
#10;
assert({result_hi, result_lo} == 32'hFFFF_FFFE);  // -2
```

**Running**:
```bash
make mul_test
```

---

### Decode Unit Testbench (`decode_unit_tb.sv`)

**Purpose**: Verify instruction decoding for all opcodes and specifiers

**Test Cases**:
1. **ALU Instructions**: All specifiers (immediate, register, memory)
2. **MOV Instructions**: All 19 specifiers
3. **Branch Instructions**: All 6 types
4. **Stack/Subroutine**: PSH, POP, JSR, RTS
5. **Control**: NOP, HLT, WFI, ENI, DSI

**Example Test**:
```verilog
// Test ADD immediate (specifier 0x00)
inst_data = {
  8'h00,   // Specifier
  8'h01,   // Opcode (ADD)
  8'h05,   // rd = R5
  8'h12,   // Immediate MSB
  8'h34,   // Immediate LSB
  {99{8'h00}}  // Padding
};
inst_len = 4'd5;
pc = 32'h1000;
valid_in = 1'b1;
#10;

assert(opcode == OP_ADD);
assert(rd_addr == 4'h5);
assert(immediate == 32'h0000_1234);
assert(rd_we == 1'b1);
assert(alu_op == ALU_ADD);
```

**Running**:
```bash
make decode_test
```

---

### Branch Unit Testbench (`branch_unit_tb.sv`)

**Purpose**: Verify branch condition evaluation

**Test Cases**:
1. **Unconditional**: B, JSR always taken
2. **Equal/Not Equal**: BE, BNE with various operands
3. **Less Than**: BLT unsigned comparison
4. **Greater Than**: BGT unsigned comparison
5. **Overflow**: BRO with V flag set/clear

**Example Test**:
```verilog
// Test BE (branch if equal)
opcode = OP_BE;
operand_a = 16'h1234;
operand_b = 16'h1234;
branch_target = 32'h0000_2000;
#10;
assert(branch_taken == 1'b1);
assert(branch_pc == 32'h0000_2000);

// Test BNE (branch if not equal)
opcode = OP_BNE;
operand_a = 16'h1234;
operand_b = 16'h5678;
#10;
assert(branch_taken == 1'b1);

// Not taken case
operand_a = 16'h1234;
operand_b = 16'h1234;
#10;
assert(branch_taken == 1'b0);
```

**Running**:
```bash
make branch_test
```

---

### Register File Testbench (`register_file_tb.sv`)

**Purpose**: Verify register file read/write and forwarding

**Test Cases**:
1. **Single Write**: Write to register, read back
2. **Dual Write**: Two simultaneous writes to different registers
3. **Internal Forwarding**: Write and read same register in same cycle
4. **Reset**: All registers initialized to zero

**Example Test**:
```verilog
// Test single write and read
rd_addr_0 = 4'd3;
rd_data_0 = 16'h1234;
rd_we_0 = 1'b1;
#10;

rs1_addr_0 = 4'd3;
#1;  // Combinational read
assert(rs1_data_0 == 16'h1234);

// Test internal forwarding
rd_addr_0 = 4'd5;
rd_data_0 = 16'hABCD;
rd_we_0 = 1'b1;
rs1_addr_0 = 4'd5;
#1;
assert(rs1_data_0 == 16'hABCD);  // Forwarded, not old value
```

**Running**:
```bash
make regfile_test
```

---

## Integration Testbenches

### Core Unified Testbench (`core_unified_tb.sv`)

**Purpose**: Full CPU verification with unified memory

**Test Components**:
1. **Core Instance**: Complete core_top with all pipeline stages
2. **Unified Memory**: Von Neumann memory with dual ports
3. **Program Loading**: Test programs loaded into memory
4. **Monitoring**: PC, pipeline status, register values

**Test Programs**:

**Program 1: Simple ALU Sequence**
```assembly
; Test ADD, SUB, basic operations
NOP
ADD R1, #0x0005      ; R1 = 5
ADD R2, #0x0003      ; R2 = 3
ADD R1, R2           ; R1 = R1 + R2 = 8
SUB R1, #0x0002      ; R1 = R1 - 2 = 6
HLT
```

**Program 2: Memory Operations**
```assembly
; Test MOV with memory
MOV R1, #0x1234
MOV [0x2000], R1     ; Store R1 to memory
MOV R2, [0x2000]     ; Load from memory to R2
; R2 should equal 0x1234
HLT
```

**Program 3: Branch Test**
```assembly
; Test conditional branch
MOV R1, #0x0005
MOV R2, #0x0005
BE R1, R2, equal_label   ; Should branch
ADD R3, #0x0001          ; Skipped
HLT

equal_label:
ADD R4, #0x0001          ; Executed
HLT
```

**Program 4: Dual-Issue Test**
```assembly
; Independent operations that should dual-issue
ADD R1, #0x0001
ADD R2, #0x0002   ; Can dual-issue with above
SUB R3, #0x0001
SUB R4, #0x0002   ; Can dual-issue with above
HLT
```

**Testbench Structure**:
```verilog
module core_unified_tb;
  // Instantiate core and memory
  unified_memory memory (...);
  core_top dut (...);
  
  // Load program
  initial begin
    // NOP
    memory.mem[0] = 8'h00;
    memory.mem[1] = 8'h00;
    
    // ADD R1, #0x0005
    memory.mem[2] = 8'h00;  // Specifier
    memory.mem[3] = 8'h01;  // Opcode (ADD)
    memory.mem[4] = 8'h01;  // rd = R1
    memory.mem[5] = 8'h00;  // Immediate MSB
    memory.mem[6] = 8'h05;  // Immediate LSB
    
    // ... rest of program
  end
  
  // Monitor execution
  always @(posedge clk) begin
    $display("Cycle %d: PC=0x%h, dual_issue=%b", 
             cycle_count, current_pc, dual_issue_active);
  end
  
  // Check final state
  initial begin
    #10000;  // Run for sufficient cycles
    
    // Verify results
    assert(dut.regfile.registers[1] == expected_r1);
    assert(dut.regfile.registers[2] == expected_r2);
    
    $display("Test PASSED");
    $finish;
  end
endmodule
```

**Running**:
```bash
make sim
gtkwave core_unified_tb.vcd &
```

**Expected Behavior**:
- Core fetches instructions starting from address 0
- Pipeline progresses through IF, ID, EX, MEM, WB stages
- Dual-issue occurs when instructions are independent
- Branches resolve in EX stage with 2-cycle penalty
- HLT stops execution
- No X or Z values in critical paths

**Verification Checklist**:
- [ ] All instructions fetch correctly
- [ ] Dual-issue occurs for independent instructions
- [ ] Branches take correct path
- [ ] Memory loads/stores work with big-endian ordering
- [ ] Register file updates correctly
- [ ] Hazard detection prevents incorrect forwarding
- [ ] CPU halts on HLT instruction

---

## Verification Procedures

### Pre-Commit Testing

Before committing changes:

```bash
# Clean previous builds
make clean

# Run all unit tests
make all_tests

# Check for simulation errors
grep -i error *.log

# Run integration test
make sim

# Verify no undefined signals
grep -i 'X' core_unified_tb.vcd
```

### Regression Testing

For major changes, run full regression suite:

```bash
#!/bin/bash
# regression.sh

echo "Running NeoCore16x32 Regression Suite"
echo "======================================"

# Unit tests
for test in alu mul decode branch regfile; do
  echo "Running ${test}_test..."
  make ${test}_test || exit 1
done

# Integration tests
echo "Running core integration test..."
make sim || exit 1

# Check for warnings
echo "Checking for warnings..."
grep -i warning *.log

echo "Regression PASSED"
```

### Coverage Analysis

**Manual Coverage Tracking**:
- Create checklist of all instructions
- Mark each as tested when verified
- Track dual-issue combinations tested
- Document edge cases covered

**Example Coverage Matrix**:

| Instruction | Unit Test | Integration Test | Edge Cases | Status |
|-------------|-----------|------------------|------------|--------|
| NOP | ✓ | ✓ | - | Complete |
| ADD imm | ✓ | ✓ | Overflow | Complete |
| ADD reg | ✓ | ✓ | Zero result | Complete |
| ADD mem | ✓ | ✓ | Unaligned | Partial |
| ... | | | | |

---

## Known Test Limitations

### Current Gaps

1. **Interrupt Testing**: WFI, ENI, DSI not fully functional (placeholders)
2. **Stack Testing**: Limited JSR/RTS depth testing
3. **Memory Boundary**: Large memory accesses near 64 KB boundary not exhaustively tested
4. **Timing**: No timing-critical path validation in simulation

### Future Test Additions

1. **Stress Testing**: Long-running programs, deep loops
2. **Random Testing**: Random instruction sequences
3. **Coverage Tools**: Integrate with commercial/open-source coverage tools
4. **Formal Verification**: Properties and assertions for key behaviors

---

## Debugging Failed Tests

### Simulation Failures

**Symptom**: Assertion fails in testbench

**Debug Steps**:
1. Note failing assertion and time
2. Open waveform at that time
3. Trace signals backward to find root cause
4. Check for X/Z values in data path
5. Verify pipeline state is correct
6. Add $display statements for more visibility

**Example**:
```verilog
// Failed assertion
assert(result == expected);

// Add debug
$display("DEBUG: operand_a=%h, operand_b=%h, alu_op=%h, result=%h, expected=%h",
         operand_a, operand_b, alu_op, result, expected);
```

### Waveform Analysis Tips

1. **Use markers**: Set markers at key events (instruction boundaries, hazards)
2. **Color-code**: Use signal coloring for different pipeline stages
3. **Search signals**: Use Ctrl+F to find signal transitions
4. **Time cursor**: Click to see all signal values at that time
5. **Zoom to fit**: After selecting interesting region

### Common Pitfalls

**Big-Endian Confusion**:
- **Symptom**: Memory values appear byte-swapped
- **Fix**: Verify byte order in instruction encoding
- **Check**: MSB should be at lower address

**Pipeline State**:
- **Symptom**: Incorrect results appear one cycle late
- **Fix**: Remember pipeline registers introduce latency
- **Check**: Trace through pipeline stages

**Forwarding Issues**:
- **Symptom**: Stale register values used
- **Fix**: Verify forwarding paths and priority
- **Check**: Forwarding MUX selects correct source

---

## Test Results Documentation

### Test Report Template

```
NeoCore16x32 Test Report
Date: [DATE]
Tester: [NAME]
Revision: [GIT_HASH]

Unit Tests:
- ALU: PASS (15/15 test cases)
- Multiply: PASS (10/10 test cases)
- Decode: PASS (26/26 opcodes)
- Branch: PASS (6/6 branch types)
- Register File: PASS (8/8 test cases)

Integration Tests:
- Core Unified: PASS
  - Program 1 (ALU sequence): PASS
  - Program 2 (Memory ops): PASS
  - Program 3 (Branches): PASS
  - Program 4 (Dual-issue): PASS

Issues Found:
- [List any issues discovered]

Overall Status: PASS/FAIL
```

---

## Continuous Integration

### Automated Testing

**Example GitHub Actions Workflow**:
```yaml
name: NeoCore16x32 CI

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Install Icarus Verilog
      run: sudo apt-get install -y iverilog
    
    - name: Run Tests
      run: |
        cd sv
        make all_tests
    
    - name: Check for Errors
      run: |
        if grep -i error sv/*.log; then
          exit 1
        fi
```

---

## Summary

The NeoCore16x32 verification strategy ensures:

1. **Comprehensive Coverage**: All modules and instructions tested
2. **Hierarchical Approach**: Unit → Integration → System testing
3. **Regression Safety**: Automated test suite catches breakage
4. **Big-Endian Verification**: Byte ordering tested throughout
5. **Pipeline Verification**: All hazards and forwarding paths validated

All testbenches are located in `sv/tb/` and can be run individually or as a suite using the Makefile. Waveforms provide detailed visibility into CPU behavior for debugging and verification.

