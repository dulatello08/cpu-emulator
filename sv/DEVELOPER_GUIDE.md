# NeoCore 16x32 CPU - Developer Guide

This guide provides step-by-step instructions for completing the CPU core implementation and extending it to full dual-issue capability.

## Quick Start

### Running Unit Tests

```bash
cd sv/
make clean
make all
```

This will compile and run all unit testbenches. You should see:
- ALU Testbench PASSED
- Register File Testbench PASSED
- Multiply Unit Testbench PASSED
- Branch Unit Testbench PASSED
- Decode Unit Testbench PASSED

### Project Structure

```
sv/
├── rtl/                    # RTL source files
│   ├── neocore_pkg.sv     # Package definitions (COMPLETE)
│   ├── alu.sv             # ALU (COMPLETE & TESTED)
│   ├── multiply_unit.sv   # Multiply unit (COMPLETE & TESTED)
│   ├── branch_unit.sv     # Branch unit (COMPLETE & TESTED)
│   ├── register_file.sv   # Register file (COMPLETE & TESTED)
│   ├── decode_unit.sv     # Decoder (COMPLETE & TESTED)
│   ├── fetch_unit.sv      # Fetch unit (COMPLETE, needs integration)
│   ├── pipeline_regs.sv   # Pipeline registers (COMPLETE)
│   ├── simple_memory.sv   # Memory model (COMPLETE, simulation only)
│   ├── hazard_unit.sv     # TODO: Hazard detection
│   ├── execute_stage.sv   # TODO: Execute stage integration
│   ├── memory_stage.sv    # TODO: Memory access
│   ├── writeback_stage.sv # TODO: Write-back
│   └── core_top.sv        # TODO: Top-level integration
├── tb/                     # Testbenches
│   ├── *_tb.sv            # Unit testbenches (COMPLETE)
│   └── core_smoke_tb.sv   # TODO: Integration test
├── mem/                    # Memory images
│   ├── test_simple.hex    # Simple test program
│   └── test_programs.txt  # Program documentation
├── scripts/
│   └── bin2hex.py         # Binary to hex converter
├── Makefile               # Build system
└── README.md              # Architecture documentation
```

## Completing the Single-Issue Pipeline

### Step 1: Implement Hazard Unit

Create `rtl/hazard_unit.sv`:

```systemverilog
module hazard_unit
  import neocore_pkg::*;
(
  // Current instruction in ID stage
  input  logic [3:0] id_rs1_addr,
  input  logic [3:0] id_rs2_addr,
  
  // Instructions in EX, MEM, WB stages
  input  logic [3:0] ex_rd_addr,
  input  logic       ex_rd_we,
  input  logic       ex_mem_read,    // Load instruction in EX
  
  input  logic [3:0] mem_rd_addr,
  input  logic       mem_rd_we,
  
  input  logic [3:0] wb_rd_addr,
  input  logic       wb_rd_we,
  
  // Forwarding control
  output logic [1:0] forward_a,      // 00=no fwd, 01=from MEM, 10=from WB
  output logic [1:0] forward_b,
  
  // Stall control
  output logic       stall_if,
  output logic       stall_id,
  output logic       flush_ex
);

  // Data hazard detection
  always_comb begin
    // Default: no forwarding
    forward_a = 2'b00;
    forward_b = 2'b00;
    
    // Forward from MEM stage
    if (mem_rd_we && (mem_rd_addr != 4'h0) && (mem_rd_addr == id_rs1_addr)) begin
      forward_a = 2'b01;
    end
    if (mem_rd_we && (mem_rd_addr != 4'h0) && (mem_rd_addr == id_rs2_addr)) begin
      forward_b = 2'b01;
    end
    
    // Forward from WB stage (if not already forwarding from MEM)
    if (wb_rd_we && (wb_rd_addr != 4'h0) && (wb_rd_addr == id_rs1_addr) && (forward_a == 2'b00)) begin
      forward_a = 2'b10;
    end
    if (wb_rd_we && (wb_rd_addr != 4'h0) && (wb_rd_addr == id_rs2_addr) && (forward_b == 2'b00)) begin
      forward_b = 2'b10;
    end
  end
  
  // Load-use hazard detection
  always_comb begin
    stall_if = 1'b0;
    stall_id = 1'b0;
    flush_ex = 1'b0;
    
    // If instruction in EX is a load and current instruction needs the result
    if (ex_mem_read) begin
      if ((ex_rd_addr == id_rs1_addr) || (ex_rd_addr == id_rs2_addr)) begin
        stall_if = 1'b1;
        stall_id = 1'b1;
        flush_ex = 1'b1;  // Insert bubble in EX
      end
    end
  end

endmodule
```

### Step 2: Implement Execute Stage

Create `rtl/execute_stage.sv`:

