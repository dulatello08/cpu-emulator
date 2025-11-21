# NeoCore16x32 Developer Guide

## Introduction

This guide provides information for developers working on the NeoCore16x32 CPU RTL implementation. It covers the development workflow, coding standards, simulation procedures, synthesis guidelines, and debugging techniques.

## Development Environment Setup

### Required Tools

**Simulation**:
- **Icarus Verilog** (iverilog) 11.0 or later
- **GTKWave** for waveform viewing

**Synthesis** (for FPGA):
- **Yosys** 0.9 or later (open-source synthesis)
- **nextpnr-ecp5** for Lattice ECP5 place-and-route
- **ecppack** for bitstream generation
- **openFPGALoader** or **ujprog** for programming ULX3S board

**Optional**:
- **Verilator** for fast cycle-accurate simulation
- **Visual Studio Code** with SystemVerilog extension

### Directory Structure

```
sv/
├── rtl/                  # RTL source files
│   ├── neocore_pkg.sv        # Package definitions
│   ├── core_top.sv           # Top-level integration
│   ├── fetch_unit.sv
│   ├── decode_unit.sv
│   ├── execute_stage.sv
│   ├── memory_stage.sv
│   ├── writeback_stage.sv
│   ├── alu.sv
│   ├── multiply_unit.sv
│   ├── branch_unit.sv
│   ├── hazard_unit.sv
│   ├── issue_unit.sv
│   ├── register_file.sv
│   ├── unified_memory.sv
│   └── pipeline_regs.sv
├── tb/                   # Testbenches
│   ├── core_unified_tb.sv
│   ├── alu_tb.sv
│   ├── multiply_unit_tb.sv
│   ├── decode_unit_tb.sv
│   ├── branch_unit_tb.sv
│   └── register_file_tb.sv
├── mem/                  # Memory initialization files
│   └── test_simple.hex
├── scripts/              # Build and simulation scripts
├── Makefile              # Build automation
└── docs/                 # This documentation
```

---

## Building and Simulation

### Makefile Targets

The provided Makefile automates common tasks:

```bash
make sim              # Run core integration test
make alu_test         # Run ALU unit test
make mul_test         # Run multiplier test
make decode_test      # Run decoder test
make branch_test      # Run branch unit test
make regfile_test     # Run register file test
make all_tests        # Run all testbenches
make wave             # Open GTKWave with last simulation
make clean            # Remove build artifacts
```

### Running Individual Tests

**ALU Test**:
```bash
cd sv
iverilog -g2012 -o alu_test.vvp rtl/neocore_pkg.sv rtl/alu.sv tb/alu_tb.sv
vvp alu_test.vvp
```

**Core Integration Test**:
```bash
iverilog -g2012 -o core_test.vvp \
  rtl/neocore_pkg.sv \
  rtl/*.sv \
  tb/core_unified_tb.sv
vvp core_test.vvp
gtkwave core_unified_tb.vcd &
```

### Waveform Analysis

**GTKWave Tips**:
1. **Add signals**: Drag from SST (Signal Search Tree) to wave window
2. **Group signals**: Right-click → Insert → Comment to add headers
3. **Zoom**: Use Zoom Fit or Zoom In/Out buttons
4. **Time cursor**: Click in wave area, values update in signal list
5. **Save layout**: File → Write Save File (load with -a option)

**Recommended Signal Groups**:
- **Clock/Reset**: clk, rst
- **PC/Fetch**: current_pc, mem_if_addr, mem_if_rdata, fetch_valid_0, fetch_valid_1
- **Decode**: decode_opcode_0, decode_rd_addr_0, decode_valid_0
- **Execute**: alu_result_0, branch_taken
- **Memory**: mem_data_addr, mem_data_rdata, mem_data_wdata
- **Writeback**: rf_wr_addr_0, rf_wr_data_0, rf_wr_en_0
- **Pipeline Control**: stall_pipeline, dual_issue_active, halted

