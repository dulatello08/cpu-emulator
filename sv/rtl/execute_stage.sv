//
// execute_stage.sv
// NeoCore 16x32 CPU - Execute Stage
//
// Integrates ALU, multiply unit, and branch unit.
// Handles operand forwarding via MUXes.
// Supports dual-issue execution.
//

module execute_stage
  import neocore_pkg::*;
(
  input  logic        clk,
  input  logic        rst,
  
  // From ID/EX register (instruction 0)
  input  id_ex_t      id_ex_0,
  
  // From ID/EX register (instruction 1, dual-issue)
  input  id_ex_t      id_ex_1,
  
  // Forwarding data sources
  input  logic [15:0] ex_fwd_data_0,   // From EX slot 0 (self-forwarding)
  input  logic [15:0] ex_fwd_data_1,   // From EX slot 1
  input  logic [15:0] mem_fwd_data_0,  // From MEM slot 0
  input  logic [15:0] mem_fwd_data_1,  // From MEM slot 1
  input  logic [15:0] wb_fwd_data_0,   // From WB slot 0
  input  logic [15:0] wb_fwd_data_1,   // From WB slot 1
  
  // Forwarding control signals
  input  logic [2:0]  forward_a_0,
  input  logic [2:0]  forward_b_0,
  input  logic [2:0]  forward_a_1,
  input  logic [2:0]  forward_b_1,
  
  // Current flags
  input  logic        z_flag,
  input  logic        v_flag,
  
  // Outputs to EX/MEM register
  output ex_mem_t     ex_mem_0,
  output ex_mem_t     ex_mem_1,
  
  // Branch signals
  output logic        branch_taken,
  output logic [31:0] branch_target
);

  // ============================================================================
  // Operand Forwarding MUXes
  // ============================================================================
  
  logic [15:0] operand_a_0, operand_b_0;
  logic [15:0] operand_a_1, operand_b_1;
  
  // Forwarding MUX for instruction 0, operand A
  always_comb begin
    case (forward_a_0)
      3'b000:  operand_a_0 = id_ex_0.rs1_data;
      3'b001:  operand_a_0 = ex_fwd_data_0;
      3'b010:  operand_a_0 = ex_fwd_data_1;
      3'b011:  operand_a_0 = mem_fwd_data_0;
      3'b100:  operand_a_0 = mem_fwd_data_1;
      3'b101:  operand_a_0 = wb_fwd_data_0;
      3'b110:  operand_a_0 = wb_fwd_data_1;
      default: operand_a_0 = id_ex_0.rs1_data;
    endcase
  end
  
  // Forwarding MUX for instruction 0, operand B
  always_comb begin
    case (forward_b_0)
      3'b000:  operand_b_0 = id_ex_0.rs2_data;
      3'b001:  operand_b_0 = ex_fwd_data_0;
      3'b010:  operand_b_0 = ex_fwd_data_1;
      3'b011:  operand_b_0 = mem_fwd_data_0;
      3'b100:  operand_b_0 = mem_fwd_data_1;
      3'b101:  operand_b_0 = wb_fwd_data_0;
      3'b110:  operand_b_0 = wb_fwd_data_1;
      default: operand_b_0 = id_ex_0.rs2_data;
    endcase
  end
  
  // Forwarding MUX for instruction 1, operand A
  always_comb begin
    case (forward_a_1)
      3'b000:  operand_a_1 = id_ex_1.rs1_data;
      3'b001:  operand_a_1 = ex_fwd_data_0;
      3'b010:  operand_a_1 = ex_fwd_data_1;
      3'b011:  operand_a_1 = mem_fwd_data_0;
      3'b100:  operand_a_1 = mem_fwd_data_1;
      3'b101:  operand_a_1 = wb_fwd_data_0;
      3'b110:  operand_a_1 = wb_fwd_data_1;
      default: operand_a_1 = id_ex_1.rs1_data;
    endcase
  end
  
  // Forwarding MUX for instruction 1, operand B
  always_comb begin
    case (forward_b_1)
      3'b000:  operand_b_1 = id_ex_1.rs2_data;
      3'b001:  operand_b_1 = ex_fwd_data_0;
      3'b010:  operand_b_1 = ex_fwd_data_1;
      3'b011:  operand_b_1 = mem_fwd_data_0;
      3'b100:  operand_b_1 = mem_fwd_data_1;
      3'b101:  operand_b_1 = wb_fwd_data_0;
      3'b110:  operand_b_1 = wb_fwd_data_1;
      default: operand_b_1 = id_ex_1.rs2_data;
    endcase
  end
  
  // ============================================================================
  // ALU Execution (Instruction 0)
  // ============================================================================
  
  logic [31:0] alu_result_0;
  logic        alu_z_0, alu_v_0;
  logic [15:0] alu_op_b_0;
  
  // Select ALU operand B (register or immediate)
  always_comb begin
    if (id_ex_0.itype == ITYPE_ALU && id_ex_0.specifier == 8'h00) begin
      alu_op_b_0 = id_ex_0.immediate[15:0];  // Immediate mode
    end else begin
      alu_op_b_0 = operand_b_0;  // Register mode
    end
  end
  
  alu alu_0 (
    .clk(clk),
    .rst(rst),
    .operand_a(operand_a_0),
    .operand_b(alu_op_b_0),
    .alu_op(id_ex_0.alu_op),
    .result(alu_result_0),
    .z_flag(alu_z_0),
    .v_flag(alu_v_0)
  );
  
  // ============================================================================
  // ALU Execution (Instruction 1)
  // ============================================================================
  
  logic [31:0] alu_result_1;
  logic        alu_z_1, alu_v_1;
  logic [15:0] alu_op_b_1;
  
  always_comb begin
    if (id_ex_1.itype == ITYPE_ALU && id_ex_1.specifier == 8'h00) begin
      alu_op_b_1 = id_ex_1.immediate[15:0];
    end else begin
      alu_op_b_1 = operand_b_1;
    end
  end
  
  alu alu_1 (
    .clk(clk),
    .rst(rst),
    .operand_a(operand_a_1),
    .operand_b(alu_op_b_1),
    .alu_op(id_ex_1.alu_op),
    .result(alu_result_1),
    .z_flag(alu_z_1),
    .v_flag(alu_v_1)
  );
  
  // ============================================================================
  // Multiply Unit (Instruction 0)
  // ============================================================================
  
  logic [15:0] mul_result_lo_0, mul_result_hi_0;
  logic        is_signed_mul_0;
  
  assign is_signed_mul_0 = (id_ex_0.opcode == OP_SMULL);
  
  multiply_unit mul_0 (
    .clk(clk),
    .rst(rst),
    .operand_a(operand_a_0),
    .operand_b(operand_b_0),
    .is_signed(is_signed_mul_0),
    .result_lo(mul_result_lo_0),
    .result_hi(mul_result_hi_0)
  );
  
  // ============================================================================
  // Multiply Unit (Instruction 1)
  // ============================================================================
  
  logic [15:0] mul_result_lo_1, mul_result_hi_1;
  logic        is_signed_mul_1;
  
  assign is_signed_mul_1 = (id_ex_1.opcode == OP_SMULL);
  
  multiply_unit mul_1 (
    .clk(clk),
    .rst(rst),
    .operand_a(operand_a_1),
    .operand_b(operand_b_1),
    .is_signed(is_signed_mul_1),
    .result_lo(mul_result_lo_1),
    .result_hi(mul_result_hi_1)
  );
  
  // ============================================================================
  // Branch Unit (Instruction 0)
  // ============================================================================
  
  logic        branch_taken_0;
  logic [31:0] branch_pc_0;
  
  branch_unit branch_0 (
    .clk(clk),
    .rst(rst),
    .opcode(id_ex_0.opcode),
    .operand_a(operand_a_0),
    .operand_b(operand_b_0),
    .v_flag_in(v_flag),
    .branch_target(id_ex_0.immediate),
    .branch_taken(branch_taken_0),
    .branch_pc(branch_pc_0)
  );
  
  // ============================================================================
  // Branch Unit (Instruction 1)
  // ============================================================================
  
  logic        branch_taken_1;
  logic [31:0] branch_pc_1;
  
  branch_unit branch_1 (
    .clk(clk),
    .rst(rst),
    .opcode(id_ex_1.opcode),
    .operand_a(operand_a_1),
    .operand_b(operand_b_1),
    .v_flag_in(v_flag),
    .branch_target(id_ex_1.immediate),
    .branch_taken(branch_taken_1),
    .branch_pc(branch_pc_1)
  );
  
  // ============================================================================
  // Result Selection and Output (Instruction 0)
  // ============================================================================
  
  always_comb begin
    ex_mem_0.valid = id_ex_0.valid;
    ex_mem_0.pc = id_ex_0.pc;
    ex_mem_0.rd_addr = id_ex_0.rd_addr;
    ex_mem_0.rd2_addr = id_ex_0.rd2_addr;
    ex_mem_0.rd_we = id_ex_0.rd_we;
    ex_mem_0.rd2_we = id_ex_0.rd2_we;
    ex_mem_0.mem_read = id_ex_0.mem_read;
    ex_mem_0.mem_write = id_ex_0.mem_write;
    ex_mem_0.mem_size = id_ex_0.mem_size;
    ex_mem_0.is_halt = id_ex_0.is_halt;
    
    // Select result based on instruction type
    if (id_ex_0.itype == ITYPE_MUL) begin
      ex_mem_0.alu_result = {16'h0, mul_result_lo_0};
      // Store high result for rd2
    end else if (id_ex_0.itype == ITYPE_MOV && id_ex_0.specifier == 8'h02) begin
      // MOV register to register: pass through operand
      ex_mem_0.alu_result = {16'h0, operand_a_0};
    end else begin
      ex_mem_0.alu_result = alu_result_0;
    end
    
    // Flags
    ex_mem_0.z_flag = alu_z_0;
    ex_mem_0.v_flag = alu_v_0;
    
    // Memory address calculation
    if (id_ex_0.mem_read || id_ex_0.mem_write) begin
      // For memory operations, address is in immediate field
      ex_mem_0.mem_addr = id_ex_0.immediate;
    end else begin
      ex_mem_0.mem_addr = 32'h0;
    end
    
    // Memory write data
    ex_mem_0.mem_wdata = operand_a_0;
    
    // Branch information
    ex_mem_0.branch_taken = id_ex_0.is_branch && branch_taken_0;
    ex_mem_0.branch_target = branch_pc_0;
  end
  
  // ============================================================================
  // Result Selection and Output (Instruction 1)
  // ============================================================================
  
  always_comb begin
    ex_mem_1.valid = id_ex_1.valid;
    ex_mem_1.pc = id_ex_1.pc;
    ex_mem_1.rd_addr = id_ex_1.rd_addr;
    ex_mem_1.rd2_addr = id_ex_1.rd2_addr;
    ex_mem_1.rd_we = id_ex_1.rd_we;
    ex_mem_1.rd2_we = id_ex_1.rd2_we;
    ex_mem_1.mem_read = id_ex_1.mem_read;
    ex_mem_1.mem_write = id_ex_1.mem_write;
    ex_mem_1.mem_size = id_ex_1.mem_size;
    ex_mem_1.is_halt = id_ex_1.is_halt;
    
    if (id_ex_1.itype == ITYPE_MUL) begin
      ex_mem_1.alu_result = {16'h0, mul_result_lo_1};
    end else if (id_ex_1.itype == ITYPE_MOV && id_ex_1.specifier == 8'h02) begin
      ex_mem_1.alu_result = {16'h0, operand_a_1};
    end else begin
      ex_mem_1.alu_result = alu_result_1;
    end
    
    ex_mem_1.z_flag = alu_z_1;
    ex_mem_1.v_flag = alu_v_1;
    
    if (id_ex_1.mem_read || id_ex_1.mem_write) begin
      ex_mem_1.mem_addr = id_ex_1.immediate;
    end else begin
      ex_mem_1.mem_addr = 32'h0;
    end
    
    ex_mem_1.mem_wdata = operand_a_1;
    ex_mem_1.branch_taken = id_ex_1.is_branch && branch_taken_1;
    ex_mem_1.branch_target = branch_pc_1;
  end
  
  // ============================================================================
  // Global Branch Decision
  // ============================================================================
  
  always_comb begin
    // Priority: instruction 0's branch takes precedence
    if (ex_mem_0.branch_taken) begin
      branch_taken = 1'b1;
      branch_target = ex_mem_0.branch_target;
    end else if (ex_mem_1.branch_taken) begin
      branch_taken = 1'b1;
      branch_target = ex_mem_1.branch_target;
    end else begin
      branch_taken = 1'b0;
      branch_target = 32'h0;
    end
  end

endmodule : execute_stage