```systemverilog
module execute_stage
  import neocore_pkg::*;
(
  input  logic        clk,
  input  logic        rst,
  
  // From ID/EX register
  input  id_ex_t      id_ex,
  
  // Forwarding inputs
  input  logic [15:0] mem_fwd_data,
  input  logic [15:0] wb_fwd_data,
  input  logic [1:0]  forward_a,
  input  logic [1:0]  forward_b,
  
  // Current flags
  input  logic        z_flag,
  input  logic        v_flag,
  
  // Outputs to EX/MEM register
  output ex_mem_t     ex_mem
);

  // Forwarding MUXes for operands
  logic [15:0] operand_a, operand_b;
  
  always_comb begin
    case (forward_a)
      2'b00: operand_a = id_ex.rs1_data;
      2'b01: operand_a = mem_fwd_data;
      2'b10: operand_a = wb_fwd_data;
      default: operand_a = id_ex.rs1_data;
    endcase
    
    case (forward_b)
      2'b00: operand_b = id_ex.rs2_data;
      2'b01: operand_b = mem_fwd_data;
      2'b10: operand_b = wb_fwd_data;
      default: operand_b = id_ex.rs2_data;
    endcase
  end
  
  // ALU
  logic [31:0] alu_result;
  logic alu_z, alu_v;
  
  alu alu_inst (
    .clk(clk),
    .rst(rst),
    .operand_a(operand_a),
    .operand_b(id_ex.itype == ITYPE_ALU && id_ex.specifier == 8'h00 ? 
                id_ex.immediate[15:0] : operand_b),
    .alu_op(id_ex.alu_op),
    .result(alu_result),
    .z_flag(alu_z),
    .v_flag(alu_v)
  );
  
  // Branch unit
  logic branch_taken;
  logic [31:0] branch_pc;
  
  branch_unit branch_inst (
    .clk(clk),
    .rst(rst),
    .opcode(id_ex.opcode),
    .operand_a(operand_a),
    .operand_b(operand_b),
    .v_flag_in(v_flag),
    .branch_target(id_ex.immediate),
    .branch_taken(branch_taken),
    .branch_pc(branch_pc)
  );
  
  // Output assignment
  always_comb begin
    ex_mem.valid = id_ex.valid;
    ex_mem.pc = id_ex.pc;
    ex_mem.alu_result = alu_result;
    ex_mem.z_flag = alu_z;
    ex_mem.v_flag = alu_v;
    ex_mem.rd_addr = id_ex.rd_addr;
    ex_mem.rd2_addr = id_ex.rd2_addr;
    ex_mem.rd_we = id_ex.rd_we;
    ex_mem.rd2_we = id_ex.rd2_we;
    ex_mem.mem_read = id_ex.mem_read;
    ex_mem.mem_write = id_ex.mem_write;
    ex_mem.mem_size = id_ex.mem_size;
    ex_mem.mem_addr = id_ex.immediate;  // For load/store
    ex_mem.mem_wdata = operand_a[15:0]; // Data to store
    ex_mem.branch_taken = id_ex.is_branch && branch_taken;
    ex_mem.branch_target = branch_pc;
    ex_mem.is_halt = id_ex.is_halt;
  end

endmodule
```

### Step 3: Test Integration

After implementing hazard_unit and execute_stage:

1. Create testbench for hazard detection
2. Create testbench for execute stage
3. Fix any compilation errors
4. Verify forwarding works correctly

### Step 4: Implement Memory and Writeback Stages

These are simpler - mostly just connecting signals:

- `memory_stage.sv`: Interface with memory, handle load/store
- `writeback_stage.sv`: Select result to write back, update register file

### Step 5: Integrate into core_top.sv

Connect all stages with pipeline registers, wire up hazard unit.

### Step 6: End-to-End Testing

Create `tb/core_smoke_tb.sv` to run actual programs.

## Extending to Dual-Issue

### Additional Modules Needed

1. **issue_unit.sv** - Decides which instructions can issue together
2. Enhanced **hazard_unit.sv** - Checks hazards between both instructions
3. **Dual instruction decoders** - Decode both instructions in parallel

### Dual-Issue Rules to Implement

```systemverilog
// In issue_unit.sv
always_comb begin
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
  
  // Rule 3: No write port conflicts
  if (inst0_rd_we && inst1_rd_we && (inst0_rd_addr == inst1_rd_addr)) begin
    can_dual_issue = 1'b0;
  end
  
  // Rule 4: Data dependency between instructions
  if (inst0_rd_we && ((inst0_rd_addr == inst1_rs1_addr) || 
                      (inst0_rd_addr == inst1_rs2_addr))) begin
    can_dual_issue = 1'b0;
  end
end
```

## Debugging Tips

1. **Use waveforms**: Run with `iverilog -DVCD` to generate VCD files
2. **Add assertions**: Liberal use of `assert` helps catch bugs early
3. **Test incrementally**: Don't integrate everything at once
4. **Compare with C emulator**: Run same program, compare register state

## Common Pitfalls

1. **Forwarding timing**: Ensure forwarded data arrives before it's needed
2. **Load-use hazards**: These MUST stall - no forwarding possible
3. **Branch flush**: Remember to flush IF and ID when branch taken
4. **Register 0**: Many ISAs treat R0 as always-zero, watch for this

## Resources

- Parent directory C emulator: `../emulator.c`, `../execute_instructions.c`
- ISA documentation: `../Instructions.md`
- Package definitions: `rtl/neocore_pkg.sv`
- This README: `README.md`

Good luck! The foundation is solid - the remaining work is mostly integration and testing.
