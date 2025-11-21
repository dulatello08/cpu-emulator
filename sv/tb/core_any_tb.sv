//
// core_any_tb.sv
// Generic Testbench for NeoCore 16x32 Dual-Issue CPU Core
//
// Loads a program from a hex file specified via command line and dumps
// register state at completion.
//
// Usage:
//   iverilog -g2012 -o core_any_tb ... -DPROGRAM_FILE=\"input.hex\"
//   vvp core_any_tb
//
// Or using Makefile:
//   make run_core_any PROGRAM=input.hex
//

`timescale 1ns/1ps

module core_any_tb;
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
  
  always_ff @(posedge clk) begin
    if (rst) begin
      cycle_count <= 0;
      dual_issue_count <= 0;
    end else begin
      cycle_count <= cycle_count + 1;
      if (dual_issue_active) begin
        dual_issue_count <= dual_issue_count + 1;
      end
    end
  end
  
  // VCD dump for waveform viewing
  initial begin
    $dumpfile("core_any_tb.vcd");
    $dumpvars(0, core_any_tb);
  end
  
  // Program file name (can be overridden with +define+ or -D)
`ifndef PROGRAM_FILE
  `define PROGRAM_FILE "input.hex"
`endif
  
  // Debug flag
  logic debug_enabled = 1'b0;
  
  // Enable debug mode with +DEBUG
  initial begin
    if ($test$plusargs("DEBUG")) begin
      debug_enabled = 1'b1;
    end
  end
  
  // Detailed cycle-by-cycle logging
  always @(posedge clk) begin
    if (debug_enabled && !rst) begin
      $display("Cycle %0d: PC=%h (FetchPC0=%h) Halt=%b BufferValid=%0d Spec0=%h Op0=%h Len0=%0d Spec1=%h Op1=%h Len1=%0d", 
               cycle_count, dut.current_pc, dut.fetch_pc_0, dut.halted,
               dut.fetch.buffer_valid,
               dut.fetch.spec_0, dut.fetch.op_0, dut.fetch.inst_len_0,
               dut.fetch.spec_1, dut.fetch.op_1, dut.fetch.inst_len_1);
      $display("         Consumed=%0d BufferPC=%h Valid0=%b Valid1=%b DualIssue=%b (from issue=%b) MemReq=%b MemAddr=%h",
               dut.fetch.consumed_bytes, dut.fetch.buffer_pc,
               dut.fetch.valid_0, dut.fetch.valid_1, dut.dual_issue,
               dut.issue.dual_issue,
               dut.fetch.mem_req, dut.fetch.mem_addr);
      $display("         Buffer[31:0]=%02h %02h %02h %02h", 
               dut.fetch.fetch_buffer[0], dut.fetch.fetch_buffer[1],
               dut.fetch.fetch_buffer[2], dut.fetch.fetch_buffer[3]);
    end
  end
  
  // Test stimulus
  initial begin
    string program_file;
    int fd;
    int byte_val;
    int addr;
    int bytes_loaded;
    
    // Get program file from command line or use default
    if ($value$plusargs("PROGRAM=%s", program_file)) begin
      $display("========================================");
      $display("NeoCore 16x32 Generic Program Test");
      $display("Program file: %s (from +PROGRAM=)", program_file);
      if (debug_enabled) $display("DEBUG MODE ENABLED");
      $display("========================================\n");
    end else begin
      program_file = `PROGRAM_FILE;
      $display("========================================");
      $display("NeoCore 16x32 Generic Program Test");
      $display("Program file: %s (default)", program_file);
      if (debug_enabled) $display("DEBUG MODE ENABLED");
      $display("========================================\n");
    end
    
    // Initialize
    rst = 1;
    @(posedge clk);
    @(posedge clk);
    rst = 0;
    
    $display("Loading program into memory...");
    
    // Initialize all memory to zero
    for (int i = 0; i < 65536; i++) begin
      memory.mem[i] = 8'h00;
    end
    
    // Load program from hex file
    fd = $fopen(program_file, "r");
    if (fd == 0) begin
      $display("ERROR: Could not open program file: %s", program_file);
      $finish;
    end
    
    addr = 0;
    bytes_loaded = 0;
    while (!$feof(fd)) begin
      if ($fscanf(fd, "%h", byte_val) == 1) begin
        memory.mem[addr] = byte_val[7:0];
        addr = addr + 1;
        bytes_loaded = bytes_loaded + 1;
      end
    end
    $fclose(fd);
    
    $display("Loaded %0d bytes from %s", bytes_loaded, program_file);
    $display("Starting execution...\n");
    
    // Run until halt or timeout
    fork
      begin
        wait(halted);
        // Wait a couple more cycles for pipeline to drain
        repeat(3) @(posedge clk);
        
        $display("\n========================================");
        $display("Program halted at PC = 0x%08h", current_pc);
        $display("Total cycles: %0d", cycle_count);
        $display("Dual-issue cycles: %0d (%.1f%%)", dual_issue_count, 
                 100.0 * dual_issue_count / cycle_count);
        $display("========================================");
        
        // Dump all register values in hex format
        $display("\nRegister Dump (hex):");
        $display("========================================");
        for (int i = 0; i < 16; i++) begin
          $display("R%2d = 0x%04h", i, dut.regfile.registers[i]);
        end
        $display("========================================");
        
        $finish;
      end
      begin
        repeat(100000) @(posedge clk);
        $display("\n========================================");
        $display("ERROR: Test timeout after %0d cycles", cycle_count);
        $display("PC = 0x%08h, Halted = %b", current_pc, halted);
        $display("========================================");
        
        // Dump registers even on timeout
        $display("\nRegister state at timeout (hex):");
        $display("========================================");
        for (int i = 0; i < 16; i++) begin
          $display("R%2d = 0x%04h", i, dut.regfile.registers[i]);
        end
        $display("========================================");
        
        $finish;
      end
    join_any
  end

endmodule
