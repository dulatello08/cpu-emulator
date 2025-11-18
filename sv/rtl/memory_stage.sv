//
// memory_stage.sv
// NeoCore 16x32 CPU - Memory Access Stage
//
// Handles load and store operations.
// Interfaces with unified Von Neumann memory.
// Supports dual-issue (two memory operations sequentially or one per cycle).
//
// Big-Endian Memory Model:
//   - Data is stored in big-endian format in memory.
//   - The unified_memory module handles byte ordering internally.
//   - This module passes data through with minimal transformation.
//

module memory_stage
  import neocore_pkg::*;
(
  input  logic        clk,
  input  logic        rst,
  
  // From EX/MEM register (instruction 0)
  input  ex_mem_t     ex_mem_0,
  
  // From EX/MEM register (instruction 1)
  input  ex_mem_t     ex_mem_1,
  
  // Data memory interface (unified memory)
  output logic [31:0] dmem_addr,
  output logic [31:0] dmem_wdata,
  output logic [1:0]  dmem_size,
  output logic        dmem_we,
  output logic        dmem_req,
  input  logic [31:0] dmem_rdata,
  input  logic        dmem_ack,
  
  // Outputs to MEM/WB register
  output mem_wb_t     mem_wb_0,
  output mem_wb_t     mem_wb_1,
  
  // Stall signal (if memory not ready)
  output logic        mem_stall
);

  // ============================================================================
  // Memory Access Arbitration
  // ============================================================================
  
  // Determine which instruction (if any) accesses memory
  logic access_mem_0, access_mem_1;
  logic [31:0] mem_result_0, mem_result_1;
  
  assign access_mem_0 = ex_mem_0.valid && (ex_mem_0.mem_read || ex_mem_0.mem_write);
  assign access_mem_1 = ex_mem_1.valid && (ex_mem_1.mem_read || ex_mem_1.mem_write);
  
  // Memory access state machine
  typedef enum logic [1:0] {
    MEM_IDLE,
    MEM_ACCESS_0,
    MEM_ACCESS_1,
    MEM_WAIT
  } mem_state_e;
  
  mem_state_e mem_state, mem_state_next;
  
  always_ff @(posedge clk) begin
    if (rst) begin
      mem_state <= MEM_IDLE;
    end else begin
      mem_state <= mem_state_next;
    end
  end
  
  // State machine logic
  always_comb begin
    mem_state_next = mem_state;
    dmem_req = 1'b0;
    dmem_addr = 32'h0;
    dmem_wdata = 32'h0;
    dmem_size = 2'b00;
    dmem_we = 1'b0;
    mem_stall = 1'b0;
    mem_result_0 = ex_mem_0.alu_result;
    mem_result_1 = ex_mem_1.alu_result;
    
    case (mem_state)
      MEM_IDLE: begin
        // Check if instruction 0 needs memory
        if (access_mem_0) begin
          dmem_req = 1'b1;
          dmem_addr = ex_mem_0.mem_addr;
          dmem_size = ex_mem_0.mem_size;
          dmem_we = ex_mem_0.mem_write;
          
          if (ex_mem_0.mem_write) begin
            // For writes, pass data directly (unified_memory handles endianness)
            case (ex_mem_0.mem_size)
              MEM_BYTE: dmem_wdata = {24'h0, ex_mem_0.mem_wdata[7:0]};
              MEM_HALF: dmem_wdata = {16'h0, ex_mem_0.mem_wdata[15:0]};
              MEM_WORD: dmem_wdata = {ex_mem_0.mem_wdata[15:0], ex_mem_0.mem_wdata[15:0]};
              default:  dmem_wdata = {16'h0, ex_mem_0.mem_wdata[15:0]};
            endcase
          end
          
          if (dmem_ack) begin
            if (ex_mem_0.mem_read) begin
              // For reads, extract appropriate bits from big-endian data
              case (ex_mem_0.mem_size)
                MEM_BYTE: mem_result_0 = {24'h0, dmem_rdata[7:0]};
                MEM_HALF: mem_result_0 = {16'h0, dmem_rdata[15:0]};
                MEM_WORD: mem_result_0 = dmem_rdata;
                default:  mem_result_0 = {16'h0, dmem_rdata[15:0]};
              endcase
            end
            // Check if instruction 1 also needs memory
            if (access_mem_1) begin
              mem_state_next = MEM_ACCESS_1;
              mem_stall = 1'b1;  // Stall to handle second memory access
            end else begin
              mem_state_next = MEM_IDLE;
            end
          end else begin
            mem_state_next = MEM_WAIT;
            mem_stall = 1'b1;
          end
        end
        // Instruction 0 doesn't need memory, check instruction 1
        else if (access_mem_1) begin
          dmem_req = 1'b1;
          dmem_addr = ex_mem_1.mem_addr;
          dmem_size = ex_mem_1.mem_size;
          dmem_we = ex_mem_1.mem_write;
          
          if (ex_mem_1.mem_write) begin
            // For writes, pass data directly (unified_memory handles endianness)
            case (ex_mem_1.mem_size)
              MEM_BYTE: dmem_wdata = {24'h0, ex_mem_1.mem_wdata[7:0]};
              MEM_HALF: dmem_wdata = {16'h0, ex_mem_1.mem_wdata[15:0]};
              MEM_WORD: dmem_wdata = {ex_mem_1.mem_wdata[15:0], ex_mem_1.mem_wdata[15:0]};
              default:  dmem_wdata = {16'h0, ex_mem_1.mem_wdata[15:0]};
            endcase
          end
          
          if (dmem_ack) begin
            if (ex_mem_1.mem_read) begin
              // For reads, extract appropriate bits from big-endian data
              case (ex_mem_1.mem_size)
                MEM_BYTE: mem_result_1 = {24'h0, dmem_rdata[7:0]};
                MEM_HALF: mem_result_1 = {16'h0, dmem_rdata[15:0]};
                MEM_WORD: mem_result_1 = dmem_rdata;
                default:  mem_result_1 = {16'h0, dmem_rdata[15:0]};
              endcase
            end
            mem_state_next = MEM_IDLE;
          end else begin
            mem_state_next = MEM_WAIT;
            mem_stall = 1'b1;
          end
        end
        // No memory access needed
        else begin
          mem_state_next = MEM_IDLE;
        end
      end
      
      MEM_ACCESS_1: begin
        // Access memory for instruction 1
        dmem_req = 1'b1;
        dmem_addr = ex_mem_1.mem_addr;
        dmem_size = ex_mem_1.mem_size;
        dmem_we = ex_mem_1.mem_write;
        
        if (ex_mem_1.mem_write) begin
          // For writes, pass data directly (unified_memory handles endianness)
          case (ex_mem_1.mem_size)
            MEM_BYTE: dmem_wdata = {24'h0, ex_mem_1.mem_wdata[7:0]};
            MEM_HALF: dmem_wdata = {16'h0, ex_mem_1.mem_wdata[15:0]};
            MEM_WORD: dmem_wdata = {ex_mem_1.mem_wdata[15:0], ex_mem_1.mem_wdata[15:0]};
            default:  dmem_wdata = {16'h0, ex_mem_1.mem_wdata[15:0]};
          endcase
        end
        
        if (dmem_ack) begin
          if (ex_mem_1.mem_read) begin
            // For reads, extract appropriate bits from big-endian data
            case (ex_mem_1.mem_size)
              MEM_BYTE: mem_result_1 = {24'h0, dmem_rdata[7:0]};
              MEM_HALF: mem_result_1 = {16'h0, dmem_rdata[15:0]};
              MEM_WORD: mem_result_1 = dmem_rdata;
              default:  mem_result_1 = {16'h0, dmem_rdata[15:0]};
            endcase
          end
          mem_state_next = MEM_IDLE;
        end else begin
          mem_state_next = MEM_WAIT;
          mem_stall = 1'b1;
        end
      end
      
      MEM_WAIT: begin
        // Wait for memory to acknowledge
        mem_stall = 1'b1;
        if (dmem_ack) begin
          mem_state_next = MEM_IDLE;
        end
      end
      
      default: begin
        mem_state_next = MEM_IDLE;
      end
    endcase
  end
  
  // ============================================================================
  // Output to MEM/WB Register (Instruction 0)
  // ============================================================================
  
  always_comb begin
    mem_wb_0.valid = ex_mem_0.valid;
    mem_wb_0.pc = ex_mem_0.pc;
    mem_wb_0.rd_addr = ex_mem_0.rd_addr;
    mem_wb_0.rd2_addr = ex_mem_0.rd2_addr;
    mem_wb_0.rd_we = ex_mem_0.rd_we;
    mem_wb_0.rd2_we = ex_mem_0.rd2_we;
    mem_wb_0.z_flag = ex_mem_0.z_flag;
    mem_wb_0.v_flag = ex_mem_0.v_flag;
    mem_wb_0.is_halt = ex_mem_0.is_halt;
    
    // Select write-back data
    if (ex_mem_0.mem_read) begin
      // Load instruction: use memory data
      mem_wb_0.wb_data = mem_result_0[15:0];
      mem_wb_0.wb_data2 = mem_result_0[31:16];  // For 32-bit loads
    end else begin
      // Non-load: use ALU result
      mem_wb_0.wb_data = ex_mem_0.alu_result[15:0];
      mem_wb_0.wb_data2 = ex_mem_0.alu_result[31:16];
    end
  end
  
  // ============================================================================
  // Output to MEM/WB Register (Instruction 1)
  // ============================================================================
  
  always_comb begin
    mem_wb_1.valid = ex_mem_1.valid;
    mem_wb_1.pc = ex_mem_1.pc;
    mem_wb_1.rd_addr = ex_mem_1.rd_addr;
    mem_wb_1.rd2_addr = ex_mem_1.rd2_addr;
    mem_wb_1.rd_we = ex_mem_1.rd_we;
    mem_wb_1.rd2_we = ex_mem_1.rd2_we;
    mem_wb_1.z_flag = ex_mem_1.z_flag;
    mem_wb_1.v_flag = ex_mem_1.v_flag;
    mem_wb_1.is_halt = ex_mem_1.is_halt;
    
    if (ex_mem_1.mem_read) begin
      mem_wb_1.wb_data = mem_result_1[15:0];
      mem_wb_1.wb_data2 = mem_result_1[31:16];
    end else begin
      mem_wb_1.wb_data = ex_mem_1.alu_result[15:0];
      mem_wb_1.wb_data2 = ex_mem_1.alu_result[31:16];
    end
  end

endmodule : memory_stage
