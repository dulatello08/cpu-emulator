//
// pipeline_regs.sv
// NeoCore 16x32 CPU - Pipeline Stage Registers
//
// Contains all pipeline registers for the 5-stage pipeline.
// Each register has enable/stall capability and flush capability.
//

module if_id_reg
  import neocore_pkg::*;
(
  input  logic   clk,
  input  logic   rst,
  input  logic   stall,      // Stall this stage
  input  logic   flush,      // Flush this stage
  input  if_id_t data_in,
  output if_id_t data_out
);

  always_ff @(posedge clk) begin
    if (rst || flush) begin
      data_out.valid <= 1'b0;
      data_out.pc <= 32'h0;
      data_out.inst_data <= 72'h0;
      data_out.inst_len <= 4'h0;
    end else if (!stall) begin
      data_out <= data_in;
    end
    // else: stalled, keep current value
  end

endmodule : if_id_reg

module id_ex_reg
  import neocore_pkg::*;
(
  input  logic   clk,
  input  logic   rst,
  input  logic   stall,
  input  logic   flush,
  input  id_ex_t data_in,
  output id_ex_t data_out
);

  always_ff @(posedge clk) begin
    if (rst || flush) begin
      data_out.valid <= 1'b0;
      data_out.pc <= 32'h0;
      data_out.opcode <= OP_NOP;
      data_out.specifier <= 8'h00;
      data_out.itype <= ITYPE_CTRL;
      data_out.alu_op <= ALU_NOP;
      data_out.rs1_addr <= 4'h0;
      data_out.rs2_addr <= 4'h0;
      data_out.rs1_data <= 16'h0;
      data_out.rs2_data <= 16'h0;
      data_out.immediate <= 32'h0;
      data_out.rd_addr <= 4'h0;
      data_out.rd2_addr <= 4'h0;
      data_out.rd_we <= 1'b0;
      data_out.rd2_we <= 1'b0;
      data_out.mem_read <= 1'b0;
      data_out.mem_write <= 1'b0;
      data_out.mem_size <= MEM_HALF;
      data_out.is_branch <= 1'b0;
      data_out.is_jsr <= 1'b0;
      data_out.is_rts <= 1'b0;
      data_out.is_halt <= 1'b0;
    end else if (!stall) begin
      data_out <= data_in;
    end
  end

endmodule : id_ex_reg

module ex_mem_reg
  import neocore_pkg::*;
(
  input  logic    clk,
  input  logic    rst,
  input  logic    stall,
  input  logic    flush,
  input  ex_mem_t data_in,
  output ex_mem_t data_out
);

  always_ff @(posedge clk) begin
    if (rst || flush) begin
      data_out.valid <= 1'b0;
      data_out.pc <= 32'h0;
      data_out.alu_result <= 32'h0;
      data_out.z_flag <= 1'b0;
      data_out.v_flag <= 1'b0;
      data_out.rd_addr <= 4'h0;
      data_out.rd2_addr <= 4'h0;
      data_out.rd_we <= 1'b0;
      data_out.rd2_we <= 1'b0;
      data_out.mem_read <= 1'b0;
      data_out.mem_write <= 1'b0;
      data_out.mem_size <= MEM_HALF;
      data_out.mem_addr <= 32'h0;
      data_out.mem_wdata <= 16'h0;
      data_out.branch_taken <= 1'b0;
      data_out.branch_target <= 32'h0;
      data_out.is_halt <= 1'b0;
    end else if (!stall) begin
      data_out <= data_in;
    end
  end

endmodule : ex_mem_reg

module mem_wb_reg
  import neocore_pkg::*;
(
  input  logic    clk,
  input  logic    rst,
  input  logic    stall,
  input  logic    flush,
  input  mem_wb_t data_in,
  output mem_wb_t data_out
);

  always_ff @(posedge clk) begin
    if (rst || flush) begin
      data_out.valid <= 1'b0;
      data_out.pc <= 32'h0;
      data_out.wb_data <= 16'h0;
      data_out.wb_data2 <= 16'h0;
      data_out.rd_addr <= 4'h0;
      data_out.rd2_addr <= 4'h0;
      data_out.rd_we <= 1'b0;
      data_out.rd2_we <= 1'b0;
      data_out.z_flag <= 1'b0;
      data_out.v_flag <= 1'b0;
      data_out.is_halt <= 1'b0;
    end else if (!stall) begin
      data_out <= data_in;
    end
  end

endmodule : mem_wb_reg
