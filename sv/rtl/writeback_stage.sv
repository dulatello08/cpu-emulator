//
// writeback_stage.sv
// NeoCore 16x32 CPU - Write-Back Stage
//
// Writes results back to the register file.
// Updates CPU flags.
// Minimal logic - mostly just passing data through.
//

module writeback_stage
  import neocore_pkg::*;
(
  input  logic        clk,
  input  logic        rst,
  
  // From MEM/WB register (instruction 0)
  input  mem_wb_t     mem_wb_0,
  
  // From MEM/WB register (instruction 1)
  input  mem_wb_t     mem_wb_1,
  
  // Register file write ports
  output logic [3:0]  rf_wr_addr_0,
  output logic [15:0] rf_wr_data_0,
  output logic        rf_wr_en_0,
  
  output logic [3:0]  rf_wr_addr_1,
  output logic [15:0] rf_wr_data_1,
  output logic        rf_wr_en_1,
  
  // Flag updates
  output logic        z_flag_update,
  output logic        z_flag_value,
  output logic        v_flag_update,
  output logic        v_flag_value,
  
  // Halt signal
  output logic        halted
);

  // ============================================================================
  // Register Write-Back
  // ============================================================================
  
  // Write port 0 (from instruction 0's primary destination)
  always_comb begin
    rf_wr_addr_0 = mem_wb_0.rd_addr;
    rf_wr_data_0 = mem_wb_0.wb_data;
    rf_wr_en_0 = mem_wb_0.valid && mem_wb_0.rd_we;
  end
  
  // Write port 1 (can be from instruction 0's secondary dest or instruction 1's primary)
  // Priority: instruction 0's rd2 > instruction 1's rd
  always_comb begin
    if (mem_wb_0.valid && mem_wb_0.rd2_we) begin
      // Instruction 0 has a second destination (e.g., UMULL/SMULL high result)
      rf_wr_addr_1 = mem_wb_0.rd2_addr;
      rf_wr_data_1 = mem_wb_0.wb_data2;
      rf_wr_en_1 = 1'b1;
    end else if (mem_wb_1.valid && mem_wb_1.rd_we) begin
      // Instruction 1's primary destination
      rf_wr_addr_1 = mem_wb_1.rd_addr;
      rf_wr_data_1 = mem_wb_1.wb_data;
      rf_wr_en_1 = 1'b1;
    end else begin
      rf_wr_addr_1 = 4'h0;
      rf_wr_data_1 = 16'h0;
      rf_wr_en_1 = 1'b0;
    end
  end
  
  // ============================================================================
  // Flag Updates
  // ============================================================================
  
  // Update flags based on the most recent valid instruction
  // Priority: instruction 1 > instruction 0
  always_comb begin
    if (mem_wb_1.valid) begin
      z_flag_update = 1'b1;
      z_flag_value = mem_wb_1.z_flag;
      v_flag_update = 1'b1;
      v_flag_value = mem_wb_1.v_flag;
    end else if (mem_wb_0.valid) begin
      z_flag_update = 1'b1;
      z_flag_value = mem_wb_0.z_flag;
      v_flag_update = 1'b1;
      v_flag_value = mem_wb_0.v_flag;
    end else begin
      z_flag_update = 1'b0;
      z_flag_value = 1'b0;
      v_flag_update = 1'b0;
      v_flag_value = 1'b0;
    end
  end
  
  // ============================================================================
  // Halt Detection
  // ============================================================================
  
  // Halt if either instruction is a halt instruction
  assign halted = (mem_wb_0.valid && mem_wb_0.is_halt) ||
                  (mem_wb_1.valid && mem_wb_1.is_halt);

endmodule : writeback_stage
