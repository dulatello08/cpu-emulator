//
// unified_memory.sv
// NeoCore 16x32 CPU - Unified Von Neumann Memory (BRAM-backed)
//
// Single unified memory for both instructions and data.
// Big-endian byte ordering throughout.
// Synthesizable for ULX3S 85F (Lattice ECP5-85F FPGA).
//
// Memory Organization (Big-Endian):
//   Address 0x00: MSB of first word
//   Address 0x01: next byte
//   Address 0x02: next byte
//   Address 0x03: LSB of first word
//
// Supports:
//   - Wide instruction fetches (128 bits = 16 bytes per cycle)
//   - Granular data access (byte, halfword, word)
//   - Dual-port access (instruction fetch + data access)
//

module unified_memory #(
  parameter MEM_SIZE_BYTES = 65536,  // 64 KB
  parameter ADDR_WIDTH = 32
)(
  input  logic        clk,
  input  logic        rst,
  
  // Instruction fetch port (wide fetch for variable-length instructions)
  input  logic [ADDR_WIDTH-1:0] if_addr,
  input  logic                  if_req,
  output logic [127:0]          if_rdata,  // 16 bytes (enough for 13-byte instruction + alignment)
  output logic                  if_ack,
  
  // Data access port (load/store)
  input  logic [ADDR_WIDTH-1:0] data_addr,
  input  logic [31:0]           data_wdata,
  input  logic [1:0]            data_size,  // 00=byte, 01=halfword, 10=word
  input  logic                  data_we,
  input  logic                  data_req,
  output logic [31:0]           data_rdata,
  output logic                  data_ack
);

  // ============================================================================
  // Memory Storage
  // ============================================================================
  
  // Byte-addressable memory array
  // Organized for FPGA block RAM inference
  logic [7:0] mem [0:MEM_SIZE_BYTES-1];
  
  // ============================================================================
  // Instruction Fetch Port (Port A)
  // ============================================================================
  
  logic [127:0] if_rdata_int;
  
  always_ff @(posedge clk) begin
    if (rst) begin
      if_rdata <= 128'h0;
      if_ack <= 1'b0;
    end else if (if_req) begin
      // Fetch 16 bytes starting at if_addr (big-endian)
      // This provides enough bandwidth for any instruction up to 13 bytes
      // plus alignment slack
      if (if_addr < (MEM_SIZE_BYTES - 16)) begin
        // Big-endian: first byte at if_addr goes to MSB position
        if_rdata <= {
          mem[if_addr + 0],   // Byte 0 -> bits [127:120]
          mem[if_addr + 1],   // Byte 1 -> bits [119:112]
          mem[if_addr + 2],   // Byte 2 -> bits [111:104]
          mem[if_addr + 3],   // Byte 3 -> bits [103:96]
          mem[if_addr + 4],   // Byte 4 -> bits [95:88]
          mem[if_addr + 5],   // Byte 5 -> bits [87:80]
          mem[if_addr + 6],   // Byte 6 -> bits [79:72]
          mem[if_addr + 7],   // Byte 7 -> bits [71:64]
          mem[if_addr + 8],   // Byte 8 -> bits [63:56]
          mem[if_addr + 9],   // Byte 9 -> bits [55:48]
          mem[if_addr + 10],  // Byte 10 -> bits [47:40]
          mem[if_addr + 11],  // Byte 11 -> bits [39:32]
          mem[if_addr + 12],  // Byte 12 -> bits [31:24]
          mem[if_addr + 13],  // Byte 13 -> bits [23:16]
          mem[if_addr + 14],  // Byte 14 -> bits [15:8]
          mem[if_addr + 15]   // Byte 15 -> bits [7:0]
        };
        if_ack <= 1'b1;
      end else begin
        // Out of bounds - return zeros
        if_rdata <= 128'h0;
        if_ack <= 1'b0;
      end
    end else begin
      if_ack <= 1'b0;
    end
  end
  
  // ============================================================================
  // Data Access Port (Port B)
  // ============================================================================
  
  always_ff @(posedge clk) begin
    if (rst) begin
      data_rdata <= 32'h0;
      data_ack <= 1'b0;
    end else if (data_req) begin
      if (data_we) begin
        // Write operation (big-endian)
        case (data_size)
          2'b00: begin  // Byte write
            if (data_addr < MEM_SIZE_BYTES) begin
              mem[data_addr] <= data_wdata[7:0];
            end
          end
          
          2'b01: begin  // Halfword (16-bit) write - big-endian
            if (data_addr < (MEM_SIZE_BYTES - 1)) begin
              mem[data_addr + 0] <= data_wdata[15:8];  // MSB first
              mem[data_addr + 1] <= data_wdata[7:0];   // LSB second
            end
          end
          
          2'b10: begin  // Word (32-bit) write - big-endian
            if (data_addr < (MEM_SIZE_BYTES - 3)) begin
              mem[data_addr + 0] <= data_wdata[31:24]; // MSB first
              mem[data_addr + 1] <= data_wdata[23:16];
              mem[data_addr + 2] <= data_wdata[15:8];
              mem[data_addr + 3] <= data_wdata[7:0];   // LSB last
            end
          end
          
          default: begin
            // Invalid size - no operation
          end
        endcase
        data_ack <= 1'b1;
      end else begin
        // Read operation (big-endian)
        case (data_size)
          2'b00: begin  // Byte read
            if (data_addr < MEM_SIZE_BYTES) begin
              data_rdata <= {24'h0, mem[data_addr]};
            end else begin
              data_rdata <= 32'h0;
            end
          end
          
          2'b01: begin  // Halfword (16-bit) read - big-endian
            if (data_addr < (MEM_SIZE_BYTES - 1)) begin
              data_rdata <= {
                16'h0,
                mem[data_addr + 0],  // MSB
                mem[data_addr + 1]   // LSB
              };
            end else begin
              data_rdata <= 32'h0;
            end
          end
          
          2'b10: begin  // Word (32-bit) read - big-endian
            if (data_addr < (MEM_SIZE_BYTES - 3)) begin
              data_rdata <= {
                mem[data_addr + 0],  // MSB
                mem[data_addr + 1],
                mem[data_addr + 2],
                mem[data_addr + 3]   // LSB
              };
            end else begin
              data_rdata <= 32'h0;
            end
          end
          
          default: begin
            data_rdata <= 32'h0;
          end
        endcase
        data_ack <= 1'b1;
      end
    end else begin
      data_ack <= 1'b0;
    end
  end

endmodule : unified_memory
