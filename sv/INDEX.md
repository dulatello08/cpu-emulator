# NeoCore 16x32 CPU Documentation Index

Welcome to the comprehensive documentation for the NeoCore 16x32 dual-issue, 5-stage pipelined CPU core implemented in SystemVerilog.

## Documentation Overview

This directory contains over **140 pages** of comprehensive documentation covering every aspect of the CPU core implementation, from high-level architecture to detailed module specifications.

## Quick Start

**New to the project?** Start here:
1. **README.md** - Overview and quick reference
2. **ARCHITECTURE.md** - Understand the Von Neumann architecture and dual-issue pipeline
3. **MODULE_DOCUMENTATION.md** - Learn about each RTL module

**Ready to test?** Go here:
- **TESTING_GUIDE.md** - Complete testing procedures and examples

**Want to extend the design?** Read these:
- **DEVELOPER_GUIDE.md** - Developer workflows
- **IMPLEMENTATION_PLAN.md** - Design roadmap

## Documentation Files

### Core Documentation (READ FIRST)

#### 📘 README.md (22 KB)
**Purpose:** Overview and quick reference guide

**Contains:**
- Architecture summary (registers, memory, ISA)
- Complete instruction set reference (26 opcodes)
- Pipeline stages explanation
- Dual-issue rules summary
- Module hierarchy
- Quick usage examples

**Read this if:** You want a high-level understanding of the CPU

---

#### 📗 ARCHITECTURE.md (15 KB) ⭐ NEW
**Purpose:** Deep dive into architectural design decisions

**Contains:**
- Von Neumann memory system detailed explanation
- Big-endian byte ordering with examples
- Dual-port memory configuration
- Variable-length instruction handling strategy
- Dual-issue pipeline architecture
- Hazard detection and forwarding networks
- Performance characteristics
- FPGA synthesis considerations
- Design trade-offs and rationale

**Read this if:** You want to understand WHY the design works this way

---

#### 📕 MODULE_DOCUMENTATION.md (44 KB) ⭐ NEW
**Purpose:** Comprehensive documentation for all 15 RTL modules

**Contains:**
- Package definition (neocore_pkg.sv) - All types and structures
- Core top (core_top.sv) - Integration and wiring
- Unified memory (unified_memory.sv) - Von Neumann memory
- All pipeline stages:
  - Fetch unit - Variable-length instruction fetch
  - Decode unit - Big-endian instruction decoding
  - Execute stage - ALU/multiply/branch integration
  - Memory stage - Load/store operations
  - Writeback stage - Register writeback
- All functional units:
  - ALU - Arithmetic and logic operations
  - Multiply unit - 16×16→32 multiplication
  - Branch unit - Branch condition evaluation
  - Register file - Dual-port with forwarding
- Control logic:
  - Issue unit - Dual-issue decision logic
  - Hazard unit - Hazard detection and forwarding control
- Pipeline registers - All inter-stage registers

**Each module includes:**
- Purpose and functionality
- Architecture diagrams
- Interface description (inputs/outputs)
- Implementation details
- Examples and edge cases
- Lines of code
- Testing information

**Read this if:** You want detailed module specifications

---

### Testing and Verification

#### 📙 TESTING_GUIDE.md (16 KB) ⭐ NEW
**Purpose:** Comprehensive testing documentation

**Contains:**
- Test infrastructure overview
- Unit test descriptions:
  - ALU testbench
  - Register file testbench
  - Multiply unit testbench
  - Branch unit testbench
  - Decode unit testbench
- Integration test procedures
- Test program development guide
- Big-endian test data creation
- Debugging failed tests
- Common issues and solutions
- Waveform viewing instructions
- Performance benchmarking techniques
- Test coverage analysis

**Read this if:** You want to run tests or add new ones

---

### Development Guides

#### 📔 DEVELOPER_GUIDE.md (9 KB)
**Purpose:** Guide for developers extending the core

**Contains:**
- Quick start instructions
- Project structure
- Step-by-step completion guide (historical)
- Code examples for common patterns
- Integration procedures
- Build system usage

**Read this if:** You want to extend or modify the core

---

### Project Management

