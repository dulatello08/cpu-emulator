# NeoCore 16x32 CPU - Makefile
# Build and test SystemVerilog RTL using Icarus Verilog

# Directories
RTL_DIR = rtl
TB_DIR = tb
MEM_DIR = mem
BUILD_DIR = build

# Tools
IVERILOG = iverilog
VVP = vvp
GTKWAVE = gtkwave

# Compiler flags
IVFLAGS = -g2012 -Wall -Winfloop
IVFLAGS += -I$(RTL_DIR)

# Source files
PKG_SRC = $(RTL_DIR)/neocore_pkg.sv

RTL_SRCS = \
	$(PKG_SRC) \
	$(RTL_DIR)/alu.sv \
	$(RTL_DIR)/multiply_unit.sv \
	$(RTL_DIR)/branch_unit.sv \
	$(RTL_DIR)/register_file.sv \
	$(RTL_DIR)/decode_unit.sv \
	$(RTL_DIR)/fetch_unit.sv

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ============================================================================
# Unit Tests
# ============================================================================

# ALU Testbench
alu_tb: $(BUILD_DIR)
	$(IVERILOG) $(IVFLAGS) -s alu_tb \
		-o $(BUILD_DIR)/alu_tb.vvp \
		$(PKG_SRC) $(RTL_DIR)/alu.sv $(TB_DIR)/alu_tb.sv

run_alu_tb: alu_tb
	cd $(BUILD_DIR) && $(VVP) alu_tb.vvp

# Register File Testbench
register_file_tb: $(BUILD_DIR)
	$(IVERILOG) $(IVFLAGS) -s register_file_tb \
		-o $(BUILD_DIR)/register_file_tb.vvp \
		$(PKG_SRC) $(RTL_DIR)/register_file.sv $(TB_DIR)/register_file_tb.sv

run_register_file_tb: register_file_tb
	cd $(BUILD_DIR) && $(VVP) register_file_tb.vvp

# Multiply Unit Testbench
multiply_unit_tb: $(BUILD_DIR)
	$(IVERILOG) $(IVFLAGS) -s multiply_unit_tb \
		-o $(BUILD_DIR)/multiply_unit_tb.vvp \
		$(PKG_SRC) $(RTL_DIR)/multiply_unit.sv $(TB_DIR)/multiply_unit_tb.sv

run_multiply_unit_tb: multiply_unit_tb
	cd $(BUILD_DIR) && $(VVP) multiply_unit_tb.vvp

# Branch Unit Testbench
branch_unit_tb: $(BUILD_DIR)
	$(IVERILOG) $(IVFLAGS) -s branch_unit_tb \
		-o $(BUILD_DIR)/branch_unit_tb.vvp \
		$(PKG_SRC) $(RTL_DIR)/branch_unit.sv $(TB_DIR)/branch_unit_tb.sv

run_branch_unit_tb: branch_unit_tb
	cd $(BUILD_DIR) && $(VVP) branch_unit_tb.vvp

# Decode Unit Testbench
decode_unit_tb: $(BUILD_DIR)
	$(IVERILOG) $(IVFLAGS) -s decode_unit_tb \
		-o $(BUILD_DIR)/decode_unit_tb.vvp \
		$(PKG_SRC) $(RTL_DIR)/decode_unit.sv $(TB_DIR)/decode_unit_tb.sv

run_decode_unit_tb: decode_unit_tb
	cd $(BUILD_DIR) && $(VVP) decode_unit_tb.vvp

# ============================================================================
# Run all tests
# ============================================================================

all: run_alu_tb run_register_file_tb run_multiply_unit_tb run_branch_unit_tb run_decode_unit_tb

# ============================================================================
# Clean
# ============================================================================

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean alu_tb run_alu_tb register_file_tb run_register_file_tb \
        multiply_unit_tb run_multiply_unit_tb branch_unit_tb run_branch_unit_tb \
        decode_unit_tb run_decode_unit_tb