---

## Coding Standards

### SystemVerilog Style Guide

**Module Declaration**:
```verilog
module module_name
  import neocore_pkg::*;  // Import package if needed
(
  input  logic        clk,
  input  logic        rst,
  input  logic [15:0] data_in,
  output logic [15:0] data_out
);
```

**Naming Conventions**:
- **Modules**: `snake_case` (e.g., `fetch_unit`, `execute_stage`)
- **Signals**: `snake_case` (e.g., `alu_result`, `branch_taken`)
- **Parameters**: `UPPER_CASE` (e.g., `MEM_SIZE_BYTES`)
- **Types**: `snake_case_t` (e.g., `if_id_t`, `opcode_e`)
- **Active-low signals**: `_n` suffix (e.g., `rst_n`)

**Indentation**:
- 2 spaces per level
- No tabs
- Align signal names in port lists

**Comments**:
```verilog
// Single-line comment for brief explanations

// Multi-line comment for longer explanations
// that span multiple lines

/* Block comment
 * for very long descriptions
 * or section headers
 */
```

**always Blocks**:
```verilog
// Combinational logic
always_comb begin
  result = operand_a + operand_b;
end

// Sequential logic (registers)
always_ff @(posedge clk) begin
  if (rst) begin
    counter <= 32'h0;
  end else begin
    counter <= counter + 1;
  end
end
```

### Big-Endian Consistency

**All multi-byte values must be big-endian**:

```verilog
// CORRECT: MSB at lower bit position
logic [31:0] address = {byte0, byte1, byte2, byte3};
//                      MSB              LSB

// INCORRECT: Little-endian (don't use)
logic [31:0] address = {byte3, byte2, byte1, byte0};
```

**Memory Access**:
```verilog
// CORRECT: MSB first
mem[addr + 0] <= data[15:8];  // MSB
mem[addr + 1] <= data[7:0];   // LSB

// INCORRECT: LSB first (don't use)
mem[addr + 0] <= data[7:0];   // Wrong!
mem[addr + 1] <= data[15:8];  // Wrong!
```

---

## Adding New Instructions

To add a new instruction to the ISA:

### Step 1: Define Opcode

Edit `rtl/neocore_pkg.sv`:

```verilog
typedef enum logic [7:0] {
  OP_NOP   = 8'h00,
  OP_ADD   = 8'h01,
  // ... existing opcodes ...
  OP_DSI   = 8'h19,
  OP_NEWINST = 8'h1A  // Add new opcode
} opcode_e;
```

### Step 2: Define Instruction Type

Still in `neocore_pkg.sv`:

```verilog
function automatic itype_e get_itype(opcode_e op);
  case (op)
    OP_ADD, OP_SUB, ...: return ITYPE_ALU;
    OP_NEWINST: return ITYPE_ALU;  // Or appropriate type
    // ...
  endcase
endfunction
```

### Step 3: Define Instruction Length

In `neocore_pkg.sv`, update `get_inst_length()`:

```verilog
function automatic logic [3:0] get_inst_length(logic [7:0] opcode, logic [7:0] specifier);
  case (opcode)
    // ... existing cases ...
    8'h1A: return 4'd4;  // NEWINST is 4 bytes
    // ...
  endcase
endfunction
```

### Step 4: Update Decoder

Edit `rtl/decode_unit.sv` to handle the new instruction:

```verilog
case (opcode_int)
  // ... existing opcodes ...
  OP_NEWINST: begin
    rd_addr = byte2[3:0];
    rs1_addr = byte3[3:0];
    rd_we = 1'b1;
    // Set other control signals as needed
  end
endcase
```

### Step 5: Update Execute Stage

If the instruction requires new execution logic, update `rtl/execute_stage.sv` or add new execution unit.

### Step 6: Add Tests

Create testbench for new instruction in `tb/`:

