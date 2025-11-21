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
	$(RTL_DIR)/fetch_unit.sv \
	$(RTL_DIR)/pipeline_regs.sv \
	$(RTL_DIR)/hazard_unit.sv \
	$(RTL_DIR)/issue_unit.sv \
	$(RTL_DIR)/execute_stage.sv \
	$(RTL_DIR)/memory_stage.sv \
	$(RTL_DIR)/writeback_stage.sv \
	$(RTL_DIR)/unified_memory.sv \
	$(RTL_DIR)/core_top.sv

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

# Core Testbench (unified memory)
core_unified_tb: $(BUILD_DIR)
	$(IVERILOG) $(IVFLAGS) -s core_unified_tb \
		-o $(BUILD_DIR)/core_unified_tb.vvp \
		$(RTL_SRCS) $(TB_DIR)/core_unified_tb.sv

run_core_unified_tb: core_unified_tb
	cd $(BUILD_DIR) && $(VVP) core_unified_tb.vvp

# Core Testbench (old, with simple_memory - deprecated)
core_tb: $(BUILD_DIR)
	$(IVERILOG) $(IVFLAGS) -s core_tb \
		-o $(BUILD_DIR)/core_tb.vvp \
		$(PKG_SRC) \
		$(RTL_DIR)/alu.sv \
		$(RTL_DIR)/multiply_unit.sv \
		$(RTL_DIR)/branch_unit.sv \
		$(RTL_DIR)/register_file.sv \
		$(RTL_DIR)/decode_unit.sv \
		$(RTL_DIR)/fetch_unit.sv \
		$(RTL_DIR)/pipeline_regs.sv \
		$(RTL_DIR)/hazard_unit.sv \
		$(RTL_DIR)/issue_unit.sv \
		$(RTL_DIR)/execute_stage.sv \
		$(RTL_DIR)/memory_stage.sv \
		$(RTL_DIR)/writeback_stage.sv \
		$(RTL_DIR)/simple_memory.sv \
		$(RTL_DIR)/core_top.sv \
		$(TB_DIR)/core_tb.sv

run_core_tb: core_tb
	cd $(BUILD_DIR) && $(VVP) core_tb.vvp

# ============================================================================
# Run all tests
# ============================================================================

# Individual test runners with VCD
alu_test: run_alu_tb
mul_test: run_multiply_unit_tb  
decode_test: run_decode_unit_tb
branch_test: run_branch_unit_tb
regfile_test: run_register_file_tb
sim: run_core_unified_tb

# Run all unit tests
all_tests: run_alu_tb run_register_file_tb run_multiply_unit_tb run_branch_unit_tb run_decode_unit_tb

# Default target runs the unified core test
default: sim

# View waveforms with GTKWave
wave: $(BUILD_DIR)/core_unified_tb.vcd
	$(GTKWAVE) $(BUILD_DIR)/core_unified_tb.vcd &

wave_alu: $(BUILD_DIR)/alu_tb.vcd
	$(GTKWAVE) $(BUILD_DIR)/alu_tb.vcd &

# ============================================================================
# Clean
# ============================================================================

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all default sim clean alu_test mul_test decode_test branch_test regfile_test all_tests \
        wave wave_alu \
        alu_tb run_alu_tb register_file_tb run_register_file_tb \
        multiply_unit_tb run_multiply_unit_tb branch_unit_tb run_branch_unit_tb \
        decode_unit_tb run_decode_unit_tb core_unified_tb run_core_unified_tb
