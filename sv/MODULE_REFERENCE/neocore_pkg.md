# neocore_pkg.sv - Package Definitions

## Purpose

The `neocore_pkg` SystemVerilog package contains all type definitions, constants, enumerations, and helper functions used throughout the NeoCore16x32 CPU implementation. It serves as the central repository for shared definitions to ensure consistency across all modules.

## Package Contents

### Opcode Enumeration

```verilog
typedef enum logic [7:0] {
  OP_NOP   = 8'h00,  // No operation
  OP_ADD   = 8'h01,  // Addition
  OP_SUB   = 8'h02,  // Subtraction
  OP_MUL   = 8'h03,  // Multiplication (truncated)
  OP_AND   = 8'h04,  // Bitwise AND
  OP_OR    = 8'h05,  // Bitwise OR
  OP_XOR   = 8'h06,  // Bitwise XOR
  OP_LSH   = 8'h07,  // Left shift
  OP_RSH   = 8'h08,  // Right shift (logical)
  OP_MOV   = 8'h09,  // Move data
  OP_B     = 8'h0A,  // Unconditional branch
  OP_BE    = 8'h0B,  // Branch if equal
  OP_BNE   = 8'h0C,  // Branch if not equal
  OP_BLT   = 8'h0D,  // Branch if less than
  OP_BGT   = 8'h0E,  // Branch if greater than
  OP_BRO   = 8'h0F,  // Branch if overflow
  OP_UMULL = 8'h10,  // Unsigned multiply long
  OP_SMULL = 8'h11,  // Signed multiply long
  OP_HLT   = 8'h12,  // Halt
  OP_PSH   = 8'h13,  // Push
  OP_POP   = 8'h14,  // Pop
  OP_JSR   = 8'h15,  // Jump to subroutine
  OP_RTS   = 8'h16,  // Return from subroutine
  OP_WFI   = 8'h17,  // Wait for interrupt (placeholder)
  OP_ENI   = 8'h18,  // Enable interrupts (placeholder)
  OP_DSI   = 8'h19   // Disable interrupts (placeholder)
} opcode_e;
```

**Usage**: Decode opcode byte from instruction and compare against these values.

### Instruction Type Enumeration

```verilog
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
```

**Usage**: Categorize instructions for pipeline control.

### ALU Operation Enumeration

```verilog
typedef enum logic [3:0] {
  ALU_ADD  = 4'h0,  // Addition
  ALU_SUB  = 4'h1,  // Subtraction
  ALU_MUL  = 4'h2,  // Multiplication
  ALU_AND  = 4'h3,  // Bitwise AND
  ALU_OR   = 4'h4,  // Bitwise OR
  ALU_XOR  = 4'h5,  // Bitwise XOR
  ALU_LSH  = 4'h6,  // Left shift
  ALU_RSH  = 4'h7,  // Right shift
  ALU_PASS = 4'h8,  // Pass operand A through
  ALU_NOP  = 4'hF   // No operation
} alu_op_e;
```

**Usage**: Control ALU operation selection.

### Memory Access Size Enumeration

```verilog
typedef enum logic [1:0] {
  MEM_BYTE = 2'b00,  // 8-bit access
  MEM_HALF = 2'b01,  // 16-bit access
  MEM_WORD = 2'b10   // 32-bit access
} mem_size_e;
```

**Usage**: Specify memory access granularity.

### Register Address Width

```verilog
parameter int REG_ADDR_WIDTH = 4;  // 4-bit register addressing
parameter int NUM_REGS = 16;       // 16 general-purpose registers
```

## Pipeline Stage Structures

### IF/ID Pipeline Register

```verilog
typedef struct packed {
  logic        valid;           // Instruction valid
  logic [31:0] pc;              // Program counter
  logic [103:0] inst_data;      // Up to 13 bytes (104 bits)
  logic [3:0]  inst_len;        // Instruction length in bytes
} if_id_t;
```

**Fields**:
- `valid`: Indicates whether the instruction is valid
- `pc`: Program counter value for this instruction
- `inst_data`: Big-endian instruction bytes (max 13 bytes)
- `inst_len`: Actual length of this instruction (2-13)

### ID/EX Pipeline Register

```verilog
typedef struct packed {
  logic        valid;
  logic [31:0] pc;
  opcode_e     opcode;
  logic [7:0]  specifier;
  itype_e      itype;
  alu_op_e     alu_op;
  logic [3:0]  rs1_addr, rs2_addr;
  logic [15:0] rs1_data, rs2_data;
  logic [31:0] immediate;
  logic [3:0]  rd_addr, rd2_addr;
  logic        rd_we, rd2_we;
  logic        mem_read, mem_write;
  mem_size_e   mem_size;
  logic        is_branch, is_jsr, is_rts, is_halt;
} id_ex_t;
```