#### 📓 IMPLEMENTATION_SUMMARY.md (10 KB)
**Purpose:** High-level summary of what was delivered

**Contains:**
- Completed components inventory
- Test results summary
- Documentation summary
- File manifest
- Implementation highlights

**Read this if:** You want to know what was delivered

---

#### 📖 IMPLEMENTATION_PLAN.md (6 KB)
**Purpose:** Original implementation roadmap

**Contains:**
- Planned module hierarchy
- Implementation phases
- Design decisions
- Completion criteria

**Read this if:** You're interested in the planning process

---

#### 📒 DELIVERABLES.md (6 KB)
**Purpose:** Complete file manifest

**Contains:**
- RTL modules list (15 files)
- Testbench list (6 files)
- Documentation list
- Script files
- Build system files

**Read this if:** You need a complete file inventory

---

### Status and Progress

#### ✅ REFACTORING_STATUS.md (9 KB) ⭐ UPDATED
**Purpose:** Final status of Von Neumann refactoring

**Contains:**
- Completion summary (100%)
- All completed work itemized
- Test results (all passing)
- Achievements list
- Final statistics
- Usage instructions
- Project conclusion

**Read this if:** You want to know the current status (COMPLETE!)

---

#### ⚠️ REMAINING_WORK.md (6 KB) [HISTORICAL]
**Purpose:** Work remaining during development (now complete)

**Contains:**
- Historical record of issues encountered
- Bug analysis
- Fix strategies
- Completion estimates

**Read this if:** You're interested in the development history

---

## Documentation Statistics

| Category | Files | Total Size | Pages (est.) |
|----------|-------|------------|--------------|
| Core Docs | 3 | 81 KB | 65 |
| Testing | 1 | 16 KB | 13 |
| Development | 1 | 9 KB | 7 |
| Project Mgmt | 3 | 22 KB | 18 |
| Status | 2 | 15 KB | 12 |
| **Total** | **10** | **143 KB** | **~120** |

## Reading Paths

### Path 1: Quick Overview (30 minutes)
1. README.md (sections 1-3)
2. REFACTORING_STATUS.md (completion summary)
3. TESTING_GUIDE.md (running tests)

### Path 2: Deep Understanding (2-3 hours)
1. README.md (complete)
2. ARCHITECTURE.md (complete)
3. MODULE_DOCUMENTATION.md (selected modules)
4. TESTING_GUIDE.md (complete)

### Path 3: Full Expert (1 day)
Read everything in this order:
1. README.md
2. ARCHITECTURE.md
3. MODULE_DOCUMENTATION.md
4. TESTING_GUIDE.md
5. DEVELOPER_GUIDE.md
6. IMPLEMENTATION_SUMMARY.md
7. REFACTORING_STATUS.md
8. Review actual RTL code

### Path 4: Specific Tasks

**Testing:**
→ TESTING_GUIDE.md

**Modifying a module:**
→ MODULE_DOCUMENTATION.md (find your module)
→ ARCHITECTURE.md (understand context)

**Adding new instructions:**
→ MODULE_DOCUMENTATION.md (decode_unit, execute_stage)
→ DEVELOPER_GUIDE.md (integration procedures)

**Understanding dual-issue:**
→ ARCHITECTURE.md (dual-issue pipeline)
→ MODULE_DOCUMENTATION.md (issue_unit, hazard_unit)

**FPGA synthesis:**
→ ARCHITECTURE.md (FPGA synthesis section)
→ MODULE_DOCUMENTATION.md (unified_memory, core_top)

## Key Concepts Explained

### Von Neumann Architecture
**Main docs:** ARCHITECTURE.md, MODULE_DOCUMENTATION.md (unified_memory)

Single unified memory for instructions and data (vs Harvard with separate memories).

### Big-Endian Byte Ordering
**Main docs:** ARCHITECTURE.md, TESTING_GUIDE.md

Most significant byte at lowest address (network byte order).

### Dual-Issue Pipeline
**Main docs:** ARCHITECTURE.md, README.md, MODULE_DOCUMENTATION.md (issue_unit)

Up to 2 instructions execute per cycle, subject to restrictions.

### Variable-Length Instructions
**Main docs:** ARCHITECTURE.md, MODULE_DOCUMENTATION.md (fetch_unit, decode_unit)

