# NeoCore16x32 Microarchitecture

## Introduction

This document provides detailed information about the internal microarchitectural implementation of the NeoCore16x32 CPU. It describes how the architectural specification is realized in hardware RTL modules.

## Module Hierarchy

```
core_top
├── fetch_unit
├── decode_unit (x2 for dual-issue)
├── issue_unit
├── register_file
├── execute_stage
│   ├── alu (x2)
│   ├── multiply_unit (x2)
│   └── branch_unit (x2)
├── hazard_unit
├── memory_stage
├── writeback_stage
├── unified_memory
└── pipeline_regs
    ├── if_id_reg (x2)
    ├── id_ex_reg (x2)
    ├── ex_mem_reg (x2)
    └── mem_wb_reg (x2)
```

---

## Core Integration (core_top.sv)

### Purpose
Top-level module integrating all pipeline stages and control logic for the complete CPU.

### Key Responsibilities
1. Instantiate all pipeline stage modules
2. Connect pipeline registers
3. Manage global control signals (stall, flush, halt)
4. Interface with unified memory
5. Coordinate dual-issue execution

### Interfaces

**Clock and Reset**:
```verilog
input logic clk;      // System clock (100 MHz target)
input logic rst;      // Synchronous reset (active high)
```

**Unified Memory - Instruction Fetch Port**:
```verilog
output logic [31:0]  mem_if_addr;    // Fetch address
output logic         mem_if_req;     // Fetch request
input  logic [127:0] mem_if_rdata;   // 16 bytes of instruction data
input  logic         mem_if_ack;     // Fetch acknowledge
```

**Unified Memory - Data Access Port**:
```verilog
output logic [31:0] mem_data_addr;   // Data address
output logic [31:0] mem_data_wdata;  // Write data
output logic [1:0]  mem_data_size;   // Access size (byte/half/word)
output logic        mem_data_we;     // Write enable
output logic        mem_data_req;    // Data request
input  logic [31:0] mem_data_rdata;  // Read data
input  logic        mem_data_ack;    // Data acknowledge
```

**Status Outputs**:
```verilog
output logic        halted;              // CPU halted
output logic [31:0] current_pc;          // Current program counter
output logic        dual_issue_active;   // Dual-issue this cycle
```

### Internal State

**CPU Flags**:
```verilog
logic z_flag, v_flag;          // Zero and Overflow flags
logic z_flag_next, v_flag_next;
```

**Control Signals**:
```verilog
logic stall_pipeline;    // Global stall signal
logic flush_if, flush_id, flush_ex;  // Pipeline flush signals
logic branch_taken;      // Branch taken this cycle
logic [31:0] branch_target;  // Branch target address
```

### Pipeline Control Logic

**Stall Generation**:
```verilog
assign stall_pipeline = hazard_stall ||  // Data hazard
                        mem_stall ||      // Memory not ready
                        halted;           // CPU halted
```

**Branch Handling**:
- Branch resolution in EX stage
- `branch_taken` signal from execute_stage
- Flush IF and ID stages when branch taken
- Redirect PC to branch_target

### Datapath Connections

**Fetch to Decode**:
- Two IF/ID pipeline registers (inst_0 and inst_1)
- Valid bits control which instructions proceed

**Decode to Execute**:
- Register file provides 4 read ports
- Issue unit determines which instructions issue
- Two ID/EX pipeline registers

**Execute to Memory**:
- ALU/multiply/branch results
- Memory addresses and write data
- Two EX/MEM pipeline registers

**Memory to Writeback**:
- Loaded data or ALU results
- Two MEM/WB pipeline registers

**Writeback to Register File**:
- Two write ports (rd_addr_0/1, rd_data_0/1, rd_we_0/1)

### Timing Behavior

**Reset Sequence**:
1. Assert rst for at least 2 clock cycles
2. All pipeline registers cleared
3. PC initialized to 0x00000000
4. Flags cleared
5. Fetch begins after reset de-asserts

**Normal Operation**:
- Each stage processes on rising clock edge
- Pipeline advances unless stalled
- Branches cause 2-cycle flush and redirect

---

## Fetch Unit (fetch_unit.sv)

### Purpose
Fetch variable-length instructions from unified memory and prepare them for decode.

