//
// neocore_pkg.sv
// NeoCore 16x32 CPU - Common Package Definitions
//
// This package contains all type definitions, constants, and enumerations
// used throughout the NeoCore CPU implementation.
//

package neocore_pkg;

  // ============================================================================
  // Opcode Definitions
  // ============================================================================
  
  typedef enum logic [7:0] {
    OP_NOP   = 8'h00,
    OP_ADD   = 8'h01,
    OP_SUB   = 8'h02,
    OP_MUL   = 8'h03,
    OP_AND   = 8'h04,
    OP_OR    = 8'h05,
    OP_XOR   = 8'h06,
    OP_LSH   = 8'h07,
    OP_RSH   = 8'h08,
    OP_MOV   = 8'h09,
    OP_B     = 8'h0A,
    OP_BE    = 8'h0B,
    OP_BNE   = 8'h0C,
    OP_BLT   = 8'h0D,
    OP_BGT   = 8'h0E,
    OP_BRO   = 8'h0F,
    OP_UMULL = 8'h10,
    OP_SMULL = 8'h11,
    OP_HLT   = 8'h12,
    OP_PSH   = 8'h13,
    OP_POP   = 8'h14,
    OP_JSR   = 8'h15,
    OP_RTS   = 8'h16,
    OP_WFI   = 8'h17,
    OP_ENI   = 8'h18,
    OP_DSI   = 8'h19
  } opcode_e;

  // ============================================================================
  // Instruction Type Encoding
  // ============================================================================
  
  typedef enum logic [3:0] {
    ITYPE_ALU     = 4'h0,  // Arithmetic/Logic operations
    ITYPE_MOV     = 4'h1,  // Data movement
    ITYPE_BRANCH  = 4'h2,  // Branches
    ITYPE_MUL     = 4'h3,  // Multiply operations
    ITYPE_STACK   = 4'h4,  // Stack operations
    ITYPE_SUB     = 4'h5,  // Subroutine (JSR/RTS)
    ITYPE_CTRL    = 4'h6,  // Control (NOP/HLT/WFI)
    ITYPE_INVALID = 4'hF
  } itype_e;

  // ============================================================================
  // ALU Operation Encoding
  // ============================================================================
  
  typedef enum logic [3:0] {
    ALU_ADD  = 4'h0,
    ALU_SUB  = 4'h1,
    ALU_MUL  = 4'h2,
    ALU_AND  = 4'h3,
    ALU_OR   = 4'h4,
    ALU_XOR  = 4'h5,
    ALU_LSH  = 4'h6,
    ALU_RSH  = 4'h7,
    ALU_PASS = 4'h8,  // Pass operand A through
    ALU_NOP  = 4'hF
  } alu_op_e;

  // ============================================================================
  // Memory Access Size
  // ============================================================================
  
  typedef enum logic [1:0] {
    MEM_BYTE = 2'b00,  // 8-bit access
    MEM_HALF = 2'b01,  // 16-bit access
    MEM_WORD = 2'b10   // 32-bit access
  } mem_size_e;

  // ============================================================================
  // Register Address Width
  // ============================================================================
  
  parameter int REG_ADDR_WIDTH = 4;  // 16 registers = 4-bit address
  parameter int NUM_REGS = 16;
  
  // ============================================================================
  // Pipeline Stage Structures
  // ============================================================================
  
  // Instruction Fetch to Decode (IF/ID) Pipeline Register
  typedef struct packed {
    logic        valid;           // Instruction valid
    logic [31:0] pc;              // Program counter for this instruction
    logic [103:0] inst_data;      // Up to 13 bytes of instruction data (big-endian)
    logic [3:0]  inst_len;        // Instruction length in bytes
  } if_id_t;

  // Instruction Decode to Execute (ID/EX) Pipeline Register
  typedef struct packed {
    logic        valid;           // Instruction valid
    logic [31:0] pc;              // Program counter
    opcode_e     opcode;          // Decoded opcode
    logic [7:0]  specifier;       // Instruction specifier
    itype_e      itype;           // Instruction type
    alu_op_e     alu_op;          // ALU operation
    
    // Source operands
    logic [3:0]  rs1_addr;        // Source register 1 address
    logic [3:0]  rs2_addr;        // Source register 2 address
    logic [15:0] rs1_data;        // Source register 1 data
    logic [15:0] rs2_data;        // Source register 2 data
    logic [31:0] immediate;       // Immediate value (if any)
    
    // Destination
    logic [3:0]  rd_addr;         // Destination register address
    logic [3:0]  rd2_addr;        // Second destination (for 32-bit ops)
    logic        rd_we;           // Destination write enable
    logic        rd2_we;          // Second destination write enable
    
    // Memory operation
    logic        mem_read;        // Memory read operation
    logic        mem_write;       // Memory write operation
    mem_size_e   mem_size;        // Memory access size
    
    // Branch operation
    logic        is_branch;       // Is a branch instruction
    logic        is_jsr;          // Is JSR (jump to subroutine)
    logic        is_rts;          // Is RTS (return from subroutine)
    
    // Control
    logic        is_halt;         // Halt instruction
  } id_ex_t;

  // Execute to Memory (EX/MEM) Pipeline Register
  typedef struct packed {
    logic        valid;           // Instruction valid
    logic [31:0] pc;              // Program counter
    
    // ALU result
    logic [31:0] alu_result;      // ALU output (may be 32-bit for address calc)
    logic        z_flag;          // Zero flag from ALU
    logic        v_flag;          // Overflow flag from ALU
    
    // Destination
    logic [3:0]  rd_addr;         // Destination register address
    logic [3:0]  rd2_addr;        // Second destination
    logic        rd_we;           // Destination write enable
    logic        rd2_we;          // Second destination write enable
    
    // Memory operation
    logic        mem_read;        // Memory read operation
    logic        mem_write;       // Memory write operation
    mem_size_e   mem_size;        // Memory access size
    logic [31:0] mem_addr;        // Memory address
    logic [15:0] mem_wdata;       // Memory write data
    
    // Branch/control
    logic        branch_taken;    // Branch was taken
    logic [31:0] branch_target;   // Branch target address
    logic        is_halt;         // Halt instruction
  } ex_mem_t;

  // Memory to Write-Back (MEM/WB) Pipeline Register
  typedef struct packed {
    logic        valid;           // Instruction valid
    logic [31:0] pc;              // Program counter
    
    // Write-back data
    logic [15:0] wb_data;         // Primary write-back data
    logic [15:0] wb_data2;        // Secondary write-back data (for 32-bit ops)
    logic [3:0]  rd_addr;         // Destination register address
    logic [3:0]  rd2_addr;        // Second destination
    logic        rd_we;           // Destination write enable
    logic        rd2_we;          // Second destination write enable
    
    // Flags
    logic        z_flag;          // Zero flag
    logic        v_flag;          // Overflow flag
    
    // Control
    logic        is_halt;         // Halt instruction
  } mem_wb_t;

  // ============================================================================
  // Helper Functions
  // ============================================================================
  
  // Determine instruction type from opcode
  function automatic itype_e get_itype(opcode_e op);
    case (op)
      OP_ADD, OP_SUB, OP_MUL, OP_AND, OP_OR, OP_XOR, OP_LSH, OP_RSH:
        return ITYPE_ALU;
      OP_MOV:
        return ITYPE_MOV;
      OP_B, OP_BE, OP_BNE, OP_BLT, OP_BGT, OP_BRO:
        return ITYPE_BRANCH;
      OP_UMULL, OP_SMULL:
        return ITYPE_MUL;
      OP_PSH, OP_POP:
        return ITYPE_STACK;
      OP_JSR, OP_RTS:
        return ITYPE_SUB;
      OP_NOP, OP_HLT, OP_WFI:
        return ITYPE_CTRL;
      default:
        return ITYPE_INVALID;
    endcase
  endfunction

  // Convert opcode to ALU operation
  function automatic alu_op_e opcode_to_alu_op(opcode_e op);
    case (op)
      OP_ADD:  return ALU_ADD;
      OP_SUB:  return ALU_SUB;
      OP_MUL:  return ALU_MUL;
      OP_AND:  return ALU_AND;
      OP_OR:   return ALU_OR;
      OP_XOR:  return ALU_XOR;
      OP_LSH:  return ALU_LSH;
      OP_RSH:  return ALU_RSH;
      default: return ALU_NOP;
    endcase
  endfunction

  // Get instruction length based on opcode and specifier
  function automatic logic [3:0] get_inst_length(logic [7:0] opcode, logic [7:0] specifier);
    case (opcode)
      8'h00, 8'h12, 8'h16, 8'h17, 8'h18, 8'h19:  // NOP, HLT, RTS, WFI, ENI, DSI
        return 4'd2;
      8'h13, 8'h14:  // PSH, POP
        return 4'd3;
      8'h01, 8'h02, 8'h03, 8'h04, 8'h05, 8'h06, 8'h07, 8'h08, 8'h10, 8'h11: begin  // ALU ops, UMULL, SMULL
        case (specifier)
          8'h00:   return 4'd5;
          8'h01, 8'h03: return 4'd4;
          8'h02:   return 4'd7;
          default: return 4'd1;
        endcase
      end
      8'h09: begin  // MOV
        case (specifier)
          8'h00:   return 4'd5;
          8'h01:   return 4'd8;
          8'h02:   return 4'd4;
          8'h03, 8'h04, 8'h05, 8'h07, 8'h08, 8'h09: return 4'd7;
          8'h06, 8'h0A, 8'h0B, 8'h0C, 8'h0D, 8'h0F, 8'h10, 8'h11: return 4'd8;
          8'h0E, 8'h12: return 4'd9;
          default: return 4'd1;
        endcase
      end
      8'h0A, 8'h15:  // B, JSR
        return 4'd6;
      8'h0B, 8'h0C, 8'h0D, 8'h0E, 8'h0F:  // BE, BNE, BLT, BGT, BRO
        return 4'd8;
      default:
        return 4'd1;
    endcase
  endfunction

endpackage : neocore_pkg
