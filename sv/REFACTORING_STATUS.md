# Von Neumann Big-Endian Refactoring - Status

## Overview
Major architectural refactoring to convert NeoCore 16x32 dual-issue core from Harvard to Von Neumann architecture with consistent big-endian semantics.

## Completed Work

### Memory Subsystem ✅
- **unified_memory.sv** - New Von Neumann memory module
  - Single BRAM for instructions and data
  - Big-endian byte ordering throughout
  - 128-bit (16-byte) instruction fetch port
  - 32-bit data access port
  - Supports byte, halfword, word access
  - Synthesizable for FPGA

### Instruction Fetch ✅
- **fetch_unit.sv** - Completely rewritten
  - 256-byte instruction buffer
  - Handles variable-length instructions (2-13 bytes)
  - Big-endian byte extraction
  - Dual-issue capable
  - Proper PC management

### Instruction Decode ✅
- **decode_unit.sv** - Completely rewritten
  - Big-endian byte extraction (byte0 at bits [103:96])
  - All 26 opcodes supported (including ENI/DSI)
  - Correct immediate/address extraction for big-endian
  - All MOV variants properly decoded

### Package Updates ✅
- **neocore_pkg.sv** - Updated
  - inst_data: 72-bit → 104-bit (13 bytes)
  - Added OP_ENI, OP_DSI opcodes
  - Updated get_inst_length() for ENI/DSI

## In-Progress Work

### Core Integration 🔄
- **core_top.sv** - Needs major updates
  - Replace dual memory interfaces (imem/dmem) with unified memory
  - Update fetch unit interface (64-bit → 128-bit)
  - Update decode unit interface (72-bit → 104-bit)
  - Wire all pipeline stages correctly

### Memory Stage 🔄
- **memory_stage.sv** - Needs updates
  - Remove dual-port arbitration (now handled by unified_memory)
  - Update interface to unified memory
  - Ensure big-endian data access

### Pipeline Registers 🔄
- **pipeline_regs.sv** - Needs updates
  - Update if_id_t usage (72-bit → 104-bit)
  - Verify all other pipeline structures

## Remaining Work

### RTL Modules to Update
1. **execute_stage.sv**
   - Verify no changes needed (mostly data path)
   - Check forwarding logic compatibility

2. **hazard_unit.sv**
   - Verify no interface changes needed
   - Check for any memory-related hazard logic

3. **issue_unit.sv**
   - Verify no changes needed

4. **writeback_stage.sv**
   - Verify no changes needed

5. **branch_unit.sv, alu.sv, multiply_unit.sv, register_file.sv**
   - Should not need changes (internal data path modules)

### Test Infrastructure
1. **Update all testbenches**
   - Create test programs in big-endian format
   - Update memory initialization (big-endian hex files)
   - Update core_tb.sv for unified_memory
   - Update decode_unit_tb.sv for 104-bit interface

2. **Create big-endian test utilities**
   - Update bin2hex.py or create big-endian version
   - Create helper to convert little-endian to big-endian

### Cleanup
1. **Remove obsolete files**
   - simple_memory.sv (replaced by unified_memory.sv)
   - *.sv.old backup files

2. **Update Makefile**
   - Update source file lists
   - Ensure all targets work

### Documentation
1. **README.md** - Major update
   - Document Von Neumann architecture
   - Document big-endian memory model
   - Update instruction fetch description
   - Update memory subsystem description

2. **DEVELOPER_GUIDE.md** - Update
   - New memory model usage
   - Big-endian programming guide
   - Test program creation

## Testing Strategy

### Phase 1: Unit Tests
- [ ] ALU test (should pass unchanged)
- [ ] Register file test (should pass unchanged)
- [ ] Multiply unit test (should pass unchanged)
- [ ] Branch unit test (should pass unchanged)
- [ ] Decode unit test (needs update for big-endian)

### Phase 2: Integration Tests
- [ ] Simple instruction fetch test
- [ ] Big-endian memory read/write test
- [ ] Variable-length instruction test
- [ ] Dual-issue test

### Phase 3: Full Core Tests
- [ ] Simple arithmetic program
- [ ] Branch test program
- [ ] Memory load/store program
- [ ] Subroutine call test (JSR/RTS)

## Known Issues to Address

1. **Fetch unit buffer management**
   - Need to verify shift logic is correct
   - Test with various instruction length combinations

2. **Big-endian test data**
   - All existing test programs are little-endian
   - Need to convert or create new test data

3. **Memory initialization**
   - Unified memory needs different initialization approach
   - Need $readmemh with big-endian data

4. **Interface changes**
   - Many modules have changed interfaces
   - Need systematic update of all connections

## Priority Order

1. ✅ Memory subsystem (unified_memory.sv)
2. ✅ Fetch unit (fetch_unit.sv)
3. ✅ Decode unit (decode_unit.sv)
4. 🔄 Core integration (core_top.sv)
5. 🔄 Memory stage (memory_stage.sv)
6. 🔄 Pipeline registers
7. ⏳ Test updates
8. ⏳ Documentation
9. ⏳ Cleanup

Legend:
- ✅ Complete
- 🔄 In progress
- ⏳ Not started