### Key Features
1. 32-byte circular instruction buffer
2. Wide 128-bit memory fetch (16 bytes/cycle)
3. Pre-decode instruction boundaries
4. Dual-instruction extraction for dual-issue
5. Big-endian byte handling

### State Machine

**States**:
- **FETCH_ACTIVE**: Requesting/receiving instructions from memory
- **FETCH_STALL**: Stalled waiting for buffer space or downstream

**Transitions**:
- Request memory when buffer_valid < 20 bytes
- Receive 16 bytes on mem_ack
- Consume bytes based on issued instructions

### Buffer Management

**Circular Buffer**:
```verilog
logic [255:0] fetch_buffer;  // 32 bytes, big-endian
logic [5:0]   buffer_valid;  // Number of valid bytes (0-32)
logic [31:0]  buffer_pc;     // PC of first byte in buffer
```

**Buffer Operations**:

*Refill*:
```verilog
if (mem_ack) begin
  fetch_buffer <= (fetch_buffer << 128) | {128'h0, mem_rdata};
  buffer_valid <= buffer_valid + 6'd16;
end
```

*Consume*:
```verilog
if (!stall && can_consume) begin
  fetch_buffer <= fetch_buffer << (consumed_bytes * 8);
  buffer_valid <= buffer_valid - consumed_bytes;
  buffer_pc <= buffer_pc + {26'h0, consumed_bytes};
end
```

### Instruction Extraction

**First Instruction**:
```verilog
inst_data_0 = fetch_buffer[255:152];  // Top 13 bytes
spec_0 = fetch_buffer[255:248];       // Specifier
op_0 = fetch_buffer[247:240];         // Opcode
inst_len_0 = get_inst_length(op_0, spec_0);
valid_0 = (buffer_valid >= inst_len_0) && (inst_len_0 > 0);
pc_0 = buffer_pc;
```

**Second Instruction**:
- Offset by inst_len_0 bytes from start of buffer
- Use case statement to extract based on inst_len_0
- Calculate inst_len_1 similarly
- Valid only if buffer has enough bytes for both

### PC Management

**PC Update**:
```verilog
if (branch_taken)
  pc_next = branch_target;
else if (!stall)
  pc_next = pc + {26'h0, consumed_bytes};
else
  pc_next = pc;  // Hold during stall
```

**Consumed Bytes Calculation**:
```verilog
consumed_bytes = (can_consume_0 ? inst_len_0 : 0) +
                 (can_consume_1 ? inst_len_1 : 0);
```

### Memory Interface

**Request Generation**:
```verilog
mem_req = (buffer_valid < 6'd20) && !stall && !branch_taken;
mem_addr = pc;
```

**Big-Endian Receive**:
```verilog
if_rdata[127:120] = mem[if_addr + 0];  // MSB
if_rdata[119:112] = mem[if_addr + 1];
...
if_rdata[7:0] = mem[if_addr + 15];     // LSB
```

### Timing

- **Fetch Latency**: 1 cycle (memory read)
- **Throughput**: Up to 2 instructions per cycle (if both fit in buffer)
- **Typical**: ~1.5 instructions per cycle average

---

## Decode Unit (decode_unit.sv)

### Purpose
Decode variable-length instructions and extract control signals and operands.

### Key Operations
1. Parse big-endian instruction fields
2. Identify opcode and instruction type
3. Extract register addresses
4. Extract immediates and addresses
5. Generate control signals

### Instruction Field Extraction

**Byte Parsing**:
```verilog
byte0  = inst_data[103:96];  // Specifier
byte1  = inst_data[95:88];   // Opcode
byte2  = inst_data[87:80];   // rd (usually)
byte3  = inst_data[79:72];   // rn or immediate byte
// ... up to byte12
```

### Opcode Decoding

**Opcode Identification**:
```verilog
opcode = opcode_e'(byte1);
itype = get_itype(opcode);
alu_op = opcode_to_alu_op(opcode);
```

### Register Address Extraction

**Pattern Matching by Opcode**:
```verilog
case (opcode)
  OP_ADD, OP_SUB, ...: begin
    rd_addr = byte2[3:0];
    case (specifier)
      8'h00: begin  // Immediate
        rs1_addr = byte2[3:0];  // rd is also source
        rs2_addr = 4'h0;
      end
      8'h01: begin  // Register
        rs1_addr = byte3[3:0];  // rn
        rs2_addr = byte2[3:0];  // rd
      end
      // ...
    endcase
  end
  // ... other opcodes
endcase
```