**Purpose**: Holds all decoded control signals and operand values.

### EX/MEM Pipeline Register

```verilog
typedef struct packed {
  logic        valid;
  logic [31:0] pc;
  logic [31:0] alu_result;
  logic        z_flag, v_flag;
  logic [3:0]  rd_addr, rd2_addr;
  logic        rd_we, rd2_we;
  logic        mem_read, mem_write;
  mem_size_e   mem_size;
  logic [31:0] mem_addr;
  logic [15:0] mem_wdata;
  logic        branch_taken;
  logic [31:0] branch_target;
  logic        is_halt;
} ex_mem_t;
```

**Purpose**: Holds execution results and memory operation info.

### MEM/WB Pipeline Register

```verilog
typedef struct packed {
  logic        valid;
  logic [31:0] pc;
  logic [15:0] wb_data;      // Primary write-back data
  logic [15:0] wb_data2;     // Secondary (for 32-bit ops)
  logic [3:0]  rd_addr, rd2_addr;
  logic        rd_we, rd2_we;
  logic        z_flag, v_flag;
  logic        is_halt;
} mem_wb_t;
```

**Purpose**: Holds final results ready for write-back.

## Helper Functions

### get_itype()

Determines instruction type from opcode.

```verilog
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
```

**Usage**: `itype = get_itype(decoded_opcode);`

### opcode_to_alu_op()

Converts opcode to ALU operation.

```verilog
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
```

**Usage**: `alu_operation = opcode_to_alu_op(opcode);`

### get_inst_length()

Calculates instruction length based on opcode and specifier.

```verilog
function automatic logic [3:0] get_inst_length(
  logic [7:0] opcode,
  logic [7:0] specifier
);
  case (opcode)
    8'h00, 8'h12, 8'h16, 8'h17, 8'h18, 8'h19:  // NOP, HLT, RTS, WFI, ENI, DSI
      return 4'd2;
    8'h13, 8'h14:  // PSH, POP
      return 4'd3;
    8'h01, 8'h02, 8'h03, 8'h04, 8'h05, 8'h06, 8'h07, 8'h08, 8'h10, 8'h11: begin
      // ALU ops, UMULL, SMULL
      case (specifier)
        8'h00:   return 4'd5;  // Immediate
        8'h01, 8'h03: return 4'd4;  // Register
        8'h02:   return 4'd7;  // Memory
        default: return 4'd1;  // Error/unknown
      endcase
    end
    8'h09: begin  // MOV - many variants
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
    8'h0B, 8'h0C, 8'h0D, 8'h0E, 8'h0F:  // Conditional branches
      return 4'd8;
    default:
      return 4'd1;  // Unknown/error
  endcase
endfunction
```

**Usage**: `length = get_inst_length(opcode_byte, specifier_byte);`

**Critical**: This function is used by fetch_unit to determine instruction boundaries.

## Usage in Modules

### Importing the Package

All modules that use these definitions must import the package:

```verilog
module some_module
  import neocore_pkg::*;  // Import all package definitions
(
  // ... ports
);
```

### Using Enumerations

```verilog
opcode_e current_opcode;
alu_op_e alu_operation;

always_comb begin
  if (current_opcode == OP_ADD) begin
    alu_operation = ALU_ADD;
  end
end
```

### Using Structures

```verilog
if_id_t if_id_reg;

always_ff @(posedge clk) begin
  if (rst) begin
    if_id_reg.valid <= 1'b0;
    if_id_reg.pc <= 32'h0;
    // ...
  end else begin
    if_id_reg <= if_id_next;
  end
end
```

## Modification Guidelines

When adding new instructions:

1. Add opcode to `opcode_e` enumeration
2. Update `get_itype()` to classify the new instruction
3. If ALU operation, add to `alu_op_e` and `opcode_to_alu_op()`
4. Add instruction length to `get_inst_length()`
5. Recompile all modules that import the package

## Synthesis Considerations

- **Enumerations**: Synthesize to logic vectors of specified width
- **Structures**: Synthesize to concatenated logic vectors (packed)
- **Functions**: Inlined during synthesis (no overhead)
- **Parameters**: Compile-time constants

**Resource Impact**: Minimal (only type definitions and functions)

## Verification

The package itself is not testable in isolation. Verification occurs through:
1. Module-level tests using package types
2. Integration tests exercising all instruction types
3. Coverage analysis ensuring all enum values used

## Summary

`neocore_pkg.sv` provides the foundation for the entire NeoCore16x32 implementation by:
- Defining all instruction opcodes and types
- Providing pipeline stage structures
- Centralizing helper functions
- Ensuring consistency across modules

All RTL files import this package and depend on its definitions.

