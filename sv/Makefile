# NeoCore 16x32 CPU - Makefile
# Build and test SystemVerilog RTL using Icarus Verilog
#
# Prerequisites:
#   - Icarus Verilog (iverilog, vvp)
#   - GTKWave (optional, for waveform viewing)
#
# Quick Start:
#   make check-tools    # Verify required tools are installed
#   make unit-tests     # Run all unit tests
#   make core-tests     # Run core integration tests
#   make all-tests      # Run everything
#   make clean          # Remove build artifacts
#
# For more information, see TESTING_AND_VERIFICATION.md

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

# ============================================================================
# Tool Verification
# ============================================================================

.PHONY: check-tools
check-tools:
	@echo "Checking for required tools..."
	@which $(IVERILOG) > /dev/null || (echo "ERROR: iverilog not found. Install with: sudo apt-get install iverilog" && exit 1)
	@which $(VVP) > /dev/null || (echo "ERROR: vvp not found. Install with: sudo apt-get install iverilog" && exit 1)
	@echo "✓ Icarus Verilog found: $$($(IVERILOG) -V | head -1)"
	@which $(GTKWAVE) > /dev/null && echo "✓ GTKWave found (optional)" || echo "  GTKWave not found (optional, for waveform viewing)"
	@echo "All required tools are available."

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
	@mkdir -p $(BUILD_DIR)
	@echo "Created build directory: $(BUILD_DIR)"

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

# Core Advanced Testbench (complex test scenarios)
core_advanced_tb: $(BUILD_DIR)
	$(IVERILOG) $(IVFLAGS) -s core_advanced_tb \
		-o $(BUILD_DIR)/core_advanced_tb.vvp \
		$(RTL_SRCS) $(TB_DIR)/core_advanced_tb.sv

run_core_advanced_tb: core_advanced_tb
	cd $(BUILD_DIR) && $(VVP) core_advanced_tb.vvp

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
.PHONY: unit-tests
unit-tests: run_alu_tb run_register_file_tb run_multiply_unit_tb run_branch_unit_tb run_decode_unit_tb
	@echo ""
	@echo "========================================"
	@echo "All unit tests completed successfully!"
	@echo "========================================"

# Run core integration tests
.PHONY: core-tests
core-tests: run_core_unified_tb
	@echo ""
	@echo "========================================"
	@echo "Core integration tests passed!"
	@echo "========================================"

# Run advanced/stress tests
.PHONY: advanced-tests
advanced-tests: run_core_advanced_tb
	@echo ""
	@echo "========================================"
	@echo "Advanced tests completed!"
	@echo "========================================"

# Run all tests
.PHONY: all-tests
all-tests: unit-tests core-tests
	@echo ""
	@echo "========================================"
	@echo "ALL TESTS PASSED!"
	@echo "========================================"

# Run all tests including experimental/long-running tests
.PHONY: all-tests-full
all-tests-full: unit-tests core-tests advanced-tests
	@echo ""
	@echo "========================================"
	@echo "FULL TEST SUITE PASSED!"
	@echo "========================================"

# Default target
.PHONY: default
default: check-tools
	@echo "NeoCore16x32 CPU Build System"
	@echo "============================="
	@echo ""
	@echo "Available targets:"
	@echo "  make check-tools     - Verify required tools are installed"
	@echo "  make unit-tests      - Run all unit tests (ALU, registers, etc.)"
	@echo "  make core-tests      - Run core integration tests"
	@echo "  make all-tests       - Run all standard tests"
	@echo "  make all-tests-full  - Run all tests including advanced tests"
	@echo "  make clean           - Remove build artifacts"
	@echo ""
	@echo "Individual unit tests:"
	@echo "  make alu_test        - ALU testbench"
	@echo "  make mul_test        - Multiply unit testbench"
	@echo "  make decode_test     - Decode unit testbench"
	@echo "  make branch_test     - Branch unit testbench"
	@echo "  make regfile_test    - Register file testbench"
	@echo ""
	@echo "Integration tests:"
	@echo "  make sim             - Run core unified testbench"
	@echo ""
	@echo "Waveform viewing:"
	@echo "  make wave            - View core unified test waveforms"
	@echo "  make wave_alu        - View ALU test waveforms"
	@echo ""
	@echo "For more information, see TESTING_AND_VERIFICATION.md"

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

.PHONY: default check-tools clean \
        unit-tests core-tests advanced-tests all-tests all-tests-full \
        alu_test mul_test decode_test branch_test regfile_test sim \
        wave wave_alu \
        alu_tb run_alu_tb register_file_tb run_register_file_tb \
        multiply_unit_tb run_multiply_unit_tb branch_unit_tb run_branch_unit_tb \
        decode_unit_tb run_decode_unit_tb core_unified_tb run_core_unified_tb \
        core_advanced_tb run_core_advanced_tb
