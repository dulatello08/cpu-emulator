//
// register_file_tb.sv
// Testbench for Register File module
//
// Tests read/write operations and forwarding logic.
//

`timescale 1ns/1ps

module register_file_tb;
  import neocore_pkg::*;

  // Testbench signals
  logic        clk;
  logic        rst;
  logic [3:0]  rs1_addr_0;
  logic [3:0]  rs2_addr_0;
  logic [15:0] rs1_data_0;
  logic [15:0] rs2_data_0;
  logic [3:0]  rs1_addr_1;
  logic [3:0]  rs2_addr_1;
  logic [15:0] rs1_data_1;
  logic [15:0] rs2_data_1;
  logic [3:0]  rd_addr_0;
  logic [15:0] rd_data_0;
  logic        rd_we_0;
  logic [3:0]  rd_addr_1;
  logic [15:0] rd_data_1;
  logic        rd_we_1;
  
  // Instantiate register file
  register_file dut (
    .clk(clk),
    .rst(rst),
    .rs1_addr_0(rs1_addr_0),
    .rs2_addr_0(rs2_addr_0),
    .rs1_data_0(rs1_data_0),
    .rs2_data_0(rs2_data_0),
    .rs1_addr_1(rs1_addr_1),
    .rs2_addr_1(rs2_addr_1),
    .rs1_data_1(rs1_data_1),
    .rs2_data_1(rs2_data_1),
    .rd_addr_0(rd_addr_0),
    .rd_data_0(rd_data_0),
    .rd_we_0(rd_we_0),
    .rd_addr_1(rd_addr_1),
    .rd_data_1(rd_data_1),
    .rd_we_1(rd_we_1)
  );
  
  // Clock generation
  initial begin
    clk = 0;
    forever #5 clk = ~clk;
  end
  
  // VCD dump for waveform viewing
  initial begin
    $dumpfile("register_file_tb.vcd");
    $dumpvars(0, register_file_tb);
  end
  
  // Test stimulus
  initial begin
    $display("========================================");
    $display("Register File Testbench");
    $display("========================================");
    
    // Initialize
    rst = 1;
    rs1_addr_0 = 0;
    rs2_addr_0 = 0;
    rs1_addr_1 = 0;
    rs2_addr_1 = 0;
    rd_addr_0 = 0;
    rd_data_0 = 0;
    rd_we_0 = 0;
    rd_addr_1 = 0;
    rd_data_1 = 0;
    rd_we_1 = 0;
    @(posedge clk);
    rst = 0;
    @(posedge clk);
    
    // Test 1: Write to register 1
    $display("\nTest 1: Write 0x1234 to R1");
    rd_addr_0 = 4'd1;
    rd_data_0 = 16'h1234;
    rd_we_0 = 1'b1;
    rs1_addr_0 = 4'd1;
    @(posedge clk);
    #1;
    // Forwarding should give us the value immediately
    assert(rs1_data_0 == 16'h1234) else $error("Forwarding R1 failed: got %h", rs1_data_0);
    rd_we_0 = 1'b0;
    @(posedge clk);
    #1;
    // After write is done, should still read the value
    assert(rs1_data_0 == 16'h1234) else $error("Read R1 failed: got %h", rs1_data_0);
    $display("  PASS: R1 = 0x%04h", rs1_data_0);
    
    // Test 2: Write to multiple registers
    $display("\nTest 2: Write to R2 and R3");
    rd_addr_0 = 4'd2;
    rd_data_0 = 16'hABCD;
    rd_we_0 = 1'b1;
    rd_addr_1 = 4'd3;
    rd_data_1 = 16'h5678;
    rd_we_1 = 1'b1;
    rs1_addr_0 = 4'd2;
    rs1_addr_1 = 4'd3;
    @(posedge clk);
    #1;
    // Forwarding during write
    assert(rs1_data_0 == 16'hABCD) else $error("Forwarding R2 failed");
    assert(rs1_data_1 == 16'h5678) else $error("Forwarding R3 failed");
    rd_we_0 = 1'b0;
    rd_we_1 = 1'b0;
    @(posedge clk);
    #1;
    // After writes complete
    assert(rs1_data_0 == 16'hABCD) else $error("Read R2 failed");
    assert(rs1_data_1 == 16'h5678) else $error("Read R3 failed");
    $display("  PASS: R2 = 0x%04h, R3 = 0x%04h", rs1_data_0, rs1_data_1);
    
    // Test 3: Read multiple registers simultaneously
    $display("\nTest 3: Read R1, R2 on port 0 and R2, R3 on port 1");
    rs1_addr_0 = 4'd1;
    rs2_addr_0 = 4'd2;
    rs1_addr_1 = 4'd2;
    rs2_addr_1 = 4'd3;
    @(posedge clk);
    #1;
    assert(rs1_data_0 == 16'h1234) else $error("Port 0 RS1 failed");
    assert(rs2_data_0 == 16'hABCD) else $error("Port 0 RS2 failed");
    assert(rs1_data_1 == 16'hABCD) else $error("Port 1 RS1 failed");
    assert(rs2_data_1 == 16'h5678) else $error("Port 1 RS2 failed");
    $display("  PASS: Port0(R1=0x%04h, R2=0x%04h), Port1(R2=0x%04h, R3=0x%04h)", 
             rs1_data_0, rs2_data_0, rs1_data_1, rs2_data_1);
    
    // Test 4: Forwarding - write and read same register
    $display("\nTest 4: Forwarding - write 0xDEAD to R4 and read it immediately");
    rd_addr_0 = 4'd4;
    rd_data_0 = 16'hDEAD;
    rd_we_0 = 1'b1;
    rs1_addr_0 = 4'd4;
    #1;  // Combinational read should see forwarded value
    assert(rs1_data_0 == 16'hDEAD) else $error("Forwarding failed: got %h", rs1_data_0);
    $display("  PASS: Forwarded value = 0x%04h", rs1_data_0);
    @(posedge clk);
    rd_we_0 = 1'b0;
    
    // Test 5: Write to same register from both ports (port 1 wins)
    $display("\nTest 5: Write to R5 from both ports (port 1 should win)");
    rd_addr_0 = 4'd5;
    rd_data_0 = 16'hBEEF;
    rd_we_0 = 1'b1;
    rd_addr_1 = 4'd5;
    rd_data_1 = 16'hCAFE;
    rd_we_1 = 1'b1;
    @(posedge clk);
    rd_we_0 = 1'b0;
    rd_we_1 = 1'b0;
    rs1_addr_0 = 4'd5;
    @(posedge clk);
    #1;
    assert(rs1_data_0 == 16'hCAFE) else $error("Port 1 priority failed: got %h", rs1_data_0);
    $display("  PASS: R5 = 0x%04h (port 1 won)", rs1_data_0);
    
    $display("\n========================================");
    $display("Register File Testbench PASSED");
    $display("========================================\n");
    
    $finish;
  end

endmodule