### Immediate and Address Extraction

**Big-Endian Assembly**:
```verilog
// 16-bit immediate (bytes 3-4)
immediate = {16'h0, byte3, byte4};

// 32-bit address (bytes 3-6)
mem_addr = {byte3, byte4, byte5, byte6};

// 32-bit branch target (varies by instruction)
branch_target = {byte4, byte5, byte6, byte7};  // For conditional branches
```

### Control Signal Generation

**Decode Logic**:
```verilog
case (opcode)
  OP_ADD, OP_SUB, ...: begin
    rd_we = 1'b1;
    if (specifier == 8'h02) begin
      mem_read = 1'b1;
      mem_size = MEM_HALF;
    end
  end
  OP_MOV: begin
    case (specifier)
      8'h05: begin  // mov rd, [addr]
        rd_we = 1'b1;
        mem_read = 1'b1;
        mem_size = MEM_HALF;
      end
      // ... 18 other specifiers
    endcase
  end
  // ... other opcodes
endcase
```

### Outputs

**Decoded Instruction**:
```verilog
output opcode_e opcode;
output logic [7:0] specifier;
output itype_e itype;
output alu_op_e alu_op;
output logic [3:0] rs1_addr, rs2_addr, rd_addr, rd2_addr;
output logic [31:0] immediate, mem_addr, branch_target;
output logic rd_we, rd2_we;
output logic mem_read, mem_write;
output mem_size_e mem_size;
output logic is_branch, is_jsr, is_rts, is_halt;
```

### Timing

- **Latency**: Combinational (0 cycles), but registered in ID/EX
- **Throughput**: Can decode 2 instructions per cycle

---

## Issue Unit (issue_unit.sv)

### Purpose
Determine which decoded instructions can issue together (dual-issue logic).

### Hazard Checks

**Structural Hazards**:
```verilog
// Memory port conflict
mem_port_conflict = (inst0_mem_read || inst0_mem_write) &&
                    (inst1_mem_read || inst1_mem_write);

// Write port conflict
write_port_conflict = inst0_rd_we && inst1_rd_we &&
                      (inst0_rd_addr == inst1_rd_addr) &&
                      (inst0_rd_addr != 4'h0);
```

**Data Dependencies**:
```verilog
// inst1 reads what inst0 writes
data_dependency = inst0_rd_we &&
                  ((inst0_rd_addr == inst1_rs1_addr) ||
                   (inst0_rd_addr == inst1_rs2_addr));
```

**Restrictions**:
```verilog
branch_restriction = inst0_is_branch || inst1_is_branch;
mul_restriction = (inst0_type == ITYPE_MUL) || (inst1_type == ITYPE_MUL);
```

### Issue Decision

**Logic**:
```verilog
issue_inst0 = inst0_valid;  // Always issue first if valid
issue_inst1 = inst0_valid && inst1_valid &&
              !mem_port_conflict &&
              !write_port_conflict &&
              !branch_restriction &&
              !data_dependency &&
              !mul_restriction;
dual_issue = issue_inst0 && issue_inst1;
```

### Timing

- **Latency**: Combinational
- **Critical Path**: Multiple AND gates checking hazards

---

## Register File (register_file.sv)

### Purpose
Store 16 general-purpose 16-bit registers with multi-port access.

### Organization

**Storage**:
```verilog
logic [15:0] registers [0:15];  // 16 registers
```

### Port Configuration

**4 Read Ports** (combinational):
```verilog
input  logic [3:0] rs1_addr_0, rs2_addr_0;  // Inst 0 sources
input  logic [3:0] rs1_addr_1, rs2_addr_1;  // Inst 1 sources
output logic [15:0] rs1_data_0, rs2_data_0;
output logic [15:0] rs1_data_1, rs2_data_1;
```

**2 Write Ports** (sequential):
```verilog
input logic [3:0]  rd_addr_0, rd_addr_1;
input logic [15:0] rd_data_0, rd_data_1;
input logic        rd_we_0, rd_we_1;
```

### Internal Forwarding

**Same-Cycle Write-Read**:
```verilog
// Read port 0, source 1
if (rd_we_0 && (rd_addr_0 == rs1_addr_0))
  rs1_data_0 = rd_data_0;  // Forward from write port 0
else if (rd_we_1 && (rd_addr_1 == rs1_addr_0))
  rs1_data_0 = rd_data_1;  // Forward from write port 1
else
  rs1_data_0 = registers[rs1_addr_0];  // Normal read
```

