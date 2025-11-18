//
// decode_unit_tb.sv
// Testbench for Decode Unit module
//
// Tests instruction decoding for key instruction types.
// Updated for big-endian, 104-bit (13-byte) instruction format.
//

`timescale 1ns/1ps

module decode_unit_tb;
  import neocore_pkg::*;

  // Testbench signals
  logic        clk;
  logic        rst;
  logic [103:0] inst_data;  // 13 bytes max (104 bits)
  logic [3:0]  inst_len;
  logic [31:0] pc;
  logic        valid_in;
  logic        valid_out;
  opcode_e     opcode;
  logic [7:0]  specifier;
  itype_e      itype;
  alu_op_e     alu_op;
  logic [3:0]  rs1_addr;
  logic [3:0]  rs2_addr;
  logic [3:0]  rd_addr;
  logic [3:0]  rd2_addr;
  logic [31:0] immediate;
  logic [31:0] mem_addr;
  logic [31:0] branch_target;
  logic        rd_we;
  logic        rd2_we;
  logic        mem_read;
  logic        mem_write;
  mem_size_e   mem_size;
  logic        is_branch;
  logic        is_jsr;
  logic        is_rts;
  logic        is_halt;
  
  // Instantiate decode unit
  decode_unit dut (
    .clk(clk),
    .rst(rst),
    .inst_data(inst_data),
    .inst_len(inst_len),
    .pc(pc),
    .valid_in(valid_in),
    .valid_out(valid_out),
    .opcode(opcode),
    .specifier(specifier),
    .itype(itype),
    .alu_op(alu_op),
    .rs1_addr(rs1_addr),
    .rs2_addr(rs2_addr),
    .rd_addr(rd_addr),
    .rd2_addr(rd2_addr),
    .immediate(immediate),
    .mem_addr(mem_addr),
    .branch_target(branch_target),
    .rd_we(rd_we),
    .rd2_we(rd2_we),
    .mem_read(mem_read),
    .mem_write(mem_write),
    .mem_size(mem_size),
    .is_branch(is_branch),
    .is_jsr(is_jsr),
    .is_rts(is_rts),
    .is_halt(is_halt)
  );
  
  // Clock generation
  initial begin
    clk = 0;
    forever #5 clk = ~clk;
  end
  
  // Test stimulus
  initial begin
    $display("========================================");
    $display("Decode Unit Testbench");
    $display("========================================");
    
    // Initialize
    rst = 1;
    inst_data = 104'h0;
    inst_len = 4'd0;
    pc = 32'h0000_0000;
    valid_in = 1'b0;
    @(posedge clk);
    rst = 0;
    @(posedge clk);
    
    // Test 1: NOP instruction
    $display("\nTest 1: NOP");
    // Big-Endian Format: byte0=spec, byte1=op
    // [sp(8)] [opcode(8)] = [00] [00]
    // Bit positions: [103:96][95:88]
    inst_data = 104'h00_00_00_00_00_00_00_00_00_00_00_00_00;
    inst_len = 4'd2;
    pc = 32'h0000_0100;
    valid_in = 1'b1;
    @(posedge clk);
    #1;
    assert(opcode == OP_NOP) else $error("Opcode mismatch");
    assert(itype == ITYPE_CTRL) else $error("Type mismatch");
    assert(valid_out == 1'b1) else $error("Valid mismatch");
    $display("  PASS: NOP decoded correctly");
    
    // Test 2: ADD immediate (rd = rd + imm)
    $display("\nTest 2: ADD R1, #0x1234");
    // Big-Endian: [sp][op][rd][imm_hi][imm_lo] = [00][01][01][12][34]
    // Bits: [103:96][95:88][87:80][79:72][71:64]
    inst_data = 104'h00_01_01_12_34_00_00_00_00_00_00_00_00;
    inst_len = 4'd5;
    valid_in = 1'b1;
    @(posedge clk);
    #1;
    assert(opcode == OP_ADD) else $error("Opcode mismatch");
    assert(specifier == 8'h00) else $error("Specifier mismatch");
    assert(itype == ITYPE_ALU) else $error("Type mismatch");
    assert(alu_op == ALU_ADD) else $error("ALU op mismatch");
    assert(rd_addr == 4'd1) else $error("rd mismatch");
    assert(rs1_addr == 4'd1) else $error("rs1 mismatch (should be rd)");
    assert(immediate == 32'h0000_1234) else $error("Immediate mismatch: got %h", immediate);
    assert(rd_we == 1'b1) else $error("rd_we should be 1");
    $display("  PASS: ADD immediate decoded");
    
    // Test 3: ADD register (rd = rd + rn)
    $display("\nTest 3: ADD R2, R3");
    // Big-Endian: [sp][op][rd][rn] = [01][01][02][03]
    inst_data = 104'h01_01_02_03_00_00_00_00_00_00_00_00_00;
    inst_len = 4'd4;
    @(posedge clk);
    #1;
    assert(opcode == OP_ADD) else $error("Opcode mismatch");
    assert(specifier == 8'h01) else $error("Specifier mismatch");
    assert(rd_addr == 4'd2) else $error("rd mismatch");
    assert(rs1_addr == 4'd3) else $error("rs1 (rn) mismatch");
    assert(rs2_addr == 4'd2) else $error("rs2 (rd) mismatch");
    $display("  PASS: ADD register decoded");
    
    // Test 4: MOV immediate 16-bit
    $display("\nTest 4: MOV R5, #0xABCD");
    // Big-Endian: [sp][op][rd][imm_hi][imm_lo] = [00][09][05][AB][CD]
    inst_data = 104'h00_09_05_AB_CD_00_00_00_00_00_00_00_00;
    inst_len = 4'd5;
    @(posedge clk);
    #1;
    assert(opcode == OP_MOV) else $error("Opcode mismatch");
    assert(specifier == 8'h00) else $error("Specifier mismatch");
    assert(itype == ITYPE_MOV) else $error("Type mismatch");
    assert(rd_addr == 4'd5) else $error("rd mismatch");
    assert(immediate == 32'h0000_ABCD) else $error("Immediate mismatch");
    assert(rd_we == 1'b1) else $error("rd_we should be 1");
    $display("  PASS: MOV immediate decoded");
    
    // Test 5: Unconditional branch
    $display("\nTest 5: B 0x12345678");
    // Big-Endian: [sp][op][addr3][addr2][addr1][addr0] = [00][0A][12][34][56][78]
    inst_data = 104'h00_0A_12_34_56_78_00_00_00_00_00_00_00;
    inst_len = 4'd6;
    @(posedge clk);
    #1;
    assert(opcode == OP_B) else $error("Opcode mismatch");
    assert(itype == ITYPE_BRANCH) else $error("Type mismatch");
    assert(is_branch == 1'b1) else $error("is_branch should be 1");
    assert(branch_target == 32'h1234_5678) else $error("Branch target mismatch: got %h", branch_target);
    $display("  PASS: B decoded with target 0x%08h", branch_target);
    
    // Test 6: BNE (branch if not equal)
    $display("\nTest 6: BNE R1, R2, 0xABCD0000");
    // Big-Endian: [sp][op][rd][rn][addr3][addr2][addr1][addr0] = [00][0C][01][02][AB][CD][00][00]
    inst_data = 104'h00_0C_01_02_AB_CD_00_00_00_00_00_00_00;
    inst_len = 4'd8;
    @(posedge clk);
    #1;
    assert(opcode == OP_BNE) else $error("Opcode mismatch");
    assert(is_branch == 1'b1) else $error("is_branch should be 1");
    assert(rs1_addr == 4'd1) else $error("rs1 mismatch");
    assert(rs2_addr == 4'd2) else $error("rs2 mismatch");
    assert(branch_target == 32'hABCD_0000) else $error("Branch target mismatch");
    $display("  PASS: BNE decoded");
    
    // Test 7: HLT instruction
    $display("\nTest 7: HLT");
    // Big-Endian: [sp][op] = [00][12]
    inst_data = 104'h00_12_00_00_00_00_00_00_00_00_00_00_00;
    inst_len = 4'd2;
    @(posedge clk);
    #1;
    assert(opcode == OP_HLT) else $error("Opcode mismatch");
    assert(is_halt == 1'b1) else $error("is_halt should be 1");
    $display("  PASS: HLT decoded");
    
    // Test 8: UMULL (unsigned multiply long)
    $display("\nTest 8: UMULL R1, R2, R3");
    // Big-Endian: [sp][op][rd][rn][rn1] = [00][10][01][02][03]
    inst_data = 104'h00_10_01_02_03_00_00_00_00_00_00_00_00;
    inst_len = 4'd5;
    @(posedge clk);
    #1;
    assert(opcode == OP_UMULL) else $error("Opcode mismatch");
    assert(itype == ITYPE_MUL) else $error("Type mismatch");
    assert(rd_addr == 4'd1) else $error("rd mismatch");
    assert(rd2_addr == 4'd2) else $error("rd2 mismatch");
    assert(rs2_addr == 4'd3) else $error("rs2 (rn1) mismatch");
    assert(rd_we == 1'b1) else $error("rd_we should be 1");
    assert(rd2_we == 1'b1) else $error("rd2_we should be 1");
    $display("  PASS: UMULL decoded");
    
    $display("\n========================================");
    $display("Decode Unit Testbench PASSED");
    $display("========================================\n");
    
    $finish;
  end

endmodule