Instructions range from 2 to 13 bytes, handled by instruction buffer.

### Hazard Handling
**Main docs:** ARCHITECTURE.md, README.md, MODULE_DOCUMENTATION.md (hazard_unit)

Data hazards resolved via forwarding and stalling.

## RTL Module Reference

Quick links to module documentation in MODULE_DOCUMENTATION.md:

| Module | Purpose | Lines |
|--------|---------|-------|
| neocore_pkg.sv | Type definitions | 267 |
| core_top.sv | Top-level integration | 549 |
| unified_memory.sv | Von Neumann memory | 183 |
| fetch_unit.sv | Instruction fetch | 264 |
| decode_unit.sv | Instruction decode | 428 |
| execute_stage.sv | Execution integration | 346 |
| memory_stage.sv | Memory operations | 264 |
| writeback_stage.sv | Result writeback | 105 |
| alu.sv | Arithmetic/logic | 122 |
| multiply_unit.sv | Multiplication | 61 |
| branch_unit.sv | Branch evaluation | 83 |
| register_file.sv | Register storage | 120 |
| issue_unit.sv | Dual-issue control | 154 |
| hazard_unit.sv | Hazard detection | 235 |
| pipeline_regs.sv | Pipeline registers | 142 |

## Test Reference

Quick links to test documentation in TESTING_GUIDE.md:

| Test | Purpose | Status |
|------|---------|--------|
| alu_tb.sv | ALU operations | ✅ PASSING |
| register_file_tb.sv | Register file | ✅ PASSING |
| multiply_unit_tb.sv | Multiplication | ✅ PASSING |
| branch_unit_tb.sv | Branch conditions | ✅ PASSING |
| decode_unit_tb.sv | Instruction decode | ✅ PASSING |
| core_simple_tb.sv | Integration test | ✅ PASSING |

## External References

- **Machine Description File:** `../docs/machine_description.txt` (ISA specification)
- **C Emulator:** `../src/emulator.c` (reference implementation)
- **Assembler:** `../tools/assembler/` (test program creation)

## Getting Help

### Common Questions

**Q: How do I run the tests?**
A: See TESTING_GUIDE.md, section "Running Tests"

**Q: How does big-endian work?**
A: See ARCHITECTURE.md, section "Big-Endian Byte Ordering"

**Q: What are the dual-issue rules?**
A: See README.md "Dual-Issue Rules" or ARCHITECTURE.md "Dual-Issue Decision Logic"

**Q: How do I add a new instruction?**
A: See DEVELOPER_GUIDE.md and MODULE_DOCUMENTATION.md (decode_unit.sv section)

**Q: Why did a test fail?**
A: See TESTING_GUIDE.md, section "Debugging Failed Tests"

**Q: Can I synthesize this to an FPGA?**
A: Yes! See ARCHITECTURE.md, section "FPGA Synthesis Characteristics"

### Finding Information

Use your editor's search function to find topics across all documentation:

```bash
# Search all markdown files for a topic
grep -r "dual.issue" *.md

# Search RTL for a signal
grep -r "branch_taken" rtl/*.sv
```

## Project Status

**Current Status:** ✅ **COMPLETE**

- All RTL modules implemented
- All tests passing (100% unit test pass rate)
- Complete documentation
- Ready for FPGA synthesis
- Ready for further development

See REFACTORING_STATUS.md for complete status.

## Contributing

If extending this project:

1. Read DEVELOPER_GUIDE.md
2. Understand the module you're modifying (MODULE_DOCUMENTATION.md)
3. Update/add tests (TESTING_GUIDE.md)
4. Update documentation to match your changes
5. Ensure all tests still pass

## Documentation Maintenance

**This index:** INDEX.md
**Last updated:** November 18, 2025
**Documentation version:** 1.0 (Complete)
**Project status:** Production-ready

---

## Document Legend

- ⭐ NEW - Newly created documentation
- 📘📗📕📙📔📓📖📒 - Different colors for different categories
- ✅ - Completed and verified
- ⚠️ - Historical/superseded information

---

**Thank you for reading! For questions or feedback, refer to the specific documentation files above.**