**Priority**: Write port 1 > Write port 0 > Stored value

### Write Behavior

**Sequential Write**:
```verilog
always_ff @(posedge clk) begin
  if (rst) begin
    for (int i = 0; i < 16; i++)
      registers[i] <= 16'h0000;
  end else begin
    if (rd_we_0) registers[rd_addr_0] <= rd_data_0;
    if (rd_we_1) registers[rd_addr_1] <= rd_data_1;
  end
end
```

**Note**: If both write ports write to same register, port 1 wins (though issue unit prevents this).

### Timing

- **Read**: Combinational (0 cycle latency)
- **Write**: Sequential (updates on next clock edge)
- **Forward**: Combinational bypass (0 cycle latency)

---

## Execute Stage (execute_stage.sv)

### Purpose
Integrate ALU, multiply, and branch units with operand forwarding.

### Sub-Modules

**Dual ALU**:
```verilog
alu alu_0 (
  .operand_a(operand_a_0),
  .operand_b(alu_op_b_0),
  .alu_op(id_ex_0.alu_op),
  .result(alu_result_0),
  .z_flag(alu_z_0),
  .v_flag(alu_v_0)
);

alu alu_1 (...);  // Second instance for inst1
```

**Dual Multiply**:
```verilog
multiply_unit mul_0 (
  .operand_a(operand_a_0),
  .operand_b(operand_b_0),
  .is_signed(id_ex_0.opcode == OP_SMULL),
  .result_lo(mul_result_lo_0),
  .result_hi(mul_result_hi_0)
);

multiply_unit mul_1 (...);
```

**Dual Branch**:
```verilog
branch_unit branch_0 (
  .opcode(id_ex_0.opcode),
  .operand_a(operand_a_0),
  .operand_b(operand_b_0),
  .v_flag_in(v_flag),
  .branch_target(id_ex_0.immediate),
  .branch_taken(branch_taken_0),
  .branch_pc(branch_pc_0)
);

branch_unit branch_1 (...);
```

### Forwarding Network

**MUX Structure**:
```verilog
always_comb begin
  case (forward_a_0)
    3'b000: operand_a_0 = id_ex_0.rs1_data;  // No forwarding
    3'b001: operand_a_0 = ex_fwd_data_0;     // From EX/MEM
    3'b010: operand_a_0 = ex_fwd_data_1;
    3'b011: operand_a_0 = mem_fwd_data_0;    // From MEM/WB
    3'b100: operand_a_0 = mem_fwd_data_1;
    3'b101: operand_a_0 = wb_fwd_data_0;     // From WB
    3'b110: operand_a_0 = wb_fwd_data_1;
  endcase
end
```

**Forwarding Sources**:
- EX/MEM: ALU result from previous instruction in EX stage
- MEM/WB: Loaded data or ALU result from MEM stage
- WB: Final write-back data

### Result Selection

**Choose Result Based on Instruction Type**:
```verilog
if (id_ex_0.itype == ITYPE_MUL)
  ex_mem_0.alu_result = {16'h0, mul_result_lo_0};
else if (id_ex_0.itype == ITYPE_MOV && id_ex_0.specifier == 8'h02)
  ex_mem_0.alu_result = {16'h0, operand_a_0};  // Register move
else
  ex_mem_0.alu_result = alu_result_0;
```

### Branch Decision

**Priority**:
```verilog
if (ex_mem_0.branch_taken) begin
  branch_taken = 1'b1;
  branch_target = ex_mem_0.branch_target;
end else if (ex_mem_1.branch_taken) begin
  branch_taken = 1'b1;
  branch_target = ex_mem_1.branch_target;
end
```

### Timing

- **Latency**: 1 cycle (combinational through ALU/multiply/branch)
- **Critical Path**: Forwarding MUX → ALU → Result selection

---

## Memory Stage (memory_stage.sv)

### Purpose
Handle load and store operations to unified memory.

### State Machine

**States**:
- **MEM_IDLE**: No active memory transaction
- **MEM_ACCESS_0**: Accessing memory for instruction 0
- **MEM_ACCESS_1**: Accessing memory for instruction 1
- **MEM_WAIT**: Waiting for memory acknowledge

