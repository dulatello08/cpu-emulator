//
// simple_memory.sv
// NeoCore 16x32 CPU - Simple Memory Model for Simulation
//
// Provides a simple synchronous memory interface for testing.
// Combines instruction and data memory into one address space.
// Not synthesizable for real hardware (uses $readmemh).
//

module simple_memory #(
  parameter MEM_SIZE = 65536  // 64KB memory
)(
  input  logic        clk,
  input  logic        rst,
  
  // Instruction fetch interface
  input  logic [31:0] imem_addr,
  input  logic        imem_req,
  output logic [63:0] imem_rdata,
  output logic        imem_ack,
  
  // Data memory interface
  input  logic [31:0] dmem_addr,
  input  logic [31:0] dmem_wdata,
  input  logic [1:0]  dmem_size,   // 00=byte, 01=half, 10=word
  input  logic        dmem_we,
  input  logic        dmem_req,
  output logic [31:0] dmem_rdata,
  output logic        dmem_ack
);

  // Memory storage (byte-addressable)
  logic [7:0] mem [0:MEM_SIZE-1];
  
  // Initialize memory (can be loaded from file in testbench)
  initial begin
    for (int i = 0; i < MEM_SIZE; i++) begin
      mem[i] = 8'h00;
    end
  end
  
  // ============================================================================
  // Instruction Fetch
  // ============================================================================
  
  always_ff @(posedge clk) begin
    if (rst) begin
      imem_rdata <= 64'h0;
      imem_ack <= 1'b0;
    end else if (imem_req) begin
      // Read 8 bytes for instruction fetch
      if (imem_addr < (MEM_SIZE - 8)) begin
        imem_rdata <= {
          mem[imem_addr + 7],
          mem[imem_addr + 6],
          mem[imem_addr + 5],
          mem[imem_addr + 4],
          mem[imem_addr + 3],
          mem[imem_addr + 2],
          mem[imem_addr + 1],
          mem[imem_addr + 0]
        };
        imem_ack <= 1'b1;
      end else begin
        imem_rdata <= 64'h0;
        imem_ack <= 1'b0;
      end
    end else begin
      imem_ack <= 1'b0;
    end
  end
  
  // ============================================================================
  // Data Memory Access
  // ============================================================================
  
  always_ff @(posedge clk) begin
    if (rst) begin
      dmem_rdata <= 32'h0;
      dmem_ack <= 1'b0;
    end else if (dmem_req) begin
      if (dmem_we) begin
        // Write
        case (dmem_size)
          2'b00: begin  // Byte
            if (dmem_addr < MEM_SIZE) begin
              mem[dmem_addr] <= dmem_wdata[7:0];
            end
          end
          2'b01: begin  // Halfword
            if (dmem_addr < (MEM_SIZE - 1)) begin
              mem[dmem_addr + 0] <= dmem_wdata[7:0];
              mem[dmem_addr + 1] <= dmem_wdata[15:8];
            end
          end
          2'b10: begin  // Word
            if (dmem_addr < (MEM_SIZE - 3)) begin
              mem[dmem_addr + 0] <= dmem_wdata[7:0];
              mem[dmem_addr + 1] <= dmem_wdata[15:8];
              mem[dmem_addr + 2] <= dmem_wdata[23:16];
              mem[dmem_addr + 3] <= dmem_wdata[31:24];
            end
          end
          default: begin
            // Invalid size
          end
        endcase
        dmem_ack <= 1'b1;
      end else begin
        // Read
        case (dmem_size)
          2'b00: begin  // Byte
            if (dmem_addr < MEM_SIZE) begin
              dmem_rdata <= {24'h0, mem[dmem_addr]};
            end else begin
              dmem_rdata <= 32'h0;
            end
          end
          2'b01: begin  // Halfword
            if (dmem_addr < (MEM_SIZE - 1)) begin
              dmem_rdata <= {16'h0, mem[dmem_addr + 1], mem[dmem_addr + 0]};
            end else begin
              dmem_rdata <= 32'h0;
            end
          end
          2'b10: begin  // Word
            if (dmem_addr < (MEM_SIZE - 3)) begin
              dmem_rdata <= {
                mem[dmem_addr + 3],
                mem[dmem_addr + 2],
                mem[dmem_addr + 1],
                mem[dmem_addr + 0]
              };
            end else begin
              dmem_rdata <= 32'h0;
            end
          end
          default: begin
            dmem_rdata <= 32'h0;
          end
        endcase
        dmem_ack <= 1'b1;
      end
    end else begin
      dmem_ack <= 1'b0;
    end
  end
  
  // ============================================================================
  // Helper task to load memory from hex file
  // ============================================================================
  
  task load_hex(input string filename);
    $readmemh(filename, mem);
  endtask
  
  // Helper task to dump memory to console
  task dump_range(input int start_addr, input int end_addr);
    for (int i = start_addr; i < end_addr; i += 16) begin
      $display("0x%08x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
               i,
               mem[i+0], mem[i+1], mem[i+2], mem[i+3],
               mem[i+4], mem[i+5], mem[i+6], mem[i+7],
               mem[i+8], mem[i+9], mem[i+10], mem[i+11],
               mem[i+12], mem[i+13], mem[i+14], mem[i+15]);
    end
  endtask

endmodule : simple_memory
