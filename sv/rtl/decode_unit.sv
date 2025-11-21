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
// Big-Endian Instruction Format:
//   inst_data[103:96]  = Byte 0 (Specifier)
//   inst_data[95:88]   = Byte 1 (Opcode)
//   inst_data[87:80]   = Byte 2 (rd or first operand byte)
//   inst_data[79:72]   = Byte 3
//   ...
//   inst_data[7:0]     = Byte 12 (last byte for longest instructions)
//

module decode_unit
  import neocore_pkg::*;
(
  input  logic         clk,
  input  logic         rst,
  
  // Input instruction (big-endian)
  input  logic [103:0] inst_data,    // Up to 13 bytes of instruction
  input  logic [3:0]   inst_len,     // Instruction length
  input  logic [31:0]  pc,           // Current PC
  input  logic         valid_in,     // Instruction valid
  
  // Decoded outputs
  output logic         valid_out,
  output opcode_e      opcode,
  output logic [7:0]   specifier,
  output itype_e       itype,
  output alu_op_e      alu_op,
  
  // Register addresses
  output logic [3:0]   rs1_addr,
  output logic [3:0]   rs2_addr,
  output logic [3:0]   rd_addr,
  output logic [3:0]   rd2_addr,
  
  // Immediate/address values
  output logic [31:0]  immediate,
  output logic [31:0]  mem_addr,
  output logic [31:0]  branch_target,
  
  // Control signals
  output logic         rd_we,        // Destination register write enable
  output logic         rd2_we,       // Second destination write enable
  output logic         mem_read,
  output logic         mem_write,
  output mem_size_e    mem_size,
  output logic         is_branch,
  output logic         is_jsr,
  output logic         is_rts,
  output logic         is_halt
);

  // ============================================================================
  // Instruction Field Extraction (Big-Endian)
  // ============================================================================
  
  // Extract individual bytes from instruction data
  // Big-endian: MSB bytes at higher bit positions
  logic [7:0] byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7, byte8, byte9, byte10, byte11, byte12;
  
  always_comb begin
    byte0  = inst_data[103:96];  // Specifier
    byte1  = inst_data[95:88];   // Opcode
    byte2  = inst_data[87:80];   // rd (for most instructions)
    byte3  = inst_data[79:72];   // rn or immediate byte
    byte4  = inst_data[71:64];   // immediate byte or rn1
    byte5  = inst_data[63:56];   // address/immediate byte
    byte6  = inst_data[55:48];   // address/immediate byte
    byte7  = inst_data[47:40];   // address/immediate byte
    byte8  = inst_data[39:32];   // address/immediate byte
    byte9  = inst_data[31:24];   // address/immediate byte (for longest instructions)
    byte10 = inst_data[23:16];
    byte11 = inst_data[15:8];
    byte12 = inst_data[7:0];
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
    
    case (opcode_int)
      OP_ADD, OP_SUB, OP_MUL, OP_AND, OP_OR, OP_XOR, OP_LSH, OP_RSH: begin
        // ALU instructions
        rd_addr = byte2[3:0];
        case (specifier)
          8'h00: begin  // rd, #immediate
            rs1_addr = byte2[3:0];  // rd is also source
            rs2_addr = 4'h0;
          end
          8'h01: begin  // rd, rn
            rs1_addr = byte3[3:0];  // rn
            rs2_addr = byte2[3:0];  // rd
          end
          8'h02: begin  // rd, [address]
            rs1_addr = 4'h0;  // Will load from memory
            rs2_addr = byte2[3:0];  // rd
          end
          default: begin
            rs1_addr = 4'h0;
            rs2_addr = 4'h0;
          end
        endcase
      end
      
      OP_MOV: begin
        // MOV instructions - many variants
        rd_addr = byte2[3:0];
        case (specifier)
          8'h00: begin  // mov rd, #immediate
            rs1_addr = 4'h0;
            rs2_addr = 4'h0;
          end
          8'h01: begin  // mov rd, rn, label
            rs1_addr = byte3[3:0];  // rn
            rs2_addr = 4'h0;
          end
          8'h02: begin  // mov rd, rn
            rs1_addr = byte3[3:0];  // rn
            rs2_addr = 4'h0;
          end
          8'h03, 8'h04, 8'h05: begin  // mov rd.L/H, [addr] or mov rd, [addr]
            rs1_addr = 4'h0;  // Load from memory
            rs2_addr = 4'h0;
          end
          8'h06: begin  // mov rd, rn1, [addr]
            rs1_addr = byte3[3:0];  // rn1
            rd2_addr = byte3[3:0];  // rn1 also destination
          end
          8'h07, 8'h08, 8'h09: begin  // mov [addr], rd.L/H or rd
            rs1_addr = byte2[3:0];  // rd to store
            rs2_addr = 4'h0;
            rd_addr = 4'h0;  // No destination (store only)
          end
          8'h0A: begin  // mov [addr], rd, rn1
            rs1_addr = byte2[3:0];  // rd
            rs2_addr = byte3[3:0];  // rn1
            rd_addr = 4'h0;
          end
          8'h0B, 8'h0C, 8'h0D: begin  // mov rd.L/H, [rn + offset] or rd
            rs1_addr = byte3[3:0];  // rn (base address)
            rs2_addr = 4'h0;
          end
          8'h0E: begin  // mov rd, rd1, [rn + offset]
            rs1_addr = byte4[3:0];  // rn
            rd2_addr = byte3[3:0];  // rd1
          end
          8'h0F, 8'h10, 8'h11: begin  // mov [rn + offset], rd.L/H or rd
            rs1_addr = byte2[3:0];  // rd (data to store)
            rs2_addr = byte3[3:0];  // rn (base)
            rd_addr = 4'h0;
          end
          8'h12: begin  // mov [rn + offset], rd, rn1
            rs1_addr = byte2[3:0];  // rd
            rs2_addr = byte4[3:0];  // rn (base)
            rd_addr = 4'h0;
          end
          default: begin
            rs1_addr = 4'h0;
            rs2_addr = 4'h0;
          end
        endcase
      end
      
      OP_BE, OP_BNE, OP_BLT, OP_BGT: begin
        // Conditional branches with two operands
        rs1_addr = byte2[3:0];  // rd
        rs2_addr = byte3[3:0];  // rn
        rd_addr = 4'h0;  // No write
      end
      
      OP_UMULL, OP_SMULL: begin
        // Multiply long: rd, rn, rn1
        rd_addr = byte2[3:0];   // rd (low result)
        rd2_addr = byte3[3:0];  // rn (high result)
        rs1_addr = byte3[3:0];  // rn (operand 1)
        rs2_addr = byte4[3:0];  // rn1 (operand 2)
      end
      
      OP_PSH: begin
        // Push: rd
        rs1_addr = byte2[3:0];  // rd (value to push)
        rd_addr = 4'h0;
      end
      
      OP_POP: begin
        // Pop: rd
        rd_addr = byte2[3:0];  // rd (destination)
        rs1_addr = 4'h0;
      end
      
      default: begin
        // NOP, HLT, B, JSR, RTS, WFI, BRO, ENI, DSI
        rs1_addr = 4'h0;
        rs2_addr = 4'h0;
        rd_addr = 4'h0;
        rd2_addr = 4'h0;
      end
    endcase
  end
  
  // ============================================================================
  // Immediate and Address Extraction (Big-Endian)
  // ============================================================================
  
  always_comb begin
    immediate = 32'h0;
    mem_addr = 32'h0;
    branch_target = 32'h0;
    
    case (opcode_int)
      OP_ADD, OP_SUB, OP_MUL, OP_AND, OP_OR, OP_XOR, OP_LSH, OP_RSH: begin
        case (specifier)
          8'h00: begin  // Immediate (16-bit, big-endian)
            immediate = {16'h0, byte3, byte4};
          end
          8'h02: begin  // Memory address (32-bit, big-endian)
            mem_addr = {byte3, byte4, byte5, byte6};
          end
          default: immediate = 32'h0;
        endcase
      end
      
      OP_MOV: begin
        case (specifier)
          8'h00: begin  // mov rd, #immediate (16-bit)
            immediate = {16'h0, byte3, byte4};
          end
          8'h01: begin  // mov rd, rn, label (32-bit address)
            branch_target = {byte4, byte5, byte6, byte7};
          end
          8'h03, 8'h04, 8'h05, 8'h07, 8'h08, 8'h09: begin  // [normAddressing]
            mem_addr = {byte3, byte4, byte5, byte6};
          end
          8'h06, 8'h0A: begin  // rd, rn1, [normAddressing]
            mem_addr = {byte4, byte5, byte6, byte7};
          end
          8'h0B, 8'h0C, 8'h0D, 8'h0F, 8'h10, 8'h11: begin  // [rn + offset]
            immediate = {byte4, byte5, byte6, byte7};  // offset
          end
          8'h0E, 8'h12: begin  // rd, rd1, [rn + offset] or [rn + offset], rd, rn1
            immediate = {byte5, byte6, byte7, byte8};  // offset
          end
          default: immediate = 32'h0;
        endcase
      end
      
      OP_B, OP_JSR: begin
        // Unconditional branch/jump (32-bit address)
        branch_target = {byte2, byte3, byte4, byte5};
      end
      
      OP_BE, OP_BNE, OP_BLT, OP_BGT, OP_BRO: begin
        // Conditional branch (32-bit address)
        if (opcode_int == OP_BRO) begin
          // BRO has no operands, just label
          branch_target = {byte2, byte3, byte4, byte5};
        end else begin
          // Other branches have rd, rn, label
          branch_target = {byte4, byte5, byte6, byte7};
        end
      end
      
      default: begin
        immediate = 32'h0;
        mem_addr = 32'h0;
        branch_target = 32'h0;
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
    
    if (valid_in) begin
      case (opcode_int)
        OP_ADD, OP_SUB, OP_MUL, OP_AND, OP_OR, OP_XOR, OP_LSH, OP_RSH: begin
          // ALU operations
          rd_we = 1'b1;
          if (specifier == 8'h02) begin
            // Load operand from memory
            mem_read = 1'b1;
            mem_size = MEM_HALF;
          end
        end
        
        OP_MOV: begin
          case (specifier)
            8'h00, 8'h01, 8'h02: begin  // Register moves
              rd_we = 1'b1;
            end
            8'h03: begin  // mov rd.L, [addr] - byte load
              rd_we = 1'b1;
              mem_read = 1'b1;
              mem_size = MEM_BYTE;
            end
            8'h04: begin  // mov rd.H, [addr] - byte load to high
              rd_we = 1'b1;
              mem_read = 1'b1;
              mem_size = MEM_BYTE;
            end
            8'h05, 8'h0B, 8'h0C, 8'h0D: begin  // mov rd, [addr] - halfword load
              rd_we = 1'b1;
              mem_read = 1'b1;
              mem_size = MEM_HALF;
            end
            8'h06, 8'h0E: begin  // Two-register load
              rd_we = 1'b1;
              rd2_we = 1'b1;
              mem_read = 1'b1;
              mem_size = MEM_WORD;
            end
            8'h07: begin  // mov [addr], rd.L - byte store
              mem_write = 1'b1;
              mem_size = MEM_BYTE;
            end
            8'h08: begin  // mov [addr], rd.H - byte store from high
              mem_write = 1'b1;
              mem_size = MEM_BYTE;
            end
            8'h09, 8'h0F, 8'h10, 8'h11: begin  // mov [addr], rd - halfword store
              mem_write = 1'b1;
              mem_size = MEM_HALF;
            end
            8'h0A, 8'h12: begin  // Two-register store
              mem_write = 1'b1;
              mem_size = MEM_WORD;
            end
            default: begin
              // Unknown specifier
            end
          endcase
        end
        
        OP_B, OP_BE, OP_BNE, OP_BLT, OP_BGT, OP_BRO: begin
          is_branch = 1'b1;
        end
        
        OP_JSR: begin
          is_jsr = 1'b1;
          is_branch = 1'b1;
        end
        
        OP_RTS: begin
          is_rts = 1'b1;
          is_branch = 1'b1;
        end
        
        OP_HLT: begin
          is_halt = 1'b1;
        end
        
        OP_UMULL, OP_SMULL: begin
          // Multiply long - writes two registers
          rd_we = 1'b1;
          rd2_we = 1'b1;
        end
        
        OP_PSH: begin
          // Push to stack - memory write
          mem_write = 1'b1;
          mem_size = MEM_HALF;
        end
        
        OP_POP: begin
          // Pop from stack - memory read
          rd_we = 1'b1;
          mem_read = 1'b1;
          mem_size = MEM_HALF;
        end
        
        OP_WFI, OP_ENI, OP_DSI, OP_NOP: begin
          // Control instructions - no register/memory access
          // WFI, ENI, DSI would be handled by interrupt controller (not implemented)
        end
        
        default: begin
          // Unknown opcode
        end
      endcase
    end
  end

endmodule : decode_unit