### Arbitration Logic

**Priority**: Instruction 0 > Instruction 1

**Sequence**:
1. If inst0 needs memory, access it first
2. If inst1 also needs memory, access it next cycle (stall)
3. If only inst1 needs memory, access it immediately

### Big-Endian Data Formatting

**Write**:
```verilog
case (mem_size)
  MEM_BYTE:  dmem_wdata = {24'h0, wdata[7:0]};
  MEM_HALF:  dmem_wdata = {16'h0, wdata[15:0]};
  MEM_WORD:  dmem_wdata = {wdata[15:0], wdata[15:0]};  // Two 16-bit regs
endcase
```

**Read**:
```verilog
case (mem_size)
  MEM_BYTE:  mem_result = {24'h0, dmem_rdata[7:0]};
  MEM_HALF:  mem_result = {16'h0, dmem_rdata[15:0]};
  MEM_WORD:  mem_result = dmem_rdata[31:0];
endcase
```

### Result Pass-Through

**For Non-Memory Instructions**:
```verilog
if (ex_mem_0.mem_read)
  mem_wb_0.wb_data = mem_result_0[15:0];
else
  mem_wb_0.wb_data = ex_mem_0.alu_result[15:0];
```

### Timing

- **Latency**: 1 cycle (single memory access), 2 cycles (if both need memory)
- **Stall**: Asserted when second instruction waits for memory

---

## Writeback Stage (writeback_stage.sv)

### Purpose
Write results to register file and update processor flags.

### Register Write Arbitration

**Port 0**: Always instruction 0's primary destination
```verilog
rf_wr_addr_0 = mem_wb_0.rd_addr;
rf_wr_data_0 = mem_wb_0.wb_data;
rf_wr_en_0 = mem_wb_0.valid && mem_wb_0.rd_we;
```

**Port 1**: Instruction 0's secondary dest OR instruction 1's primary
```verilog
if (mem_wb_0.valid && mem_wb_0.rd2_we) begin
  // UMULL/SMULL high result
  rf_wr_addr_1 = mem_wb_0.rd2_addr;
  rf_wr_data_1 = mem_wb_0.wb_data2;
  rf_wr_en_1 = 1'b1;
end else if (mem_wb_1.valid && mem_wb_1.rd_we) begin
  // Instruction 1's result
  rf_wr_addr_1 = mem_wb_1.rd_addr;
  rf_wr_data_1 = mem_wb_1.wb_data;
  rf_wr_en_1 = 1'b1;
end
```

### Flag Updates

**Priority**: Instruction 1 > Instruction 0 (most recent wins)
```verilog
if (mem_wb_1.valid) begin
  z_flag_value = mem_wb_1.z_flag;
  v_flag_value = mem_wb_1.v_flag;
end else if (mem_wb_0.valid) begin
  z_flag_value = mem_wb_0.z_flag;
  v_flag_value = mem_wb_0.v_flag;
end
```

### Halt Detection

```verilog
halted = (mem_wb_0.valid && mem_wb_0.is_halt) ||
         (mem_wb_1.valid && mem_wb_1.is_halt);
```

### Timing

- **Latency**: 1 cycle (register file write on next clock edge)

---

## Hazard Unit (hazard_unit.sv)

### Purpose
Detect data hazards and generate forwarding control signals.

### Forwarding Detection

**For Each Source Operand**:
Check all possible forwarding sources in priority order:
1. EX stage slot 0
2. EX stage slot 1
3. MEM stage slot 0
4. MEM stage slot 1
5. WB stage slot 0
6. WB stage slot 1

**Example for inst0, operand A**:
```verilog
if (ex_valid_0 && ex_rd_we_0 && (ex_rd_addr_0 == id_rs1_addr_0))
  forward_a_0 = 3'b001;  // Forward from EX slot 0
else if (ex_valid_1 && ex_rd_we_1 && (ex_rd_addr_1 == id_rs1_addr_0))
  forward_a_0 = 3'b010;  // Forward from EX slot 1
// ... check MEM and WB stages
else
  forward_a_0 = 3'b000;  // No forwarding
```

### Load-Use Hazard Detection

```verilog
if (id_valid_0 && ex_valid_0 && ex_mem_read_0) begin
  if ((ex_rd_addr_0 == id_rs1_addr_0) || (ex_rd_addr_0 == id_rs2_addr_0)) begin
    load_use_hazard_0 = 1'b1;
  end
end
```

