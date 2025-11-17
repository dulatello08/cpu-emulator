//
// decode_unit.sv
// NeoCore 16x32 CPU - Instruction Decode Unit
//
// Decodes variable-length instructions and extracts:
// - Opcode and specifier
// - Source and destination register addresses
// - Immediate values
// - Memory addresses
// - Branch targets
//
// Handles all instruction formats in the NeoCore ISA.
//

module decode_unit
  import neocore_pkg::*;
(
  input  logic        clk,
  input  logic        rst,
  
  // Input instruction
  input  logic [71:0] inst_data,    // Up to 9 bytes of instruction
  input  logic [3:0]  inst_len,     // Instruction length
  input  logic [31:0] pc,           // Current PC
  input  logic        valid_in,     // Instruction valid
  
  // Decoded outputs
  output logic        valid_out,
  output opcode_e     opcode,
  output logic [7:0]  specifier,
  output itype_e      itype,
  output alu_op_e     alu_op,
  
  // Register addresses
  output logic [3:0]  rs1_addr,
  output logic [3:0]  rs2_addr,
  output logic [3:0]  rd_addr,
  output logic [3:0]  rd2_addr,
  
  // Immediate/address values
  output logic [31:0] immediate,
  output logic [31:0] mem_addr,
  output logic [31:0] branch_target,
  
  // Control signals
  output logic        rd_we,        // Destination register write enable
  output logic        rd2_we,       // Second destination write enable
  output logic        mem_read,
  output logic        mem_write,
  output mem_size_e   mem_size,
  output logic        is_branch,
  output logic        is_jsr,
  output logic        is_rts,
  output logic        is_halt
);

  // ============================================================================
  // Instruction Field Extraction
  // ============================================================================
  
  logic [7:0]  byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7, byte8;
  
  always_comb begin
    // Extract individual bytes from instruction data
    byte0 = inst_data[7:0];      // Specifier
    byte1 = inst_data[15:8];     // Opcode
    byte2 = inst_data[23:16];    // rd (often)
    byte3 = inst_data[31:24];    // rn or immediate low
    byte4 = inst_data[39:32];    // immediate high or rn1
    byte5 = inst_data[47:40];    // address byte 0
    byte6 = inst_data[55:48];    // address byte 1
    byte7 = inst_data[63:56];    // address byte 2
    byte8 = inst_data[71:64];    // address byte 3
  end
  
  // ============================================================================
  // Opcode and Type Decoding
  // ============================================================================
  
  opcode_e opcode_int;
  
  always_comb begin
    opcode_int = opcode_e'(byte1);
    opcode = opcode_int;
    specifier = byte0;
    itype = get_itype(opcode_int);
    alu_op = opcode_to_alu_op(opcode_int);
    valid_out = valid_in;
  end
  
  // ============================================================================
  // Register Address Decoding
  // ============================================================================
  
  always_comb begin
    // Default values
    rs1_addr = 4'h0;
    rs2_addr = 4'h0;
    rd_addr = 4'h0;
    rd2_addr = 4'h0;
    
    case (itype)
      ITYPE_ALU: begin
        // ALU operations: rd, rn
        rd_addr = byte2[3:0];
        case (specifier)
          8'h00: begin  // Immediate mode
            rs1_addr = byte2[3:0];  // rd is also source
            rs2_addr = 4'h0;
          end
          8'h01: begin  // Register mode
            rs1_addr = byte3[3:0];  // rn
            rs2_addr = byte2[3:0];  // rd is also source
          end
          8'h02: begin  // Memory mode
            rs1_addr = byte2[3:0];  // rd is also source
            rs2_addr = 4'h0;
          end
          default: begin
            rs1_addr = 4'h0;
            rs2_addr = 4'h0;
          end
        endcase
      end
      
      ITYPE_MOV: begin
        // MOV has many variants
        rd_addr = byte2[3:0];
        rd2_addr = byte3[3:0];  // For 32-bit moves
        case (specifier)
          8'h00: begin  // mov rd, #imm
            rs1_addr = 4'h0;
            rs2_addr = 4'h0;
          end
          8'h01: begin  // mov rd, rn, #imm32
            rs1_addr = 4'h0;
            rs2_addr = 4'h0;
          end
          8'h02: begin  // mov rd, rn (copy)
            rs1_addr = byte3[3:0];
            rs2_addr = 4'h0;
          end
          8'h03, 8'h04, 8'h05: begin  // Load from memory
            rs1_addr = 4'h0;
            rs2_addr = 4'h0;
          end
          8'h06: begin  // Load 32-bit from memory
            rs1_addr = 4'h0;
            rs2_addr = 4'h0;
          end
          8'h07, 8'h08, 8'h09: begin  // Store to memory
            rs1_addr = byte2[3:0];
            rs2_addr = 4'h0;
          end
          8'h0A: begin  // Store 32-bit to memory
            rs1_addr = byte2[3:0];
            rs2_addr = byte3[3:0];
          end
          8'h0B, 8'h0C, 8'h0D: begin  // Load with offset
            rs1_addr = byte3[3:0];  // rn (base)
            rs2_addr = 4'h0;
          end
          8'h0E: begin  // Load 32-bit with offset
            rs1_addr = byte3[3:0];
            rs2_addr = 4'h0;
          end
          8'h0F, 8'h10, 8'h11: begin  // Store with offset
            rs1_addr = byte2[3:0];
            rs2_addr = byte3[3:0];  // rn (base) is also source for address
          end
          8'h12: begin  // Store 32-bit with offset
            rs1_addr = byte2[3:0];
            rs2_addr = byte4[3:0];  // rn for address
          end
          default: begin
            rs1_addr = 4'h0;
            rs2_addr = 4'h0;
          end
        endcase
      end
      
      ITYPE_BRANCH: begin
        // Branches: compare rd and rn
        case (opcode_int)
          OP_B, OP_BRO: begin  // No comparison
            rs1_addr = 4'h0;
            rs2_addr = 4'h0;
            rd_addr = 4'h0;
          end
          default: begin  // BE, BNE, BLT, BGT
            rs1_addr = byte2[3:0];  // rd (first operand)
            rs2_addr = byte3[3:0];  // rn (second operand)
            rd_addr = 4'h0;
          end
        endcase
      end
      
      ITYPE_MUL: begin
        // UMULL/SMULL: rd, rn1, rn
        rd_addr = byte2[3:0];
        rd2_addr = byte3[3:0];  // rn1
        rs1_addr = byte2[3:0];  // rd is also source
        rs2_addr = byte4[3:0];  // rn
      end
      
      ITYPE_STACK: begin
        // PSH/POP: rd
        rd_addr = byte2[3:0];
        rs1_addr = byte2[3:0];  // For PSH, rd is source
        rs2_addr = 4'h0;
      end
      
      ITYPE_SUB: begin
        // JSR/RTS
        rs1_addr = 4'h0;
        rs2_addr = 4'h0;
        rd_addr = 4'h0;
      end
      
      default: begin
        rs1_addr = 4'h0;
        rs2_addr = 4'h0;
        rd_addr = 4'h0;
        rd2_addr = 4'h0;
      end
    endcase
  end
  
  // ============================================================================
  // Immediate and Address Decoding
  // ============================================================================
  
  always_comb begin
    // Default values
    immediate = 32'h0000_0000;
    mem_addr = 32'h0000_0000;
    branch_target = 32'h0000_0000;
    
    case (itype)
      ITYPE_ALU: begin
        case (specifier)
          8'h00: begin  // Immediate mode
            immediate = {16'h0000, byte3, byte4};
          end
          8'h02: begin  // Memory addressing
            mem_addr = {byte3, byte4, byte5, byte6};
          end
          default: begin
            immediate = 32'h0000_0000;
          end
        endcase
      end
      
      ITYPE_MOV: begin
        case (specifier)
          8'h00: begin  // Immediate 16-bit
            immediate = {16'h0000, byte3, byte4};
          end
          8'h01: begin  // Immediate 32-bit
            immediate = {byte4, byte5, byte6, byte7};
          end
          8'h03, 8'h04, 8'h05, 8'h07, 8'h08, 8'h09: begin  // Normal addressing
            mem_addr = {byte3, byte4, byte5, byte6};
          end
          8'h06, 8'h0A: begin  // Normal addressing for 32-bit
            mem_addr = {byte4, byte5, byte6, byte7};
          end
          8'h0B, 8'h0C, 8'h0D, 8'h0F, 8'h10, 8'h11: begin  // Offset addressing
            immediate = {byte4, byte5, byte6, byte7};  // Offset
          end
          8'h0E, 8'h12: begin  // Offset addressing for 32-bit
            immediate = {byte5, byte6, byte7, byte8};  // Offset
          end
          default: begin
            immediate = 32'h0000_0000;
          end
        endcase
      end
      
      ITYPE_BRANCH: begin
        case (opcode_int)
          OP_B, OP_BRO, OP_JSR: begin  // 6-byte branch
            branch_target = {byte2, byte3, byte4, byte5};
          end
          default: begin  // 8-byte conditional branch
            branch_target = {byte4, byte5, byte6, byte7};
          end
        endcase
      end
      
      default: begin
        immediate = 32'h0000_0000;
        mem_addr = 32'h0000_0000;
        branch_target = 32'h0000_0000;
      end
    endcase
  end
  
  // ============================================================================
  // Control Signal Generation
  // ============================================================================
  
  always_comb begin
    // Default values
    rd_we = 1'b0;
    rd2_we = 1'b0;
    mem_read = 1'b0;
    mem_write = 1'b0;
    mem_size = MEM_HALF;
    is_branch = 1'b0;
    is_jsr = 1'b0;
    is_rts = 1'b0;
    is_halt = 1'b0;
    
    case (itype)
      ITYPE_ALU: begin
        rd_we = 1'b1;
        if (specifier == 8'h02) begin
          mem_read = 1'b1;
          mem_size = MEM_HALF;
        end
      end
      
      ITYPE_MOV: begin
        case (specifier)
          8'h00, 8'h02: begin  // Register writes
            rd_we = 1'b1;
          end
          8'h01: begin  // 32-bit immediate
            rd_we = 1'b1;
            rd2_we = 1'b1;
          end
          8'h03: begin  // Load byte to rd.L
            rd_we = 1'b1;
            mem_read = 1'b1;
            mem_size = MEM_BYTE;
          end
          8'h04: begin  // Load byte to rd.H
            rd_we = 1'b1;
            mem_read = 1'b1;
            mem_size = MEM_BYTE;
          end
          8'h05, 8'h0D: begin  // Load halfword
            rd_we = 1'b1;
            mem_read = 1'b1;
            mem_size = MEM_HALF;
          end
          8'h06, 8'h0E: begin  // Load word (32-bit)
            rd_we = 1'b1;
            rd2_we = 1'b1;
            mem_read = 1'b1;
            mem_size = MEM_WORD;
          end
          8'h07, 8'h0F: begin  // Store byte from rd.L
            mem_write = 1'b1;
            mem_size = MEM_BYTE;
          end
          8'h08, 8'h10: begin  // Store byte from rd.H
            mem_write = 1'b1;
            mem_size = MEM_BYTE;
          end
          8'h09, 8'h11: begin  // Store halfword
            mem_write = 1'b1;
            mem_size = MEM_HALF;
          end
          8'h0A, 8'h12: begin  // Store word (32-bit)
            mem_write = 1'b1;
            mem_size = MEM_WORD;
          end
          8'h0B: begin  // Load byte with offset
            rd_we = 1'b1;
            mem_read = 1'b1;
            mem_size = MEM_BYTE;
          end
          8'h0C: begin  // Load byte to rd.H with offset
            rd_we = 1'b1;
            mem_read = 1'b1;
            mem_size = MEM_BYTE;
          end
          default: begin
            // Keep defaults
          end
        endcase
      end
      
      ITYPE_BRANCH: begin
        is_branch = 1'b1;
      end
      
      ITYPE_MUL: begin
        rd_we = 1'b1;
        rd2_we = 1'b1;
      end
      
      ITYPE_STACK: begin
        case (opcode_int)
          OP_PSH: begin
            mem_write = 1'b1;
            mem_size = MEM_HALF;
          end
          OP_POP: begin
            rd_we = 1'b1;
            mem_read = 1'b1;
            mem_size = MEM_HALF;
          end
          default: begin
            // Keep defaults
          end
        endcase
      end
      
      ITYPE_SUB: begin
        case (opcode_int)
          OP_JSR: begin
            is_jsr = 1'b1;
            is_branch = 1'b1;
            mem_write = 1'b1;  // Push return address
            mem_size = MEM_WORD;
          end
          OP_RTS: begin
            is_rts = 1'b1;
            is_branch = 1'b1;
            mem_read = 1'b1;  // Pop return address
            mem_size = MEM_WORD;
          end
          default: begin
            // Keep defaults
          end
        endcase
      end
      
      ITYPE_CTRL: begin
        if (opcode_int == OP_HLT) begin
          is_halt = 1'b1;
        end
        // WFI and NOP don't set any control signals
      end
      
      default: begin
        // Keep defaults
      end
    endcase
  end

endmodule : decode_unit
