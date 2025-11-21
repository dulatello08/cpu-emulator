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
  input  logic        dual_issue,   // Dual-issue enable from issue unit
  
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
  
  // NOTE: The actual program counter is buffer_pc, which tracks the PC of the
  // first byte in the instruction buffer. This pc variable is NOT used and
  // should be removed, but kept for now to avoid breaking other logic.
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
  // 
  // Using byte array for clarity and correctness
  logic [7:0]   fetch_buffer[32];  // 32 bytes, index 0 = first byte
  logic [5:0]   buffer_valid;      // Number of valid bytes in buffer
  logic [31:0]  buffer_pc;         // PC of first byte in buffer
  
  // Calculate consumed bytes (combinational)
  logic [5:0] consumed_bytes;
  logic       can_consume_0, can_consume_1;
  logic [5:0] new_buffer_valid;
  logic [5:0] refill_amount;  // Used in always_ff for refill calculation
  
  always_comb begin
    can_consume_0 = (buffer_valid >= {2'b0, inst_len_0}) && (inst_len_0 > 0) && !branch_taken;
    can_consume_1 = can_consume_0 && 
                    (buffer_valid >= ({2'b0, inst_len_0} + {2'b0, inst_len_1})) && 
                    (inst_len_1 > 0) &&
                    dual_issue &&
                    (op_1 != OP_HLT);  // Never consume HLT in slot 1
    
    if (!stall) begin
      consumed_bytes = (can_consume_0 ? {2'b0, inst_len_0} : 6'h0) + 
                      (can_consume_1 ? {2'b0, inst_len_1} : 6'h0);
    end else begin
      consumed_bytes = 6'h0;
    end
    
    // Calculate new buffer state after consumption
    new_buffer_valid = buffer_valid - consumed_bytes;
  end
  
  always_ff @(posedge clk) begin
    if (rst) begin
      for (int i = 0; i < 32; i++) begin
        fetch_buffer[i] <= 8'h00;
      end
      buffer_valid <= 6'h0;
      buffer_pc <= 32'h0;
    end else if (branch_taken) begin
      // DEBUG logging
      if ($time/10000 < 25) begin
        $display("[FETCH] Cycle %0d: PC=%h BufPC=%h BufValid=%0d Consumed=%0d MemAck=%b", 
                 $time/10000, buffer_pc, buffer_pc, buffer_valid, consumed_bytes, mem_ack);
        if (buffer_valid >= 4) begin
          $display("        Buf[0:5]=%h %h %h %h %h %h Spec0=%h Op0=%h", 
                   fetch_buffer[0], fetch_buffer[1], fetch_buffer[2], fetch_buffer[3],
                   fetch_buffer[4], fetch_buffer[5], spec_0, op_0);
        end
      end
      // Flush buffer on branch
      for (int i = 0; i < 32; i++) begin
        fetch_buffer[i] <= 8'h00;
      end
      buffer_valid <= 6'h0;
      buffer_pc <= branch_target;
    end else if (!stall) begin
      // Handle THREE cases with explicit byte operations:
      // 1. Consume only
      // 2. Refill only  
      // 3. Consume AND refill
      
      // DEBUG
      if ($time/10000 < 25) begin
        $display("[FETCH] Cyc %0d: BufPC=%h BufV=%0d Cons=%0d MemAck=%b NewV=%0d MemAddr=%h",
                 $time/10000, buffer_pc, buffer_valid, consumed_bytes, mem_ack, new_buffer_valid, mem_addr);
        if (buffer_valid >= 6) $display("        Buf[0:5]=%h %h %h %h %h %h", 
                 fetch_buffer[0], fetch_buffer[1], fetch_buffer[2], fetch_buffer[3], fetch_buffer[4], fetch_buffer[5]);
      end
      
      if (consumed_bytes > 0 && mem_ack) begin
        // Case 3: BOTH consume and refill in same cycle
        refill_amount = (new_buffer_valid >= 6'd32) ? 6'd0 :
                       (new_buffer_valid + 6'd16 > 6'd32) ? (6'd32 - new_buffer_valid) : 
                       6'd16;
        
        // Step 1: Shift remaining bytes to front
        for (int i = 0; i < 32; i++) begin
          if (i < new_buffer_valid && (i + consumed_bytes) < 32) begin
            fetch_buffer[i] <= fetch_buffer[i + consumed_bytes];
            if (i < 6 && $time/10000 < 25) $display("    Shift: buf[%0d] <= buf[%0d] (val=%h)", i, i+consumed_bytes, fetch_buffer[i+consumed_bytes]);
          end else begin
            fetch_buffer[i] <= 8'h00;
          end
        end
        
        // Step 2: Add refilled bytes at the end
        if (mem_ack && $time/10000 < 25) $display("    Refill: mem_rdata=%h from addr=%h", mem_rdata, buffer_pc + buffer_valid);
        for (int i = 0; i < 16; i++) begin
          if (i < refill_amount) begin
            fetch_buffer[new_buffer_valid + i] <= mem_rdata[(15-i)*8 +: 8];
            if (i < 4 && $time/10000 < 25) $display("    Refill: buf[%0d] <= mem_rdata[%0d:%0d] (val=%h)", new_buffer_valid+i, (15-i)*8+7, (15-i)*8, mem_rdata[(15-i)*8 +: 8]);
          end
        end
        
        buffer_valid <= new_buffer_valid + refill_amount;
        buffer_pc <= buffer_pc + {26'h0, consumed_bytes};
        
      end else if (consumed_bytes > 0) begin
        // Case 1: Consume only (no refill)
        for (int i = 0; i < 32; i++) begin
          if (i < new_buffer_valid && (i + consumed_bytes) < 32) begin
            fetch_buffer[i] <= fetch_buffer[i + consumed_bytes];
          end else begin
            fetch_buffer[i] <= 8'h00;
          end
        end
        buffer_valid <= new_buffer_valid;
        buffer_pc <= buffer_pc + {26'h0, consumed_bytes};
        
      end else if (mem_ack) begin
        // Case 2: Refill only (no consumption)
        refill_amount = (buffer_valid >= 6'd32) ? 6'd0 :
                       (buffer_valid + 6'd16 > 6'd32) ? (6'd32 - buffer_valid) : 
                       6'd16;
        
        for (int i = 0; i < 16; i++) begin
          if (i < refill_amount) begin
            fetch_buffer[buffer_valid + i] <= mem_rdata[(15-i)*8 +: 8];
          end
        end
        
        buffer_valid <= buffer_valid + refill_amount;
        // Note: buffer_pc doesn't change on refill-only
      end
      // else: no consume, no refill - buffer unchanged
    end
  end
  
  // ============================================================================
  // Instruction Pre-Decode (Length Detection)
  // ============================================================================
  
  // Extract bytes for first instruction (from byte array)
  logic [7:0] spec_0, op_0;
  logic [7:0] spec_1, op_1;
  
  always_comb begin
    // Extract specifier and opcode for first instruction
    spec_0 = fetch_buffer[0];  // Byte 0 (specifier)
    op_0 = fetch_buffer[1];    // Byte 1 (opcode)
    
    // Calculate first instruction length
    inst_len_0 = get_inst_length(op_0, spec_0);
    
    // Extract second instruction (starts after first)
    // Need at least 2 more bytes after first instruction for spec+op
    if (({2'b0, inst_len_0} + 6'd2) <= buffer_valid && inst_len_0 > 0) begin
      spec_1 = fetch_buffer[inst_len_0];
      op_1 = fetch_buffer[inst_len_0 + 1];
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
    
    // Extract instruction bytes (up to 13 bytes) from byte array
    // inst_data format: bits[103:96]=byte0, bits[95:88]=byte1, etc. (big-endian)
    for (int i = 0; i < 13; i++) begin
      inst_data_0[(12-i)*8 +: 8] = fetch_buffer[i];
    end
    pc_0 = buffer_pc;
    
    // Second instruction (dual-issue)
    // Only valid if first is valid and buffer has enough bytes
    valid_1 = valid_0 && 
              (buffer_valid >= ({2'b0, inst_len_0} + {2'b0, inst_len_1})) && 
              !branch_taken &&
              (inst_len_1 > 0);
    
    // Extract second instruction data (starting at inst_len_0 offset)
    for (int i = 0; i < 13; i++) begin
      if (inst_len_0 + i < 32) begin
        inst_data_1[(12-i)*8 +: 8] = fetch_buffer[inst_len_0 + i];
      end else begin
        inst_data_1[(12-i)*8 +: 8] = 8'h00;
      end
    end
    
    pc_1 = buffer_pc + {28'h0, inst_len_0};
  end
  
  // ============================================================================
  // Memory Request
  // ============================================================================
  
  always_comb begin
    // Request memory when buffer needs refilling
    // Keep buffer topped up to handle dual-issue and long instructions
    mem_req = (buffer_valid < 6'd20) && !stall && !branch_taken;
    // CRITICAL: Fetch from where the buffer ends, not from PC!
    // buffer_pc points to start of buffer, buffer_valid is how many bytes we have
    // So next fetch should be from buffer_pc + buffer_valid
    mem_addr = buffer_pc + {26'h0, buffer_valid};
  end
  
  // ============================================================================
  // PC Update Logic
  // ============================================================================
  
  always_comb begin
    if (branch_taken) begin
      pc_next = branch_target;
    end else if (!stall) begin
      // Sequential execution: advance by number of bytes consumed
      // NOTE: This should match buffer_pc for consistency
      pc_next = buffer_pc;
    end else begin
      pc_next = pc;
    end
  end

endmodule : fetch_unit