### Stall and Flush Generation

```verilog
if (load_use_hazard_0 || load_use_hazard_1) begin
  stall = 1'b1;
  flush_ex = 1'b1;  // Insert bubble in EX stage
end
```

### Timing

- **Latency**: Combinational (critical path)
- **Critical Path**: Multiple comparators for all forwarding sources

---

## Unified Memory (unified_memory.sv)

### Purpose
Provide single unified memory for both instructions and data with dual-port access.

### Memory Array

```verilog
logic [7:0] mem [0:MEM_SIZE_BYTES-1];  // Byte-addressable
```

**Default Size**: 64 KB (65536 bytes)

### Instruction Fetch Port

**Operation**:
```verilog
if (if_req) begin
  if_rdata <= {
    mem[if_addr + 0],   // MSB
    mem[if_addr + 1],
    ...
    mem[if_addr + 15]   // LSB (16 bytes total)
  };
  if_ack <= 1'b1;
end
```

**Big-Endian**: First byte (lowest address) goes to MSB of if_rdata

### Data Access Port

**Read**:
```verilog
if (data_req && !data_we) begin
  case (data_size)
    2'b00: data_rdata <= {24'h0, mem[data_addr]};  // Byte
    2'b01: data_rdata <= {16'h0, mem[data_addr], mem[data_addr+1]};  // Halfword
    2'b10: data_rdata <= {mem[data_addr], mem[data_addr+1],
                          mem[data_addr+2], mem[data_addr+3]};  // Word
  endcase
  data_ack <= 1'b1;
end
```

**Write**:
```verilog
if (data_req && data_we) begin
  case (data_size)
    2'b00: mem[data_addr] <= data_wdata[7:0];  // Byte
    2'b01: begin  // Halfword
      mem[data_addr + 0] <= data_wdata[15:8];  // MSB
      mem[data_addr + 1] <= data_wdata[7:0];   // LSB
    end
    2'b10: begin  // Word
      mem[data_addr + 0] <= data_wdata[31:24];  // MSB
      mem[data_addr + 1] <= data_wdata[23:16];
      mem[data_addr + 2] <= data_wdata[15:8];
      mem[data_addr + 3] <= data_wdata[7:0];   // LSB
    end
  endcase
  data_ack <= 1'b1;
end
```

### Timing

- **Latency**: 1 cycle (registered outputs)
- **Throughput**: 1 access per port per cycle (true dual-port)

---

## Pipeline Registers (pipeline_regs.sv)

### Purpose
Hold pipeline state between stages with stall and flush capability.

### Common Structure

Each pipeline register module has:
```verilog
input  logic   clk, rst;
input  logic   stall;  // Freeze current data
input  logic   flush;  // Insert NOP (bubble)
input  type_t  data_in;
output type_t  data_out;
```

### Behavior

```verilog
always_ff @(posedge clk) begin
  if (rst || flush) begin
    data_out <= RESET_VALUE;  // Clear or NOP
  end else if (!stall) begin
    data_out <= data_in;  // Normal advancement
  end
  // else: stalled, keep current value
end
```

### Pipeline Register Types

1. **if_id_reg**: Holds fetched instructions (if_id_t)
2. **id_ex_reg**: Holds decoded control and operands (id_ex_t)
3. **ex_mem_reg**: Holds execution results (ex_mem_t)
4. **mem_wb_reg**: Holds memory/ALU results (mem_wb_t)

### Timing

- **Latency**: 1 cycle (registered on clock edge)
- **Stall**: Holds current data indefinitely
- **Flush**: Inserts NOP in one cycle

---

## Summary

The NeoCore16x32 microarchitecture implements a dual-issue, 5-stage pipeline with:

1. **Modular Design**: Clear separation of pipeline stages
2. **Comprehensive Forwarding**: Minimize stalls with multi-source forwarding
3. **Dual-Issue Logic**: Hardware checks for hazards before issuing pairs
4. **Big-Endian Throughout**: Consistent byte ordering in all modules
5. **FPGA-Optimized**: Uses BRAM efficiently, achieves timing goals

All modules are synthesizable and verified through comprehensive testbenches. The design balances performance (dual-issue) with complexity (in-order pipeline, no speculation beyond branches).