```verilog
initial begin
  // Test NEWINST
  // Load instruction into memory
  // Run simulation
  // Check results
end
```

---

## Debugging Techniques

### Common Issues and Solutions

**Issue**: Instruction not fetching correctly

**Debug Steps**:
1. Check `mem_if_addr` - is it pointing to correct address?
2. Check `mem_if_rdata` - does it contain expected instruction bytes?
3. Check `fetch_buffer` in fetch_unit - is buffer managing correctly?
4. Verify instruction length calculation in `get_inst_length()`

**Issue**: Dual-issue not working

**Debug Steps**:
1. Check `issue_inst0` and `issue_inst1` signals
2. Check `dual_issue` output from issue_unit
3. Examine issue_unit hazard detection logic
4. Verify no data dependencies between instructions
5. Check that neither instruction is a branch or memory operation

**Issue**: Wrong ALU result

**Debug Steps**:
1. Check `operand_a` and `operand_b` inputs to ALU
2. Verify `alu_op` signal matches expected operation
3. Check forwarding: is `forward_a` or `forward_b` selecting wrong source?
4. Trace operand back through pipeline to register file read

**Issue**: Memory access fails

**Debug Steps**:
1. Check `mem_data_addr` - correct address?
2. Check `mem_data_req` - is request being made?
3. Check `mem_data_ack` - is memory responding?
4. Verify `mem_data_size` matches expected access size
5. For writes, check `mem_data_we` is asserted

### Simulation Debug Flags

**Verbose Output**:
```verilog
initial begin
  $display("Starting simulation at time %t", $time);
end

always @(posedge clk) begin
  if (debug_enable)
    $display("Cycle %d: PC=0x%h, opcode=0x%h", cycle_count, current_pc, decode_opcode_0);
end
```

**Assertion Checking**:
```verilog
// Check for invalid opcode
always @(posedge clk) begin
  if (decode_valid_0 && decode_itype_0 == ITYPE_INVALID) begin
    $error("Invalid opcode detected: 0x%h at PC 0x%h", decode_opcode_0, if_id_out_0.pc);
    $finish;
  end
end
```

---

## FPGA Synthesis

### Synthesis for Lattice ECP5

**Using Yosys**:
```bash
yosys -p "synth_ecp5 -top core_top -json core_top.json" rtl/*.sv
```

**Place and Route with nextpnr**:
```bash
nextpnr-ecp5 --85k --package CABGA381 --json core_top.json --lpf ulx3s.lpf --textcfg core_top.config
```

**Generate Bitstream**:
```bash
ecppack core_top.config core_top.bit
```

**Program FPGA**:
```bash
ujprog core_top.bit
```

### Resource Utilization

Expected resource usage on Lattice ECP5-85F:

| Resource | Used | Available | Percentage |
|----------|------|-----------|------------|
| **LUTs** | ~8,000 | 83,920 | ~10% |
| **Registers** | ~3,000 | 83,920 | ~4% |
| **BRAM** | ~64 KB | 208 KB | ~31% |
| **DSPs** | 2 | 4 | 50% |

**Note**: Actual usage depends on synthesis options and optimizations.

### Timing Constraints

**Clock Period**:
- Target: 100 MHz (10 ns period)
- Critical path: ~8-9 ns (fetch → decode → execute)

**Constraint File** (ulx3s.lpf):
```
LOCATE COMP "clk" SITE "G2";
FREQUENCY PORT "clk" 100 MHz;
```

---

## Testing Guidelines

### Unit Test Best Practices

