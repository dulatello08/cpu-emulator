//
// core_advanced_tb.sv
// Advanced testbench for NeoCore 16x32 CPU
// Tests more complex scenarios: hazards, branches, load-use dependencies
//

`timescale 1ns/1ps

module core_advanced_tb;
  import neocore_pkg::*;

  // Testbench signals
  logic        clk;
  logic        rst;
  
  // Unified memory interface signals
  logic [31:0]  mem_if_addr;
  logic         mem_if_req;
  logic [127:0] mem_if_rdata;
  logic         mem_if_ack;
  logic [31:0]  mem_data_addr;
  logic [31:0]  mem_data_wdata;
  logic [1:0]   mem_data_size;
  logic         mem_data_we;
  logic         mem_data_req;
  logic [31:0]  mem_data_rdata;
  logic         mem_data_ack;
  
  logic        halted;
  logic [31:0] current_pc;
  logic        dual_issue_active;
  
  // Unified memory instance
  unified_memory #(
    .MEM_SIZE_BYTES(65536),
    .ADDR_WIDTH(32)
  ) memory (
    .clk(clk),
    .rst(rst),
    .if_addr(mem_if_addr),
    .if_req(mem_if_req),
    .if_rdata(mem_if_rdata),
    .if_ack(mem_if_ack),
    .data_addr(mem_data_addr),
    .data_wdata(mem_data_wdata),
    .data_size(mem_data_size),
    .data_we(mem_data_we),
    .data_req(mem_data_req),
    .data_rdata(mem_data_rdata),
    .data_ack(mem_data_ack)
  );
  
  // Core instance
  core_top dut (
    .clk(clk),
    .rst(rst),
    .mem_if_addr(mem_if_addr),
    .mem_if_req(mem_if_req),
    .mem_if_rdata(mem_if_rdata),
    .mem_if_ack(mem_if_ack),
    .mem_data_addr(mem_data_addr),
    .mem_data_wdata(mem_data_wdata),
    .mem_data_size(mem_data_size),
    .mem_data_we(mem_data_we),
    .mem_data_req(mem_data_req),
    .mem_data_rdata(mem_data_rdata),
    .mem_data_ack(mem_data_ack),
    .halted(halted),
    .current_pc(current_pc),
    .dual_issue_active(dual_issue_active)
  );
  
  // Clock generation (100 MHz)
  initial begin
    clk = 0;
    forever #5 clk = ~clk;
  end
  
  // Cycle counter
  int cycle_count;
  int dual_issue_count;
  int branch_count;
  
  always_ff @(posedge clk) begin
    if (rst) begin
      cycle_count <= 0;
      dual_issue_count <= 0;
      branch_count <= 0;
    end else begin
      cycle_count <= cycle_count + 1;
      if (dual_issue_active) begin
        dual_issue_count <= dual_issue_count + 1;
      end
      if (dut.branch_taken) begin
        branch_count <= branch_count + 1;
      end
    end
  end
  
  // Debug monitoring
  always @(posedge clk) begin
    if (!rst && cycle_count < 30) begin
      $display("Cycle %3d: PC=0x%08h Halt=%b DualIssue=%b Valid0=%b Valid1=%b Len0=%d Len1=%d", 
               cycle_count, current_pc, halted, dual_issue_active,
               dut.fetch_valid_0, dut.fetch_valid_1, 
               dut.fetch_inst_len_0, dut.fetch_inst_len_1);
      $display("         BufferValid=%d BufferPC=0x%h PC=0x%h consumed=%d",
               dut.fetch.buffer_valid, dut.fetch.buffer_pc, 
               dut.fetch.pc, dut.fetch.consumed_bytes);
      $display("         Spec0=0x%02h Op0=0x%02h Spec1=0x%02h Op1=0x%02h",
               dut.fetch.spec_0, dut.fetch.op_0,
               dut.fetch.spec_1, dut.fetch.op_1);
      $display("         Mem[0x%h]=0x%02h 0x%02h 0x%02h 0x%02h",
               dut.fetch.pc,
               memory.mem[dut.fetch.pc],
               memory.mem[dut.fetch.pc+1],
               memory.mem[dut.fetch.pc+2],
               memory.mem[dut.fetch.pc+3]);
    end
  end
  
  // VCD dump for waveform viewing
  initial begin
    $dumpfile("core_advanced_tb.vcd");
    $dumpvars(0, core_advanced_tb);
  end
  
  // Test stimulus
  initial begin
    $display("========================================");
    $display("NeoCore 16x32 Advanced Core Test");
    $display("Testing: Hazards, Branches, Dependencies");
    $display("========================================\n");
    
    // Initialize
    rst = 1;
    @(posedge clk);
    @(posedge clk);
    rst = 0;
    
    //-------------------------------------------
    // Test 1: RAW dependency chain
    //-------------------------------------------
    $display("\n*** Test 1: RAW Dependency Chain ***");
    load_program_dependency_chain();
    run_test("Dependency Chain", 5, 5, 1, 1);
    
    //-------------------------------------------
    // Test 2: Load-use hazard
    //-------------------------------------------
    $display("\n*** Test 2: Load-Use Hazard ***");
    load_program_load_use_hazard();
    run_test("Load-Use Hazard", 16'h1234, 16'h1234, 2, 3);
    
    //-------------------------------------------
    // Test 3: Branch sequence
    //-------------------------------------------
    $display("\n*** Test 3: Branch Sequence ***");
    load_program_branch_sequence();
    run_test("Branch Sequence", 5, 5, 1, 4);
    
    $display("\n========================================");
    $display("All Advanced Tests PASSED!");
    $display("========================================\n");
    $finish;
  end
  
  //============================================================================
  // Task: Load dependency chain program
  //============================================================================
  task load_program_dependency_chain();
    begin
      // Clear memory
      for (int i = 0; i < 256; i++) begin
        memory.mem[i] = 8'h00;
      end
      
      // MOV R1, #0x0001 at 0x00
      memory.mem[32'h00] = 8'h00;  // spec
      memory.mem[32'h01] = 8'h09;  // MOV
      memory.mem[32'h02] = 8'h01;  // R1
      memory.mem[32'h03] = 8'h00;  // imm high
      memory.mem[32'h04] = 8'h01;  // imm low

      // ADD R2, R1 at 0x05
      memory.mem[32'h05] = 8'h01;  // spec (register mode)
      memory.mem[32'h06] = 8'h01;  // ADD
      memory.mem[32'h07] = 8'h02;  // R2
      memory.mem[32'h08] = 8'h01;  // R1
      
      // ADD R3, R2 at 0x09
      memory.mem[32'h09] = 8'h01;  // spec
      memory.mem[32'h0A] = 8'h01;  // ADD
      memory.mem[32'h0B] = 8'h03;  // R3
      memory.mem[32'h0C] = 8'h02;  // R2
      
      // ADD R4, R3 at 0x0D
      memory.mem[32'h0D] = 8'h01;  // spec
      memory.mem[32'h0E] = 8'h01;  // ADD
      memory.mem[32'h0F] = 8'h04;  // R4
      memory.mem[32'h10] = 8'h03;  // R3
      
      // ADD R5, R4 at 0x11
      memory.mem[32'h11] = 8'h01;  // spec
      memory.mem[32'h12] = 8'h01;  // ADD
      memory.mem[32'h13] = 8'h05;  // R5
      memory.mem[32'h14] = 8'h04;  // R4
      
      // HLT at 0x15
      memory.mem[32'h15] = 8'h00;  // spec
      memory.mem[32'h16] = 8'h12;  // HLT
    end
  endtask
  
  //============================================================================
  // Task: Load load-use hazard program
  //============================================================================
  task load_program_load_use_hazard();
    begin
      // Clear memory
      for (int i = 0; i < 8192; i++) begin
        memory.mem[i] = 8'h00;
      end
      
      // MOV R1, #0x1234 at 0x00
      memory.mem[32'h00] = 8'h00;  // spec
      memory.mem[32'h01] = 8'h09;  // MOV
      memory.mem[32'h02] = 8'h01;  // R1
      memory.mem[32'h03] = 8'h12;  // imm high
      memory.mem[32'h04] = 8'h34;  // imm low
      
      // MOV [0x1000], R1 at 0x05 (store halfword)
      memory.mem[32'h05] = 8'h09;  // spec (reg to mem halfword)
      memory.mem[32'h06] = 8'h09;  // MOV
      memory.mem[32'h07] = 8'h01;  // R1
      memory.mem[32'h08] = 8'h00;  // addr byte 3
      memory.mem[32'h09] = 8'h00;  // addr byte 2
      memory.mem[32'h0A] = 8'h10;  // addr byte 1
      memory.mem[32'h0B] = 8'h00;  // addr byte 0
      
      // MOV R2, [0x1000] at 0x0C (load halfword)
      memory.mem[32'h0C] = 8'h05;  // spec (mem halfword to reg)
      memory.mem[32'h0D] = 8'h09;  // MOV
      memory.mem[32'h0E] = 8'h02;  // R2
      memory.mem[32'h0F] = 8'h00;  // addr byte 3
      memory.mem[32'h10] = 8'h00;  // addr byte 2
      memory.mem[32'h11] = 8'h10;  // addr byte 1
      memory.mem[32'h12] = 8'h00;  // addr byte 0
      
      // ADD R3, R2 at 0x13 (use loaded value - potential stall)
      memory.mem[32'h13] = 8'h01;  // spec
      memory.mem[32'h14] = 8'h01;  // ADD
      memory.mem[32'h15] = 8'h03;  // R3
      memory.mem[32'h16] = 8'h02;  // R2
      
      // HLT at 0x17
      memory.mem[32'h17] = 8'h00;  // spec
      memory.mem[32'h18] = 8'h12;  // HLT
    end
  endtask
  
  //============================================================================
  // Task: Load branch sequence program
  //============================================================================
  task load_program_branch_sequence();
    begin
      // Clear memory
      for (int i = 0; i < 256; i++) begin
        memory.mem[i] = 8'h00;
      end
      
      // MOV R1, #5 at 0x00
      memory.mem[32'h00] = 8'h00;
      memory.mem[32'h01] = 8'h09;
      memory.mem[32'h02] = 8'h01;
      memory.mem[32'h03] = 8'h00;
      memory.mem[32'h04] = 8'h05;
      
      // MOV R2, #5 at 0x05
      memory.mem[32'h05] = 8'h00;
      memory.mem[32'h06] = 8'h09;
      memory.mem[32'h07] = 8'h02;
      memory.mem[32'h08] = 8'h00;
      memory.mem[32'h09] = 8'h05;
      
      // BE R1, R2, 0x14 at 0x0A
      memory.mem[32'h0A] = 8'h00;  // spec
      memory.mem[32'h0B] = 8'h0B;  // BE
      memory.mem[32'h0C] = 8'h01;  // R1
      memory.mem[32'h0D] = 8'h02;  // R2
      memory.mem[32'h0E] = 8'h00;  // target byte 3
      memory.mem[32'h0F] = 8'h00;  // target byte 2
      memory.mem[32'h10] = 8'h00;  // target byte 1
      memory.mem[32'h11] = 8'h14;  // target byte 0 (0x14)
      
      // MOV R3, #0xFF at 0x12 (should be skipped)
      memory.mem[32'h12] = 8'h00;
      memory.mem[32'h13] = 8'h09;
      memory.mem[32'h14] = 8'h03;
      memory.mem[32'h15] = 8'h00;
      memory.mem[32'h16] = 8'hFF;
      
      // HLT at 0x17 (should be skipped)
      memory.mem[32'h17] = 8'h00;
      memory.mem[32'h18] = 8'h12;
      
      // MOV R4, #0xAA at 0x14 (target)
      memory.mem[32'h14] = 8'h00;
      memory.mem[32'h15] = 8'h09;
      memory.mem[32'h16] = 8'h04;
      memory.mem[32'h17] = 8'h00;
      memory.mem[32'h18] = 8'hAA;
      
      // HLT at 0x19
      memory.mem[32'h19] = 8'h00;
      memory.mem[32'h1A] = 8'h12;
    end
  endtask
  
  //============================================================================
  // Task: Run test and check results
  //============================================================================
  task run_test(input string test_name, 
                input logic [15:0] expected_r1, 
                input logic [15:0] expected_r2,
                input int expected_reg,
                input int expected_val_reg);
    begin
      // Reset core
      rst = 1;
      @(posedge clk);
      @(posedge clk);
      rst = 0;
      
      $display("Starting %s test...", test_name);
      
      // Run until halt or timeout
      fork
        begin
          wait(halted);
          $display("  Program halted at PC=0x%h after %0d cycles", current_pc, cycle_count);
          $display("  Dual-issue: %0d/%0d (%.1f%%)", dual_issue_count, cycle_count,
                   (100.0 * dual_issue_count) / cycle_count);
          $display("  Branches: %0d", branch_count);
          
          // Check results (accessing internal registers)
          $display("  Register values:");
          $display("    R1 = 0x%04h (expected 0x%04h)", dut.regfile.registers[1], expected_r1);
          $display("    R2 = 0x%04h (expected 0x%04h)", dut.regfile.registers[2], expected_r2);
          
          if (dut.regfile.registers[1] != expected_r1) begin
            $display("  ERROR: R1 mismatch!");
            $finish;
          end
          
          if (dut.regfile.registers[2] != expected_r2) begin
            $display("  ERROR: R2 mismatch!");
            $finish;
          end
          
          $display("  %s test PASSED", test_name);
        end
        begin
          repeat(5000) @(posedge clk);
          $display("  ERROR: Test timeout after %0d cycles", cycle_count);
          $display("  PC = 0x%08h, Halted = %b", current_pc, halted);
          $finish;
        end
      join_any
      disable fork;
      
      // Small delay between tests
      repeat(5) @(posedge clk);
    end
  endtask

endmodule : core_advanced_tb
