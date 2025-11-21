//
// multiply_unit_tb.sv
// Testbench for Multiply Unit module
//
// Tests unsigned and signed 16x16 → 32-bit multiplication.
//

`timescale 1ns/1ps

module multiply_unit_tb;
  import neocore_pkg::*;

  // Testbench signals
  logic        clk;
  logic        rst;
  logic [15:0] operand_a;
  logic [15:0] operand_b;
  logic        is_signed;
  logic [15:0] result_lo;
  logic [15:0] result_hi;
  
  // Instantiate multiply unit
  multiply_unit dut (
    .clk(clk),
    .rst(rst),
    .operand_a(operand_a),
    .operand_b(operand_b),
    .is_signed(is_signed),
    .result_lo(result_lo),
    .result_hi(result_hi)
  );
  
  // Clock generation
  initial begin
    clk = 0;
    forever #5 clk = ~clk;
  end
  
  // VCD dump for waveform viewing
  initial begin
    $dumpfile("multiply_unit_tb.vcd");
    $dumpvars(0, multiply_unit_tb);
  end
  
  // Test stimulus
  initial begin
    $display("========================================");
    $display("Multiply Unit Testbench");
    $display("========================================");
    
    // Reset
    rst = 1;
    operand_a = 16'h0000;
    operand_b = 16'h0000;
    is_signed = 1'b0;
    @(posedge clk);
    rst = 0;
    @(posedge clk);
    
    // Test 1: Unsigned multiply - small numbers
    $display("\nTest 1: UMULL 5 * 7");
    operand_a = 16'd5;
    operand_b = 16'd7;
    is_signed = 1'b0;
    @(posedge clk);
    #1;
    assert({result_hi, result_lo} == 32'd35) else $error("UMULL failed: got %h%h", result_hi, result_lo);
    $display("  PASS: 5 * 7 = %d (0x%04h_%04h)", {result_hi, result_lo}, result_hi, result_lo);
    
    // Test 2: Unsigned multiply - larger numbers
    $display("\nTest 2: UMULL 0x100 * 0x200");
    operand_a = 16'h0100;
    operand_b = 16'h0200;
    is_signed = 1'b0;
    @(posedge clk);
    #1;
    assert({result_hi, result_lo} == 32'h0002_0000) else $error("UMULL failed");
    $display("  PASS: 0x100 * 0x200 = 0x%04h_%04h", result_hi, result_lo);
    
    // Test 3: Unsigned multiply - max values
    $display("\nTest 3: UMULL 0xFFFF * 0xFFFF");
    operand_a = 16'hFFFF;
    operand_b = 16'hFFFF;
    is_signed = 1'b0;
    @(posedge clk);
    #1;
    assert({result_hi, result_lo} == 32'hFFFE_0001) else $error("UMULL max failed");
    $display("  PASS: 0xFFFF * 0xFFFF = 0x%04h_%04h", result_hi, result_lo);
    
    // Test 4: Signed multiply - positive numbers
    $display("\nTest 4: SMULL 5 * 7");
    operand_a = 16'd5;
    operand_b = 16'd7;
    is_signed = 1'b1;
    @(posedge clk);
    #1;
    assert({result_hi, result_lo} == 32'd35) else $error("SMULL positive failed");
    $display("  PASS: 5 * 7 = %d (0x%04h_%04h)", $signed({result_hi, result_lo}), result_hi, result_lo);
    
    // Test 5: Signed multiply - negative * positive
    $display("\nTest 5: SMULL -5 * 7");
    operand_a = 16'hFFFB;  // -5 in two's complement
    operand_b = 16'd7;
    is_signed = 1'b1;
    @(posedge clk);
    #1;
    assert($signed({result_hi, result_lo}) == -35) else $error("SMULL negative failed");
    $display("  PASS: -5 * 7 = %d (0x%04h_%04h)", $signed({result_hi, result_lo}), result_hi, result_lo);
    
    // Test 6: Signed multiply - negative * negative
    $display("\nTest 6: SMULL -5 * -7");
    operand_a = 16'hFFFB;  // -5
    operand_b = 16'hFFF9;  // -7
    is_signed = 1'b1;
    @(posedge clk);
    #1;
    assert($signed({result_hi, result_lo}) == 35) else $error("SMULL neg*neg failed");
    $display("  PASS: -5 * -7 = %d (0x%04h_%04h)", $signed({result_hi, result_lo}), result_hi, result_lo);
    
    // Test 7: Signed multiply - large negative
    $display("\nTest 7: SMULL -100 * 200");
    operand_a = 16'hFF9C;  // -100
    operand_b = 16'd200;
    is_signed = 1'b1;
    @(posedge clk);
    #1;
    assert($signed({result_hi, result_lo}) == -20000) else $error("SMULL large neg failed: got %d", $signed({result_hi, result_lo}));
    $display("  PASS: -100 * 200 = %d (0x%04h_%04h)", $signed({result_hi, result_lo}), result_hi, result_lo);
    
    $display("\n========================================");
    $display("Multiply Unit Testbench PASSED");
    $display("========================================\n");
    
    $finish;
  end

endmodule