**Test Structure**:
```verilog
module module_tb;
  // 1. Declare signals
  logic clk, rst;
  logic [15:0] data_in, data_out;
  
  // 2. Instantiate DUT
  module_name dut (
    .clk(clk),
    .rst(rst),
    .data_in(data_in),
    .data_out(data_out)
  );
  
  // 3. Clock generation
  initial begin
    clk = 0;
    forever #5 clk = ~clk;  // 100 MHz
  end
  
  // 4. Test stimulus
  initial begin
    // Reset
    rst = 1;
    data_in = 16'h0;
    #20 rst = 0;
    
    // Test case 1
    #10 data_in = 16'h1234;
    #10 assert(data_out == expected_value);
    
    // Test case 2
    // ...
    
    // Finish
    #100 $finish;
  end
  
  // 5. Waveform dump
  initial begin
    $dumpfile("module_tb.vcd");
    $dumpvars(0, module_tb);
  end
endmodule
```

### Coverage Goals

Aim for:
- **Statement coverage**: 100% of RTL lines executed
- **Branch coverage**: All if/else and case branches taken
- **Toggle coverage**: All signals toggle at least once
- **Functional coverage**: All instruction types tested

### Continuous Integration

**Regression Testing**:
```bash
#!/bin/bash
# run_tests.sh

echo "Running NeoCore16x32 test suite..."

make clean
make all_tests || exit 1

echo "All tests passed!"
```

---

## Performance Optimization

### Improving IPC

**Reduce Dependencies**:
- Schedule independent instructions together
- Use different register banks for parallel operations
- Avoid load-use hazards (insert independent instructions after loads)

**Optimize Memory Access**:
- Batch memory operations
- Use register temporaries
- Minimize store-load forwarding

**Branch Optimization**:
- Reduce branch frequency
- Use conditional moves instead of branches where possible
- Ensure branch targets are aligned

### Code Example

**Unoptimized** (IPC ≈ 1.0):
```assembly
MOV R1, [0x1000]   ; Load
ADD R2, R1         ; Load-use hazard (stall)
MOV R3, [0x1004]   ; Load
SUB R4, R3         ; Load-use hazard (stall)
```

**Optimized** (IPC ≈ 1.5):
```assembly
MOV R1, [0x1000]   ; Load
MOV R3, [0x1004]   ; Load (dual-issue with above)
NOP                ; Give time for loads
ADD R2, R1         ; Use R1 (no hazard)
SUB R4, R3         ; Use R3 (dual-issue)
```

---

## Contributing

### Code Review Checklist

Before submitting changes:

- [ ] Code follows style guide
- [ ] All new instructions documented in ISA_REFERENCE.md
- [ ] Unit tests added for new features
- [ ] All existing tests pass
- [ ] Simulation runs without errors/warnings
- [ ] Synthesis completes successfully (if applicable)
- [ ] Big-endian consistency maintained
- [ ] Pipeline behavior verified
- [ ] Commit messages are clear and descriptive

### Commit Message Format

```
Short summary (50 chars or less)

Longer description if needed. Explain what changed and why.
Include any relevant issue numbers.

- Bullet points for multiple changes
- Keep lines under 72 characters
```

---

## Troubleshooting

### Simulation Issues

**Problem**: Icarus Verilog errors about SystemVerilog features

**Solution**: Ensure you're using iverilog 11.0+ with `-g2012` flag

**Problem**: Undefined symbols or package errors

**Solution**: Ensure `neocore_pkg.sv` is compiled first and imported correctly

### Synthesis Issues

**Problem**: Yosys can't infer BRAM

**Solution**: Ensure memory array uses:
- Byte array `logic [7:0] mem [0:SIZE-1]`
- Sequential read (registered output)
- Simple address indexing

**Problem**: Timing violations

**Solution**:
- Add pipeline stages to critical paths
- Reduce clock frequency target
- Optimize combinational logic depth

---

## Resources

- **NeoCore16x32 Documentation**: See other .md files in this directory
- **Icarus Verilog**: http://iverilog.icarus.com/
- **GTKWave**: http://gtkwave.sourceforge.net/
- **Yosys**: http://www.clifford.at/yosys/
- **ULX3S Board**: https://github.com/emard/ulx3s

For questions or issues, consult the repository issue tracker.

