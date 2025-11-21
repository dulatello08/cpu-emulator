//
// core_simple_tb.sv
// Simple testbench for debugging core execution
//

`timescale 1ns/1ps

module core_simple_tb;
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
  
  always_ff @(posedge clk) begin
    if (rst) begin
      cycle_count <= 0;
    end else begin
      cycle_count <= cycle_count + 1;
    end
  end
  
  // Test stimulus
  initial begin
    $display("===========================================");
    $display("Simple Core Test - Just NOP and HLT");
    $display("===========================================\n");
    
    // Initialize
    rst = 1;
    @(posedge clk);
    @(posedge clk);
    rst = 0;
    
    $display("Loading minimal test program...");
    
    // Minimal test program (big-endian encoding):
    // 0x00: NOP    [00][00]
    // 0x02: NOP    [00][00]
    // 0x04: HLT    [00][12]
    
    // Initialize all memory to zero
    for (int i = 0; i < 256; i++) begin
      memory.mem[i] = 8'h00;
    end
    
    // Load program (big-endian)
    memory.mem[32'h00] = 8'h00;  // NOP spec
    memory.mem[32'h01] = 8'h00;  // NOP op
    
    memory.mem[32'h02] = 8'h00;  // NOP spec
    memory.mem[32'h03] = 8'h00;  // NOP op
    
    memory.mem[32'h04] = 8'h00;  // HLT spec
    memory.mem[32'h05] = 8'h12;  // HLT op
    
    $display("Program loaded.\n");
    $display("Expected execution:");
    $display("  PC=0x00: NOP");
    $display("  PC=0x02: NOP");
    $display("  PC=0x04: HLT");
    $display("");
    
    // Run for limited cycles
    repeat(50) @(posedge clk);
    
    $display("\n===========================================");
    $display("Test completed after %0d cycles", cycle_count);
    $display("Final PC = 0x%08h, Halted = %b", current_pc, halted);
    
    if (halted && current_pc == 32'h04) begin
      $display("TEST PASSED - Core halted at correct PC");
    end else if (halted) begin
      $display("TEST PARTIAL - Core halted but at wrong PC");
    end else begin
      $display("TEST FAILED - Core did not halt");
    end
    $display("===========================================");
    
    $finish;
  end
  
  // Monitor execution
  logic [31:0] prev_pc;
  always_ff @(posedge clk) begin
    if (rst) begin
      prev_pc <= 32'hFFFFFFFF;
    end else if (current_pc != prev_pc) begin
      $display("Cycle %3d: PC changed 0x%08h -> 0x%08h, Halt=%b", 
               cycle_count, prev_pc, current_pc, halted);
      prev_pc <= current_pc;
    end
  end

endmodule
