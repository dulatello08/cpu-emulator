//
// hazard_unit.sv
// NeoCore 16x32 CPU - Hazard Detection and Forwarding Unit
//
// Detects data hazards (RAW dependencies) and generates forwarding control signals.
// Also detects load-use hazards that require pipeline stalls.
// Extended for dual-issue: checks hazards between both issuing instructions
// and with in-flight instructions.
//

module hazard_unit
  import neocore_pkg::*;
(
  input  logic        clk,
  input  logic        rst,
  
  // Instructions in ID stage (dual-issue)
  input  logic [3:0]  id_rs1_addr_0,
  input  logic [3:0]  id_rs2_addr_0,
  input  logic        id_valid_0,
  
  input  logic [3:0]  id_rs1_addr_1,
  input  logic [3:0]  id_rs2_addr_1,
  input  logic        id_valid_1,
  
  // Instruction in EX stage
  input  logic [3:0]  ex_rd_addr_0,
  input  logic        ex_rd_we_0,
  input  logic        ex_mem_read_0,
  input  logic [3:0]  ex_rd_addr_1,
  input  logic        ex_rd_we_1,
  input  logic        ex_mem_read_1,
  input  logic        ex_valid_0,
  input  logic        ex_valid_1,
  
  // Instruction in MEM stage
  input  logic [3:0]  mem_rd_addr_0,
  input  logic        mem_rd_we_0,
  input  logic [3:0]  mem_rd_addr_1,
  input  logic        mem_rd_we_1,
  input  logic        mem_valid_0,
  input  logic        mem_valid_1,
  
  // Instruction in WB stage
  input  logic [3:0]  wb_rd_addr_0,
  input  logic        wb_rd_we_0,
  input  logic [3:0]  wb_rd_addr_1,
  input  logic        wb_rd_we_1,
  input  logic        wb_valid_0,
  input  logic        wb_valid_1,
  
  // Forwarding control for instruction 0
  output logic [2:0]  forward_a_0,  // 000=none, 001=EX0, 010=EX1, 011=MEM0, 100=MEM1, 101=WB0, 110=WB1
  output logic [2:0]  forward_b_0,
  
  // Forwarding control for instruction 1
  output logic [2:0]  forward_a_1,
  output logic [2:0]  forward_b_1,
  
  // Stall control
  output logic        stall,         // Stall pipeline
  output logic        flush_id,      // Flush ID stage
  output logic        flush_ex       // Flush EX stage
);

  // ============================================================================
  // Forwarding Logic for Instruction 0
  // ============================================================================
  
  always_comb begin
    forward_a_0 = 3'b000;  // Default: no forwarding
    forward_b_0 = 3'b000;
    
    // Forward operand A for instruction 0
    if (id_valid_0 && (id_rs1_addr_0 != 4'h0)) begin
      // Priority: EX > MEM > WB (most recent first)
      // Check EX stage slot 0
      if (ex_valid_0 && ex_rd_we_0 && (ex_rd_addr_0 == id_rs1_addr_0)) begin
        forward_a_0 = 3'b001;  // Forward from EX slot 0
      end
      // Check EX stage slot 1
      else if (ex_valid_1 && ex_rd_we_1 && (ex_rd_addr_1 == id_rs1_addr_0)) begin
        forward_a_0 = 3'b010;  // Forward from EX slot 1
      end
      // Check MEM stage slot 0
      else if (mem_valid_0 && mem_rd_we_0 && (mem_rd_addr_0 == id_rs1_addr_0)) begin
        forward_a_0 = 3'b011;  // Forward from MEM slot 0
      end
      // Check MEM stage slot 1
      else if (mem_valid_1 && mem_rd_we_1 && (mem_rd_addr_1 == id_rs1_addr_0)) begin
        forward_a_0 = 3'b100;  // Forward from MEM slot 1
      end
      // Check WB stage slot 0
      else if (wb_valid_0 && wb_rd_we_0 && (wb_rd_addr_0 == id_rs1_addr_0)) begin
        forward_a_0 = 3'b101;  // Forward from WB slot 0
      end
      // Check WB stage slot 1
      else if (wb_valid_1 && wb_rd_we_1 && (wb_rd_addr_1 == id_rs1_addr_0)) begin
        forward_a_0 = 3'b110;  // Forward from WB slot 1
      end
    end
    
    // Forward operand B for instruction 0
    if (id_valid_0 && (id_rs2_addr_0 != 4'h0)) begin
      if (ex_valid_0 && ex_rd_we_0 && (ex_rd_addr_0 == id_rs2_addr_0)) begin
        forward_b_0 = 3'b001;
      end
      else if (ex_valid_1 && ex_rd_we_1 && (ex_rd_addr_1 == id_rs2_addr_0)) begin
        forward_b_0 = 3'b010;
      end
      else if (mem_valid_0 && mem_rd_we_0 && (mem_rd_addr_0 == id_rs2_addr_0)) begin
        forward_b_0 = 3'b011;
      end
      else if (mem_valid_1 && mem_rd_we_1 && (mem_rd_addr_1 == id_rs2_addr_0)) begin
        forward_b_0 = 3'b100;
      end
      else if (wb_valid_0 && wb_rd_we_0 && (wb_rd_addr_0 == id_rs2_addr_0)) begin
        forward_b_0 = 3'b101;
      end
      else if (wb_valid_1 && wb_rd_we_1 && (wb_rd_addr_1 == id_rs2_addr_0)) begin
        forward_b_0 = 3'b110;
      end
    end
  end
  
  // ============================================================================
  // Forwarding Logic for Instruction 1
  // ============================================================================
  
  always_comb begin
    forward_a_1 = 3'b000;
    forward_b_1 = 3'b000;
    
    // Forward operand A for instruction 1
    if (id_valid_1 && (id_rs1_addr_1 != 4'h0)) begin
      if (ex_valid_0 && ex_rd_we_0 && (ex_rd_addr_0 == id_rs1_addr_1)) begin
        forward_a_1 = 3'b001;
      end
      else if (ex_valid_1 && ex_rd_we_1 && (ex_rd_addr_1 == id_rs1_addr_1)) begin
        forward_a_1 = 3'b010;
      end
      else if (mem_valid_0 && mem_rd_we_0 && (mem_rd_addr_0 == id_rs1_addr_1)) begin
        forward_a_1 = 3'b011;
      end
      else if (mem_valid_1 && mem_rd_we_1 && (mem_rd_addr_1 == id_rs1_addr_1)) begin
        forward_a_1 = 3'b100;
      end
      else if (wb_valid_0 && wb_rd_we_0 && (wb_rd_addr_0 == id_rs1_addr_1)) begin
        forward_a_1 = 3'b101;
      end
      else if (wb_valid_1 && wb_rd_we_1 && (wb_rd_addr_1 == id_rs1_addr_1)) begin
        forward_a_1 = 3'b110;
      end
    end
    
    // Forward operand B for instruction 1
    if (id_valid_1 && (id_rs2_addr_1 != 4'h0)) begin
      if (ex_valid_0 && ex_rd_we_0 && (ex_rd_addr_0 == id_rs2_addr_1)) begin
        forward_b_1 = 3'b001;
      end
      else if (ex_valid_1 && ex_rd_we_1 && (ex_rd_addr_1 == id_rs2_addr_1)) begin
        forward_b_1 = 3'b010;
      end
      else if (mem_valid_0 && mem_rd_we_0 && (mem_rd_addr_0 == id_rs2_addr_1)) begin
        forward_b_1 = 3'b011;
      end
      else if (mem_valid_1 && mem_rd_we_1 && (mem_rd_addr_1 == id_rs2_addr_1)) begin
        forward_b_1 = 3'b100;
      end
      else if (wb_valid_0 && wb_rd_we_0 && (wb_rd_addr_0 == id_rs2_addr_1)) begin
        forward_b_1 = 3'b101;
      end
      else if (wb_valid_1 && wb_rd_we_1 && (wb_rd_addr_1 == id_rs2_addr_1)) begin
        forward_b_1 = 3'b110;
      end
    end
  end
  
  // ============================================================================
  // Load-Use Hazard Detection
  // ============================================================================
  
  logic load_use_hazard_0;
  logic load_use_hazard_1;
  
  always_comb begin
    load_use_hazard_0 = 1'b0;
    load_use_hazard_1 = 1'b0;
    
    // Check if instruction 0 in ID needs data from load in EX stage
    if (id_valid_0) begin
      if (ex_valid_0 && ex_mem_read_0) begin
        if ((ex_rd_addr_0 == id_rs1_addr_0) || (ex_rd_addr_0 == id_rs2_addr_0)) begin
          load_use_hazard_0 = 1'b1;
        end
      end
      if (ex_valid_1 && ex_mem_read_1) begin
        if ((ex_rd_addr_1 == id_rs1_addr_0) || (ex_rd_addr_1 == id_rs2_addr_0)) begin
          load_use_hazard_0 = 1'b1;
        end
      end
    end
    
    // Check if instruction 1 in ID needs data from load in EX stage
    if (id_valid_1) begin
      if (ex_valid_0 && ex_mem_read_0) begin
        if ((ex_rd_addr_0 == id_rs1_addr_1) || (ex_rd_addr_0 == id_rs2_addr_1)) begin
          load_use_hazard_1 = 1'b1;
        end
      end
      if (ex_valid_1 && ex_mem_read_1) begin
        if ((ex_rd_addr_1 == id_rs1_addr_1) || (ex_rd_addr_1 == id_rs2_addr_1)) begin
          load_use_hazard_1 = 1'b1;
        end
      end
    end
  end
  
  // ============================================================================
  // Stall and Flush Control
  // ============================================================================
  
  always_comb begin
    stall = 1'b0;
    flush_id = 1'b0;
    flush_ex = 1'b0;
    
    // Stall on load-use hazard
    if (load_use_hazard_0 || load_use_hazard_1) begin
      stall = 1'b1;
      flush_ex = 1'b1;  // Insert bubble in EX stage
    end
  end

endmodule : hazard_unit
