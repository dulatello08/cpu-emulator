//
// fetch_unit.sv
// NeoCore 16x32 CPU - Instruction Fetch Unit
//
// Fetches variable-length instructions from memory.
// Maintains instruction buffer for dual-issue capability.
// Handles PC updates for sequential execution and branches.
//

module fetch_unit
  import neocore_pkg::*;
(
  input  logic        clk,
  input  logic        rst,
  
  // PC control
  input  logic        branch_taken,
  input  logic [31:0] branch_target,
  input  logic        stall,        // Stall fetch (from hazard detection)
  
  // Memory interface
  output logic [31:0] imem_addr,
  output logic        imem_req,
  input  logic [63:0] imem_rdata,   // 8 bytes of instruction data
  input  logic        imem_ack,
  
  // Output to decode
  output logic [71:0] inst_data_0,  // First instruction (up to 9 bytes)
  output logic [3:0]  inst_len_0,   // First instruction length
  output logic [31:0] pc_0,         // PC of first instruction
  output logic        valid_0,      // First instruction valid
  
  output logic [71:0] inst_data_1,  // Second instruction (for dual-issue)
  output logic [3:0]  inst_len_1,
  output logic [31:0] pc_1,
  output logic        valid_1
);

  // ============================================================================
  // Program Counter
  // ============================================================================
  
  logic [31:0] pc;
  logic [31:0] pc_next;
  
  always_ff @(posedge clk) begin
    if (rst) begin
      pc <= 32'h0000_0000;
    end else if (!stall) begin
      pc <= pc_next;
    end
  end
  
  // ============================================================================
  // Fetch Buffer
  // ============================================================================
  
  // Buffer to hold fetched instruction bytes
  logic [127:0] fetch_buffer;  // 16 bytes
  logic [4:0]   buffer_valid;  // Number of valid bytes in buffer
  logic [31:0]  buffer_pc;     // PC of first byte in buffer
  
  always_ff @(posedge clk) begin
    if (rst) begin
      fetch_buffer <= 128'h0;
      buffer_valid <= 5'h0;
      buffer_pc <= 32'h0;
    end else if (branch_taken) begin
      // Flush buffer on branch
      fetch_buffer <= 128'h0;
      buffer_valid <= 5'h0;
      buffer_pc <= branch_target;
    end else if (imem_ack && !stall) begin
      // Shift out consumed bytes and add new bytes
      // For simplicity, we fetch 8 new bytes each cycle
      // and shift out bytes consumed by decoded instructions
      logic [4:0] consumed_bytes;
      consumed_bytes = (valid_0 ? inst_len_0 : 5'h0) + (valid_1 ? inst_len_1 : 5'h0);
      
      // Shift buffer
      fetch_buffer <= {imem_rdata, fetch_buffer[127:64]};
      buffer_valid <= buffer_valid + 5'd8 - consumed_bytes;
      buffer_pc <= buffer_pc + {27'h0, consumed_bytes};
    end
  end
  
  // ============================================================================
  // Instruction Pre-Decode (Length Detection)
  // ============================================================================
  
  logic [7:0] spec_0, op_0;
  logic [7:0] spec_1, op_1;
  
  always_comb begin
    // Extract specifier and opcode for first instruction
    spec_0 = fetch_buffer[7:0];
    op_0 = fetch_buffer[15:8];
    
    // Calculate first instruction length
    inst_len_0 = get_inst_length(op_0, spec_0);
    
    // Extract second instruction (starts after first)
    if (inst_len_0 <= buffer_valid) begin
      case (inst_len_0)
        4'd2: begin
          spec_1 = fetch_buffer[23:16];
          op_1 = fetch_buffer[31:24];
        end
        4'd3: begin
          spec_1 = fetch_buffer[31:24];
          op_1 = fetch_buffer[39:32];
        end
        4'd4: begin
          spec_1 = fetch_buffer[39:32];
          op_1 = fetch_buffer[47:40];
        end
        4'd5: begin
          spec_1 = fetch_buffer[47:40];
          op_1 = fetch_buffer[55:48];
        end
        4'd6: begin
          spec_1 = fetch_buffer[55:48];
          op_1 = fetch_buffer[63:56];
        end
        4'd7: begin
          spec_1 = fetch_buffer[63:56];
          op_1 = fetch_buffer[71:64];
        end
        4'd8: begin
          spec_1 = fetch_buffer[71:64];
          op_1 = fetch_buffer[79:72];
        end
        4'd9: begin
          spec_1 = fetch_buffer[79:72];
          op_1 = fetch_buffer[87:80];
        end
        default: begin
          spec_1 = 8'h00;
          op_1 = 8'h00;
        end
      endcase
    end else begin
      spec_1 = 8'h00;
      op_1 = 8'h00;
    end
    
    inst_len_1 = get_inst_length(op_1, spec_1);
  end
  
  // ============================================================================
  // Instruction Output
  // ============================================================================
  
  always_comb begin
    // First instruction
    valid_0 = (buffer_valid >= inst_len_0) && !branch_taken;
    inst_data_0 = fetch_buffer[71:0];
    pc_0 = buffer_pc;
    
    // Second instruction (dual-issue)
    // Only valid if first is valid and buffer has enough bytes
    valid_1 = valid_0 && (buffer_valid >= (inst_len_0 + inst_len_1)) && !branch_taken;
    
    // Extract second instruction data (shift by first instruction length)
    case (inst_len_0)
      4'd2: inst_data_1 = fetch_buffer[87:16];
      4'd3: inst_data_1 = fetch_buffer[95:24];
      4'd4: inst_data_1 = fetch_buffer[103:32];
      4'd5: inst_data_1 = fetch_buffer[111:40];
      4'd6: inst_data_1 = fetch_buffer[119:48];
      4'd7: inst_data_1 = fetch_buffer[127:56];
      4'd8: inst_data_1 = {8'h00, fetch_buffer[127:64]};
      4'd9: inst_data_1 = 72'h0;  // Not enough space
      default: inst_data_1 = 72'h0;
    endcase
    
    pc_1 = buffer_pc + {28'h0, inst_len_0};
  end
  
  // ============================================================================
  // Memory Request
  // ============================================================================
  
  always_comb begin
    // Request memory when buffer needs refilling
    imem_req = (buffer_valid < 5'd12) && !stall;
    imem_addr = pc;
  end
  
  // ============================================================================
  // PC Update Logic
  // ============================================================================
  
  always_comb begin
    if (branch_taken) begin
      pc_next = branch_target;
    end else if (!stall) begin
      // Sequential execution: advance by number of bytes consumed
      logic [4:0] consumed;
      consumed = (valid_0 ? inst_len_0 : 5'h0) + (valid_1 ? inst_len_1 : 5'h0);
      pc_next = pc + {27'h0, consumed};
    end else begin
      pc_next = pc;
    end
  end

endmodule : fetch_unit
