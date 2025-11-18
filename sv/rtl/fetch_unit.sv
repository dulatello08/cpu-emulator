//
// fetch_unit.sv
// NeoCore 16x32 CPU - Instruction Fetch Unit (Fixed)
//
// Fetches variable-length instructions from unified memory.
// Maintains instruction buffer for dual-issue capability.
// Handles PC updates for sequential execution and branches.
//
// Big-Endian Memory Model:
//   Instructions are stored in big-endian format.
//   Byte at address N is more significant than byte at address N+1.
//
// Instruction Format (from machine description):
//   Byte 0: Specifier
//   Byte 1: Opcode
//   Bytes 2+: Operands (varying length)
//
// Buffer Management Strategy:
//   - Maintain 32-byte circular buffer
//   - Track number of valid bytes
//   - Extract instructions from top of buffer (big-endian)
//   - Shift out consumed bytes and refill from bottom
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
  
  // Unified memory interface (wide fetch for variable-length instructions)
  output logic [31:0] mem_addr,
  output logic        mem_req,
  input  logic [127:0] mem_rdata,   // 16 bytes of instruction data (big-endian)
  input  logic        mem_ack,
  
  // Output to decode
  output logic [103:0] inst_data_0,  // First instruction (up to 13 bytes)
  output logic [3:0]   inst_len_0,   // First instruction length
  output logic [31:0]  pc_0,         // PC of first instruction
  output logic         valid_0,      // First instruction valid
  
  output logic [103:0] inst_data_1,  // Second instruction (for dual-issue)
  output logic [3:0]   inst_len_1,
  output logic [31:0]  pc_1,
  output logic         valid_1
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
  
  // Buffer to hold fetched instruction bytes (big-endian)
  // We maintain a 32-byte buffer to handle:
  // - Up to 13-byte instructions
  // - Alignment issues
  // - Dual-issue (two instructions)
  logic [255:0] fetch_buffer;  // 32 bytes
  logic [5:0]   buffer_valid;  // Number of valid bytes in buffer
  logic [31:0]  buffer_pc;     // PC of first byte in buffer
  
  // Calculate consumed bytes (combinational)
  logic [5:0] consumed_bytes;
  logic       can_consume_0, can_consume_1;
  
  always_comb begin
    can_consume_0 = (buffer_valid >= {2'b0, inst_len_0}) && (inst_len_0 > 0) && !branch_taken;
    can_consume_1 = can_consume_0 && 
                    (buffer_valid >= ({2'b0, inst_len_0} + {2'b0, inst_len_1})) && 
                    (inst_len_1 > 0);
    
    if (!stall) begin
      consumed_bytes = (can_consume_0 ? {2'b0, inst_len_0} : 6'h0) + 
                      (can_consume_1 ? {2'b0, inst_len_1} : 6'h0);
    end else begin
      consumed_bytes = 6'h0;
    end
  end
  
  always_ff @(posedge clk) begin
    if (rst) begin
      fetch_buffer <= 256'h0;
      buffer_valid <= 6'h0;
      buffer_pc <= 32'h0;
    end else if (branch_taken) begin
      // Flush buffer on branch
      fetch_buffer <= 256'h0;
      buffer_valid <= 6'h0;
      buffer_pc <= branch_target;
    end else if (!stall) begin
      // Handle buffer consumption and refill
      if (mem_ack && consumed_bytes > 0) begin
        // Both consume and refill
        // Shift out consumed bytes and append new data at bottom
        fetch_buffer <= (fetch_buffer << (consumed_bytes * 8)) | 
                       {128'h0, mem_rdata};
        buffer_valid <= buffer_valid - consumed_bytes + 6'd16;
        buffer_pc <= buffer_pc + {26'h0, consumed_bytes};
      end else if (mem_ack) begin
        // Only refill (no consumption)
        fetch_buffer <= (fetch_buffer << 128) | {128'h0, mem_rdata};
        buffer_valid <= buffer_valid + 6'd16;
        if (buffer_valid == 0) begin
          buffer_pc <= pc;  // Initialize buffer_pc on first fetch
        end
      end else if (consumed_bytes > 0) begin
        // Only consume (no refill)
        fetch_buffer <= fetch_buffer << (consumed_bytes * 8);
        buffer_valid <= buffer_valid - consumed_bytes;
        buffer_pc <= buffer_pc + {26'h0, consumed_bytes};
      end
      // else: no change
    end
  end
  
  // ============================================================================
  // Instruction Pre-Decode (Length Detection)
  // ============================================================================
  
  // Extract bytes for first instruction (big-endian: MSB at top)
  logic [7:0] spec_0, op_0;
  logic [7:0] spec_1, op_1;
  
  always_comb begin
    // Extract specifier and opcode for first instruction from buffer
    // Buffer is big-endian, so MSB bytes are at top
    spec_0 = fetch_buffer[255:248];  // Byte 0 (specifier)
    op_0 = fetch_buffer[247:240];    // Byte 1 (opcode)
    
    // Calculate first instruction length
    inst_len_0 = get_inst_length(op_0, spec_0);
    
    // Extract second instruction (starts after first)
    // Need to shift by inst_len_0 bytes
    if ({2'b0, inst_len_0} <= buffer_valid) begin
      case (inst_len_0)
        4'd2: begin
          spec_1 = fetch_buffer[239:232];  // After 2 bytes
          op_1 = fetch_buffer[231:224];
        end
        4'd3: begin
          spec_1 = fetch_buffer[231:224];  // After 3 bytes
          op_1 = fetch_buffer[223:216];
        end
        4'd4: begin
          spec_1 = fetch_buffer[223:216];  // After 4 bytes
          op_1 = fetch_buffer[215:208];
        end
        4'd5: begin
          spec_1 = fetch_buffer[215:208];  // After 5 bytes
          op_1 = fetch_buffer[207:200];
        end
        4'd6: begin
          spec_1 = fetch_buffer[207:200];  // After 6 bytes
          op_1 = fetch_buffer[199:192];
        end
        4'd7: begin
          spec_1 = fetch_buffer[199:192];  // After 7 bytes
          op_1 = fetch_buffer[191:184];
        end
        4'd8: begin
          spec_1 = fetch_buffer[191:184];  // After 8 bytes
          op_1 = fetch_buffer[183:176];
        end
        4'd9: begin
          spec_1 = fetch_buffer[183:176];  // After 9 bytes
          op_1 = fetch_buffer[175:168];
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
    valid_0 = (buffer_valid >= {2'b0, inst_len_0}) && !branch_taken && (inst_len_0 > 0);
    
    // Extract instruction bytes (up to 13 bytes)
    // Big-endian: top bytes are most significant
    inst_data_0 = fetch_buffer[255:152];  // Top 13 bytes
    pc_0 = buffer_pc;
    
    // Second instruction (dual-issue)
    // Only valid if first is valid and buffer has enough bytes
    valid_1 = valid_0 && 
              (buffer_valid >= ({2'b0, inst_len_0} + {2'b0, inst_len_1})) && 
              !branch_taken &&
              (inst_len_1 > 0);
    
    // Extract second instruction data (shifted by first instruction length)
    case (inst_len_0)
      4'd2:  inst_data_1 = fetch_buffer[239:136];  // After 2 bytes
      4'd3:  inst_data_1 = fetch_buffer[231:128];  // After 3 bytes
      4'd4:  inst_data_1 = fetch_buffer[223:120];  // After 4 bytes
      4'd5:  inst_data_1 = fetch_buffer[215:112];  // After 5 bytes
      4'd6:  inst_data_1 = fetch_buffer[207:104];  // After 6 bytes
      4'd7:  inst_data_1 = fetch_buffer[199:96];   // After 7 bytes
      4'd8:  inst_data_1 = fetch_buffer[191:88];   // After 8 bytes
      4'd9:  inst_data_1 = fetch_buffer[183:80];   // After 9 bytes
      default: inst_data_1 = 104'h0;
    endcase
    
    pc_1 = buffer_pc + {28'h0, inst_len_0};
  end
  
  // ============================================================================
  // Memory Request
  // ============================================================================
  
  always_comb begin
    // Request memory when buffer needs refilling
    // Keep buffer topped up to handle dual-issue and long instructions
    mem_req = (buffer_valid < 6'd20) && !stall && !branch_taken;
    mem_addr = pc;
  end
  
  // ============================================================================
  // PC Update Logic
  // ============================================================================
  
  always_comb begin
    if (branch_taken) begin
      pc_next = branch_target;
    end else if (!stall) begin
      // Sequential execution: advance by number of bytes consumed
      pc_next = pc + {26'h0, consumed_bytes};
    end else begin
      pc_next = pc;
    end
  end

endmodule : fetch_unit
