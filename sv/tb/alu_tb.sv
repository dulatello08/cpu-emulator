//
// alu_tb.sv
// Testbench for ALU module
//
// Tests all ALU operations and flag generation.
//

`timescale 1ns/1ps

module alu_tb;
  import neocore_pkg::*;

  // Testbench signals
  logic        clk;
  logic        rst;
  logic [15:0] operand_a;
  logic [15:0] operand_b;
  alu_op_e     alu_op;
  logic [31:0] result;
  logic        z_flag;
  logic        v_flag;
  
  // Instantiate ALU
  alu dut (
    .clk(clk),
    .rst(rst),
    .operand_a(operand_a),
    .operand_b(operand_b),
    .alu_op(alu_op),
    .result(result),
    .z_flag(z_flag),
    .v_flag(v_flag)
  );
  
  // Clock generation
  initial begin
    clk = 0;
    forever #5 clk = ~clk;
  end
  
  // VCD dump for waveform viewing
  initial begin
    $dumpfile("alu_tb.vcd");
    $dumpvars(0, alu_tb);
  end
  
  // Test stimulus
  initial begin
    $display("========================================");
    $display("ALU Testbench");
    $display("========================================");
    
    // Reset
    rst = 1;
    operand_a = 16'h0000;
    operand_b = 16'h0000;
    alu_op = ALU_NOP;
    @(posedge clk);
    rst = 0;
    @(posedge clk);
    
    // Test ADD
    $display("\nTest ADD");
    operand_a = 16'h0005;
    operand_b = 16'h0003;
    alu_op = ALU_ADD;
    @(posedge clk);
    #1;
    assert(result == 32'h0000_0008) else $error("ADD failed: %h + %h = %h (expected 0x8)", operand_a, operand_b, result);
    assert(z_flag == 0) else $error("Z flag should be 0");
    assert(v_flag == 0) else $error("V flag should be 0");
    $display("  PASS: 0x%04h + 0x%04h = 0x%08h (Z=%b, V=%b)", operand_a, operand_b, result, z_flag, v_flag);
    
    // Test ADD with overflow
    $display("\nTest ADD with overflow");
    operand_a = 16'hFFFF;
    operand_b = 16'h0002;
    alu_op = ALU_ADD;
    @(posedge clk);
    #1;
    assert(result == 32'h0001_0001) else $error("ADD overflow failed");
    assert(z_flag == 0) else $error("Z flag should be 0");
    assert(v_flag == 1) else $error("V flag should be 1");
    $display("  PASS: 0x%04h + 0x%04h = 0x%08h (Z=%b, V=%b)", operand_a, operand_b, result, z_flag, v_flag);
    
    // Test SUB
    $display("\nTest SUB");
    operand_a = 16'h0010;
    operand_b = 16'h0005;
    alu_op = ALU_SUB;
    @(posedge clk);
    #1;
    assert(result == 32'h0000_000B) else $error("SUB failed");
    assert(z_flag == 0) else $error("Z flag should be 0");
    $display("  PASS: 0x%04h - 0x%04h = 0x%08h (Z=%b, V=%b)", operand_a, operand_b, result, z_flag, v_flag);
    
    // Test SUB with result = 0
    $display("\nTest SUB with zero result");
    operand_a = 16'h0005;
    operand_b = 16'h0005;
    alu_op = ALU_SUB;
    @(posedge clk);
    #1;
    assert(result == 32'h0000_0000) else $error("SUB zero failed");
    assert(z_flag == 1) else $error("Z flag should be 1");
    $display("  PASS: 0x%04h - 0x%04h = 0x%08h (Z=%b, V=%b)", operand_a, operand_b, result, z_flag, v_flag);
    
    // Test SUB with negative (should return 0)
    $display("\nTest SUB with underflow (returns 0)");
    operand_a = 16'h0002;
    operand_b = 16'h0005;
    alu_op = ALU_SUB;
    @(posedge clk);
    #1;
    assert(result == 32'h0000_0000) else $error("SUB underflow failed");
    assert(z_flag == 1) else $error("Z flag should be 1");
    $display("  PASS: 0x%04h - 0x%04h = 0x%08h (Z=%b, V=%b)", operand_a, operand_b, result, z_flag, v_flag);
    
    // Test MUL
    $display("\nTest MUL");
    operand_a = 16'h0005;
    operand_b = 16'h0007;
    alu_op = ALU_MUL;
    @(posedge clk);
    #1;
    assert(result == 32'h0000_0023) else $error("MUL failed");
    $display("  PASS: 0x%04h * 0x%04h = 0x%08h (Z=%b, V=%b)", operand_a, operand_b, result, z_flag, v_flag);
    
    // Test AND
    $display("\nTest AND");
    operand_a = 16'hFF00;
    operand_b = 16'h0F0F;
    alu_op = ALU_AND;
    @(posedge clk);
    #1;
    assert(result == 32'h0000_0F00) else $error("AND failed");
    $display("  PASS: 0x%04h & 0x%04h = 0x%08h (Z=%b, V=%b)", operand_a, operand_b, result, z_flag, v_flag);
    
    // Test OR
    $display("\nTest OR");
    operand_a = 16'hF000;
    operand_b = 16'h000F;
    alu_op = ALU_OR;
    @(posedge clk);
    #1;
    assert(result == 32'h0000_F00F) else $error("OR failed");
    $display("  PASS: 0x%04h | 0x%04h = 0x%08h (Z=%b, V=%b)", operand_a, operand_b, result, z_flag, v_flag);
    
    // Test XOR
    $display("\nTest XOR");
    operand_a = 16'hFFFF;
    operand_b = 16'hF0F0;
    alu_op = ALU_XOR;
    @(posedge clk);
    #1;
    assert(result == 32'h0000_0F0F) else $error("XOR failed");
    $display("  PASS: 0x%04h ^ 0x%04h = 0x%08h (Z=%b, V=%b)", operand_a, operand_b, result, z_flag, v_flag);
    
    // Test LSH
    $display("\nTest LSH");
    operand_a = 16'h0005;
    operand_b = 16'h0002;  // Shift left by 2
    alu_op = ALU_LSH;
    @(posedge clk);
    #1;
    assert(result == 32'h0000_0014) else $error("LSH failed: got %h", result);
    $display("  PASS: 0x%04h << %d = 0x%08h (Z=%b, V=%b)", operand_a, operand_b, result, z_flag, v_flag);
    
    // Test RSH
    $display("\nTest RSH");
    operand_a = 16'h0014;
    operand_b = 16'h0002;  // Shift right by 2
    alu_op = ALU_RSH;
    @(posedge clk);
    #1;
    assert(result == 32'h0000_0005) else $error("RSH failed");
    $display("  PASS: 0x%04h >> %d = 0x%08h (Z=%b, V=%b)", operand_a, operand_b, result, z_flag, v_flag);
    
    $display("\n========================================");
    $display("ALU Testbench PASSED");
    $display("========================================\n");
    
    $finish;
  end

endmodule
