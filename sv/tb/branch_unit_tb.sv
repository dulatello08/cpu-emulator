//
// branch_unit_tb.sv
// Testbench for Branch Unit module
//
// Tests all branch condition evaluations.
//

`timescale 1ns/1ps

module branch_unit_tb;
  import neocore_pkg::*;

  // Testbench signals
  logic        clk;
  logic        rst;
  opcode_e     opcode;
  logic [15:0] operand_a;
  logic [15:0] operand_b;
  logic        v_flag_in;
  logic [31:0] branch_target;
  logic        branch_taken;
  logic [31:0] branch_pc;
  
  // Instantiate branch unit
  branch_unit dut (
    .clk(clk),
    .rst(rst),
    .opcode(opcode),
    .operand_a(operand_a),
    .operand_b(operand_b),
    .v_flag_in(v_flag_in),
    .branch_target(branch_target),
    .branch_taken(branch_taken),
    .branch_pc(branch_pc)
  );
  
  // Clock generation
  initial begin
    clk = 0;
    forever #5 clk = ~clk;
  end
  
  // VCD dump for waveform viewing
  initial begin
    $dumpfile("branch_unit_tb.vcd");
    $dumpvars(0, branch_unit_tb);
  end
  
  // Test stimulus
  initial begin
    $display("========================================");
    $display("Branch Unit Testbench");
    $display("========================================");
    
    // Initialize
    rst = 1;
    opcode = OP_NOP;
    operand_a = 16'h0000;
    operand_b = 16'h0000;
    v_flag_in = 1'b0;
    branch_target = 32'h0000_1000;
    @(posedge clk);
    rst = 0;
    @(posedge clk);
    
    // Test 1: Unconditional branch
    $display("\nTest 1: B (unconditional)");
    opcode = OP_B;
    branch_target = 32'h0000_5000;
    @(posedge clk);
    #1;
    assert(branch_taken == 1'b1) else $error("B should always be taken");
    assert(branch_pc == 32'h0000_5000) else $error("B target mismatch");
    $display("  PASS: Branch taken to 0x%08h", branch_pc);
    
    // Test 2: BE - equal (should branch)
    $display("\nTest 2: BE with equal operands");
    opcode = OP_BE;
    operand_a = 16'h1234;
    operand_b = 16'h1234;
    branch_target = 32'h0000_2000;
    @(posedge clk);
    #1;
    assert(branch_taken == 1'b1) else $error("BE should be taken when equal");
    assert(branch_pc == 32'h0000_2000) else $error("BE target mismatch");
    $display("  PASS: 0x%04h == 0x%04h, branch taken", operand_a, operand_b);
    
    // Test 3: BE - not equal (should not branch)
    $display("\nTest 3: BE with unequal operands");
    opcode = OP_BE;
    operand_a = 16'h1234;
    operand_b = 16'h5678;
    @(posedge clk);
    #1;
    assert(branch_taken == 1'b0) else $error("BE should not be taken when not equal");
    $display("  PASS: 0x%04h != 0x%04h, branch not taken", operand_a, operand_b);
    
    // Test 4: BNE - not equal (should branch)
    $display("\nTest 4: BNE with unequal operands");
    opcode = OP_BNE;
    operand_a = 16'h1234;
    operand_b = 16'h5678;
    branch_target = 32'h0000_3000;
    @(posedge clk);
    #1;
    assert(branch_taken == 1'b1) else $error("BNE should be taken when not equal");
    $display("  PASS: 0x%04h != 0x%04h, branch taken", operand_a, operand_b);
    
    // Test 5: BNE - equal (should not branch)
    $display("\nTest 5: BNE with equal operands");
    opcode = OP_BNE;
    operand_a = 16'hABCD;
    operand_b = 16'hABCD;
    @(posedge clk);
    #1;
    assert(branch_taken == 1'b0) else $error("BNE should not be taken when equal");
    $display("  PASS: 0x%04h == 0x%04h, branch not taken", operand_a, operand_b);
    
    // Test 6: BLT - less than (should branch)
    $display("\nTest 6: BLT with a < b");
    opcode = OP_BLT;
    operand_a = 16'h0010;
    operand_b = 16'h0020;
    branch_target = 32'h0000_4000;
    @(posedge clk);
    #1;
    assert(branch_taken == 1'b1) else $error("BLT should be taken when a < b");
    $display("  PASS: 0x%04h < 0x%04h, branch taken", operand_a, operand_b);
    
    // Test 7: BLT - not less than (should not branch)
    $display("\nTest 7: BLT with a >= b");
    opcode = OP_BLT;
    operand_a = 16'h0020;
    operand_b = 16'h0010;
    @(posedge clk);
    #1;
    assert(branch_taken == 1'b0) else $error("BLT should not be taken when a >= b");
    $display("  PASS: 0x%04h >= 0x%04h, branch not taken", operand_a, operand_b);
    
    // Test 8: BGT - greater than (should branch)
    $display("\nTest 8: BGT with a > b");
    opcode = OP_BGT;
    operand_a = 16'h0030;
    operand_b = 16'h0010;
    branch_target = 32'h0000_5000;
    @(posedge clk);
    #1;
    assert(branch_taken == 1'b1) else $error("BGT should be taken when a > b");
    $display("  PASS: 0x%04h > 0x%04h, branch taken", operand_a, operand_b);
    
    // Test 9: BGT - not greater than (should not branch)
    $display("\nTest 9: BGT with a <= b");
    opcode = OP_BGT;
    operand_a = 16'h0010;
    operand_b = 16'h0030;
    @(posedge clk);
    #1;
    assert(branch_taken == 1'b0) else $error("BGT should not be taken when a <= b");
    $display("  PASS: 0x%04h <= 0x%04h, branch not taken", operand_a, operand_b);
    
    // Test 10: BRO - overflow set (should branch)
    $display("\nTest 10: BRO with V flag set");
    opcode = OP_BRO;
    v_flag_in = 1'b1;
    branch_target = 32'h0000_6000;
    @(posedge clk);
    #1;
    assert(branch_taken == 1'b1) else $error("BRO should be taken when V=1");
    $display("  PASS: V flag set, branch taken");
    
    // Test 11: BRO - overflow clear (should not branch)
    $display("\nTest 11: BRO with V flag clear");
    opcode = OP_BRO;
    v_flag_in = 1'b0;
    @(posedge clk);
    #1;
    assert(branch_taken == 1'b0) else $error("BRO should not be taken when V=0");
    $display("  PASS: V flag clear, branch not taken");
    
    // Test 12: JSR (should always branch)
    $display("\nTest 12: JSR");
    opcode = OP_JSR;
    branch_target = 32'h0000_7000;
    @(posedge clk);
    #1;
    assert(branch_taken == 1'b1) else $error("JSR should always be taken");
    assert(branch_pc == 32'h0000_7000) else $error("JSR target mismatch");
    $display("  PASS: JSR taken to 0x%08h", branch_pc);
    
    $display("\n========================================");
    $display("Branch Unit Testbench PASSED");
    $display("========================================\n");
    
    $finish;
  end

endmodule
