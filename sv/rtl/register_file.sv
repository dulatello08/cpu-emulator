//
// register_file.sv
// NeoCore 16x32 CPU - Register File
//
// 16 general-purpose registers, each 16 bits wide.
// Supports dual-issue: 4 read ports, 2 write ports.
// Provides internal forwarding to minimize hazards.
//

module register_file
  import neocore_pkg::*;
(
  input  logic        clk,
  input  logic        rst,
  
  // Read ports for instruction 0
  input  logic [3:0]  rs1_addr_0,
  input  logic [3:0]  rs2_addr_0,
  output logic [15:0] rs1_data_0,
  output logic [15:0] rs2_data_0,
  
  // Read ports for instruction 1 (dual-issue)
  input  logic [3:0]  rs1_addr_1,
  input  logic [3:0]  rs2_addr_1,
  output logic [15:0] rs1_data_1,
  output logic [15:0] rs2_data_1,
  
  // Write ports
  input  logic [3:0]  rd_addr_0,
  input  logic [15:0] rd_data_0,
  input  logic        rd_we_0,
  
  input  logic [3:0]  rd_addr_1,
  input  logic [15:0] rd_data_1,
  input  logic        rd_we_1
);

  // ============================================================================
  // Register Storage
  // ============================================================================
  
  logic [15:0] registers [0:15];

  // ============================================================================
  // Write Logic (Sequential)
  // ============================================================================
  
  always_ff @(posedge clk) begin
    if (rst) begin
      // On reset, initialize all registers to 0
      // Note: In actual hardware, registers may contain garbage after reset.
      // We initialize to 0 for simulation convenience.
      for (int i = 0; i < NUM_REGS; i++) begin
        registers[i] <= 16'h0000;
      end
    end else begin
      // Write port 0
      if (rd_we_0) begin
        registers[rd_addr_0] <= rd_data_0;
      end
      
      // Write port 1
      // If both ports write to same register, port 1 takes precedence
      // (This should be prevented by issue logic, but we handle it here)
      if (rd_we_1) begin
        registers[rd_addr_1] <= rd_data_1;
      end
    end
  end

  // ============================================================================
  // Read Logic (Combinational with Forwarding)
  // ============================================================================
  
  // Read port 0, source 1
  always_comb begin
    // Check for write-to-read forwarding (bypass)
    if (rd_we_0 && (rd_addr_0 == rs1_addr_0)) begin
      rs1_data_0 = rd_data_0;
    end else if (rd_we_1 && (rd_addr_1 == rs1_addr_0)) begin
      rs1_data_0 = rd_data_1;
    end else begin
      rs1_data_0 = registers[rs1_addr_0];
    end
  end
  
  // Read port 0, source 2
  always_comb begin
    if (rd_we_0 && (rd_addr_0 == rs2_addr_0)) begin
      rs2_data_0 = rd_data_0;
    end else if (rd_we_1 && (rd_addr_1 == rs2_addr_0)) begin
      rs2_data_0 = rd_data_1;
    end else begin
      rs2_data_0 = registers[rs2_addr_0];
    end
  end
  
  // Read port 1, source 1
  always_comb begin
    if (rd_we_0 && (rd_addr_0 == rs1_addr_1)) begin
      rs1_data_1 = rd_data_0;
    end else if (rd_we_1 && (rd_addr_1 == rs1_addr_1)) begin
      rs1_data_1 = rd_data_1;
    end else begin
      rs1_data_1 = registers[rs1_addr_1];
    end
  end
  
  // Read port 1, source 2
  always_comb begin
    if (rd_we_0 && (rd_addr_0 == rs2_addr_1)) begin
      rs2_data_1 = rd_data_0;
    end else if (rd_we_1 && (rd_addr_1 == rs2_addr_1)) begin
      rs2_data_1 = rd_data_1;
    end else begin
      rs2_data_1 = registers[rs2_addr_1];
    end
  end

endmodule : register_file
