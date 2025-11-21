//
// core_tb.sv
// Testbench for NeoCore 16x32 Dual-Issue CPU Core
//
// Tests the complete core with simple programs.
//

`timescale 1ns/1ps

module core_tb;
  import neocore_pkg::*;

  // Testbench signals
  logic        clk;
  logic        rst;
  logic [31:0] imem_addr;
  logic        imem_req;
  logic [63:0] imem_rdata;
  logic        imem_ack;
  logic [31:0] dmem_addr;
  logic [31:0] dmem_wdata;
  logic [1:0]  dmem_size;
  logic        dmem_we;
  logic        dmem_req;
  logic [31:0] dmem_rdata;
  logic        dmem_ack;
  logic        halted;
  logic [31:0] current_pc;
  logic        dual_issue_active;
  
  // Memory instance
  simple_memory #(
    .MEM_SIZE(65536)
  ) memory (
    .clk(clk),
    .rst(rst),
    .imem_addr(imem_addr),
    .imem_req(imem_req),
    .imem_rdata(imem_rdata),
    .imem_ack(imem_ack),
    .dmem_addr(dmem_addr),
    .dmem_wdata(dmem_wdata),
    .dmem_size(dmem_size),
    .dmem_we(dmem_we),
    .dmem_req(dmem_req),
    .dmem_rdata(dmem_rdata),
    .dmem_ack(dmem_ack)
  );
  
  // Core instance
  core_top dut (
    .clk(clk),
    .rst(rst),
    .imem_addr(imem_addr),
    .imem_req(imem_req),
    .imem_rdata(imem_rdata),
    .imem_ack(imem_ack),
    .dmem_addr(dmem_addr),
    .dmem_wdata(dmem_wdata),
    .dmem_size(dmem_size),
    .dmem_we(dmem_we),
    .dmem_req(dmem_req),
    .dmem_rdata(dmem_rdata),
    .dmem_ack(dmem_ack),
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
  
  always_ff @(posedge clk) begin
    if (rst) cycle_count <= 0;
    else cycle_count <= cycle_count + 1;
  end
  
  // Test stimulus
  initial begin
    $display("========================================");
    $display("NeoCore 16x32 Dual-Issue CPU Core Test");
    $display("========================================");
    
    // Reset
    rst = 1;
    repeat(5) @(posedge clk);
    rst = 0;
    @(posedge clk);
    
    // =======================================================================
    // Test 1: Simple Arithmetic
    // =======================================================================
    $display("\n=== Test 1: Simple Arithmetic ===");
    $display("Program:");
    $display("  MOV R1, #5");
    $display("  MOV R2, #7");
    $display("  ADD R1, R2    (R1 = R1 + R2 = 12)");
    $display("  HLT");
    
    // Load program into memory
    // MOV R1, #5 (specifier=00, opcode=09, rd=01, imm=00 05)
    memory.mem[0] = 8'h00;
    memory.mem[1] = 8'h09;
    memory.mem[2] = 8'h01;
    memory.mem[3] = 8'h00;
    memory.mem[4] = 8'h05;
    
    // MOV R2, #7
    memory.mem[5] = 8'h00;
    memory.mem[6] = 8'h09;
    memory.mem[7] = 8'h02;
    memory.mem[8] = 8'h00;
    memory.mem[9] = 8'h07;
    
    // ADD R1, R2 (specifier=01, opcode=01, rd=01, rn=02)
    memory.mem[10] = 8'h01;
    memory.mem[11] = 8'h01;
    memory.mem[12] = 8'h01;
    memory.mem[13] = 8'h02;
    
    // HLT
    memory.mem[14] = 8'h00;
    memory.mem[15] = 8'h12;
    
    // Run until halt or timeout
    fork
      begin
        wait(halted);
        $display("\nCore halted at cycle %0d", cycle_count);
      end
      begin
        repeat(1000) @(posedge clk);
        $display("\nTimeout after 1000 cycles");
        $finish;
      end
    join_any
    disable fork;
    
    // Check results
    @(posedge clk);
    $display("\nResults:");
    $display("  R1 = 0x%04h (expected 0x000C)", dut.regfile.registers[1]);
    $display("  R2 = 0x%04h (expected 0x0007)", dut.regfile.registers[2]);
    
    if (dut.regfile.registers[1] == 16'h000C && 
        dut.regfile.registers[2] == 16'h0007) begin
      $display("  ✓ Test 1 PASSED");
    end else begin
      $display("  ✗ Test 1 FAILED");
    end
    
    // =======================================================================
    // Test 2: Dual-Issue Test
    // =======================================================================
    $display("\n=== Test 2: Dual-Issue Test ===");
    $display("Program:");
    $display("  MOV R3, #10");
    $display("  MOV R4, #20   (should dual-issue with above)");
    $display("  ADD R3, R4");
    $display("  HLT");
    
    // Reset core
    rst = 1;
    repeat(5) @(posedge clk);
    rst = 0;
    @(posedge clk);
    
    // Clear memory
    for (int i = 0; i < 100; i++) memory.mem[i] = 8'h00;
    
    // MOV R3, #10
    memory.mem[0] = 8'h00;
    memory.mem[1] = 8'h09;
    memory.mem[2] = 8'h03;
    memory.mem[3] = 8'h00;
    memory.mem[4] = 8'h0A;
    
    // MOV R4, #20
    memory.mem[5] = 8'h00;
    memory.mem[6] = 8'h09;
    memory.mem[7] = 8'h04;
    memory.mem[8] = 8'h00;
    memory.mem[9] = 8'h14;
    
    // ADD R3, R4
    memory.mem[10] = 8'h01;
    memory.mem[11] = 8'h01;
    memory.mem[12] = 8'h03;
    memory.mem[13] = 8'h04;
    
    // HLT
    memory.mem[14] = 8'h00;
    memory.mem[15] = 8'h12;
    
    // Monitor dual-issue activity
    dual_issue_count = 0;
    fork
      begin
        forever begin
          @(posedge clk);
          if (dual_issue_active) begin
            dual_issue_count++;
            $display("  [Cycle %0d] Dual-issue detected!", cycle_count);
          end
        end
      end
    join_none
    
    // Run until halt
    fork
      begin
        wait(halted);
        $display("\nCore halted at cycle %0d", cycle_count);
      end
      begin
        repeat(1000) @(posedge clk);
        $display("\nTimeout");
        $finish;
      end
    join_any
    disable fork;
    
    @(posedge clk);
    $display("\nResults:");
    $display("  R3 = 0x%04h (expected 0x001E = 30)", dut.regfile.registers[3]);
    $display("  R4 = 0x%04h (expected 0x0014 = 20)", dut.regfile.registers[4]);
    $display("  Dual-issue events: %0d", dual_issue_count);
    
    if (dut.regfile.registers[3] == 16'h001E && 
        dut.regfile.registers[4] == 16'h0014) begin
      $display("  ✓ Test 2 PASSED");
    end else begin
      $display("  ✗ Test 2 FAILED");
    end
    
    // =======================================================================
    // Test 3: Data Hazard and Forwarding
    // =======================================================================
    $display("\n=== Test 3: Data Hazard and Forwarding ===");
    $display("Program:");
    $display("  MOV R5, #3");
    $display("  ADD R5, #2    (R5 = 5, should forward from previous ADD)");
    $display("  ADD R5, #1    (R5 = 6, should forward from previous ADD)");
    $display("  HLT");
    
    // Reset
    rst = 1;
    repeat(5) @(posedge clk);
    rst = 0;
    @(posedge clk);
    
    // Clear memory
    for (int i = 0; i < 100; i++) memory.mem[i] = 8'h00;
    
    // MOV R5, #3
    memory.mem[0] = 8'h00;
    memory.mem[1] = 8'h09;
    memory.mem[2] = 8'h05;
    memory.mem[3] = 8'h00;
    memory.mem[4] = 8'h03;
    
    // ADD R5, #2 (immediate add)
    memory.mem[5] = 8'h00;  // specifier 00 = immediate
    memory.mem[6] = 8'h01;  // opcode ADD
    memory.mem[7] = 8'h05;  // rd = R5
    memory.mem[8] = 8'h00;  // immediate high
    memory.mem[9] = 8'h02;  // immediate low
    
    // ADD R5, #1
    memory.mem[10] = 8'h00;
    memory.mem[11] = 8'h01;
    memory.mem[12] = 8'h05;
    memory.mem[13] = 8'h00;
    memory.mem[14] = 8'h01;
    
    // HLT
    memory.mem[15] = 8'h00;
    memory.mem[16] = 8'h12;
    
    // Run
    fork
      begin
        wait(halted);
        $display("\nCore halted at cycle %0d", cycle_count);
      end
      begin
        repeat(1000) @(posedge clk);
        $display("\nTimeout");
        $finish;
      end
    join_any
    disable fork;
    
    @(posedge clk);
    $display("\nResults:");
    $display("  R5 = 0x%04h (expected 0x0006)", dut.regfile.registers[5]);
    
    if (dut.regfile.registers[5] == 16'h0006) begin
      $display("  ✓ Test 3 PASSED");
    end else begin
      $display("  ✗ Test 3 FAILED");
    end
    
    // =======================================================================
    // Summary
    // =======================================================================
    $display("\n========================================");
    $display("Core Testbench Complete");
    $display("========================================\n");
    
    $finish;
  end
  
  // Timeout watchdog
  initial begin
    #500000;  // 500 microseconds
    $display("\nERROR: Global timeout!");
    $finish;
  end

endmodule
