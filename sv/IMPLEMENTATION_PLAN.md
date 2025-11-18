# Von Neumann Big-Endian Refactoring - Implementation Plan

## Current Status
We have completed approximately 30% of the refactoring work:

### ✅ Completed (3 major modules)
1. **unified_memory.sv** - New BRAM-backed Von Neumann memory
2. **fetch_unit.sv** - Rewritten for big-endian, 128-bit fetch, 256-byte buffer  
3. **decode_unit.sv** - Rewritten for big-endian byte extraction

### 🔄 Partially Complete
1. **neocore_pkg.sv** - Updated inst_data width and opcodes

## Remaining Work (Estimated 6-8 hours)

### Phase 1: Core Structure Updates (2-3 hours)

#### 1.1 Update core_top.sv
**Current state:** Has dual memory interfaces (imem_addr/dmem_addr separate)
**Target state:** Single unified memory interface

**Changes needed:**
```systemverilog
// OLD (current):
output logic [31:0] imem_addr,
output logic        imem_req,
input  logic [63:0] imem_rdata,
output logic [31:0] dmem_addr,
... separate ports ...

// NEW (target):
output logic [31:0] mem_if_addr,
output logic        mem_if_req,
input  logic [127:0] mem_if_rdata,  // 128-bit for instruction fetch
output logic [31:0] mem_data_addr,
output logic [31:0] mem_data_wdata,
output logic [1:0]  mem_data_size,
... unified memory interface ...
```

**Internal changes:**
- Update fetch_unit instantiation (64-bit → 128-bit interface)
- Update decode_unit instantiation (72-bit → 104-bit interface)  
- Update all if_id_t pipeline register uses
- Verify all signal connections

#### 1.2 Update memory_stage.sv
**Changes needed:**
- Remove internal arbitration logic (unified_memory handles dual-port)
- Update to use unified memory interface
- Ensure big-endian data packing/unpacking
- Simplify state machine (no longer needs complex arbitration)

#### 1.3 Update pipeline_regs.sv
**Changes needed:**
- Already uses if_id_t typedef, which was updated
- Should compile but needs verification
- Test with new inst_data width

### Phase 2: Test Infrastructure (2-3 hours)

#### 2.1 Create big-endian test utilities
**New files needed:**
```
sv/scripts/le_to_be.py      # Convert little-endian hex to big-endian
sv/mem/test_be_simple.hex   # Big-endian version of test programs
```

#### 2.2 Update testbenches
**Files to update:**
- `decode_unit_tb.sv` - Change inst_data from 72-bit to 104-bit, use big-endian
- `core_tb.sv` - Use unified_memory instead of simple_memory
- All test programs must be converted to big-endian format

#### 2.3 Create new tests
- Big-endian memory access test
- Variable-length instruction test (up to 13 bytes)
- Instruction buffer management test

### Phase 3: Integration & Debug (2-3 hours)

#### 3.1 Compilation
- Fix all syntax errors
- Resolve interface mismatches
- Update Makefile source lists

#### 3.2 Unit tests
- Run ALU test (should pass)
- Run register file test (should pass)
- Run multiply test (should pass)
- Run branch test (should pass)
- Run decode test (update & run)

#### 3.3 Integration tests
- Simple fetch test
- Memory read/write test
- Full core smoke test

#### 3.4 Debug
- Fix buffer management issues in fetch_unit
- Fix any big-endian byte swapping issues
- Fix pipeline flow issues

### Phase 4: Cleanup & Documentation (1 hour)

#### 4.1 Remove obsolete files
- simple_memory.sv
- *.sv.old backup files
- Old test files

#### 4.2 Update documentation
- README.md - Big-endian memory model, Von Neumann architecture
- DEVELOPER_GUIDE.md - How to create big-endian programs
- Update build instructions

#### 4.3 Makefile
- Update RTL_SRCS list
- Remove simple_memory.sv
- Add unified_memory.sv
- Ensure all targets work

## Files Requiring Updates

### RTL Files (High Priority)
1. ✅ unified_memory.sv - DONE (new file)
2. ✅ fetch_unit.sv - DONE (rewritten)
3. ✅ decode_unit.sv - DONE (rewritten)
4. ✅ neocore_pkg.sv - DONE (updated)
5. 🔄 core_top.sv - IN PROGRESS (needs major update)
6. 🔄 memory_stage.sv - IN PROGRESS (needs update)
7. ⏳ pipeline_regs.sv - Should work, needs verification
8. ⏳ execute_stage.sv - Might need minor updates
9. ⏳ hazard_unit.sv - Probably no changes needed
10. ⏳ issue_unit.sv - Probably no changes needed
11. ⏳ writeback_stage.sv - Probably no changes needed

### Testbench Files
1. ⏳ decode_unit_tb.sv - Needs update for 104-bit interface
2. ⏳ core_tb.sv - Needs unified_memory usage
3. ⏳ Other unit tests - May need big-endian test data

### Build & Utility Files
1. ⏳ Makefile - Update source lists
2. ⏳ scripts/le_to_be.py - NEW (convert endianness)
3. ⏳ mem/test_*.hex - Convert to big-endian

### Documentation Files
1. ⏳ README.md - Major update
2. ⏳ DEVELOPER_GUIDE.md - Update
3. ✅ REFACTORING_STATUS.md - NEW (tracking)
4. ✅ IMPLEMENTATION_PLAN.md - NEW (this file)

## Critical Path

The following must be done in order:

1. ✅ Create unified_memory.sv
2. ✅ Update fetch_unit.sv
3. ✅ Update decode_unit.sv  
4. ✅ Update neocore_pkg.sv
5. 🔄 Update core_top.sv → **NEXT PRIORITY**
6. 🔄 Update memory_stage.sv
7. ⏳ Create big-endian test utilities
8. ⏳ Update testbenches
9. ⏳ Compile and fix errors
10. ⏳ Run and debug tests
11. ⏳ Update documentation
12. ⏳ Final cleanup

## Testing Strategy

### Incremental Testing
After each major module update:
1. Try to compile (catch syntax errors early)
2. Fix compilation errors
3. Update one testbench
4. Run that test
5. Fix runtime issues
6. Move to next module

### Don't try to compile everything at once
- Too many errors to debug simultaneously
- Update modules incrementally
- Test incrementally

## Estimated Completion Time

- **Optimistic:** 6 hours (if no major issues)
- **Realistic:** 8 hours (with debugging)
- **Pessimistic:** 10-12 hours (if significant issues found)

## Next Steps

Immediate priorities:
1. Finish core_top.sv update
2. Update memory_stage.sv
3. Create le_to_be.py utility
4. Update one testbench and try to run it
5. Iterate based on findings

## Notes

- This is a fundamental architectural change
- Affects nearly every module
- Requires careful testing at each step
- Big-endian conversion is critical
- Documentation must be comprehensive
- No shortcuts - do it right

---

**Last Updated:** During initial refactoring phase
**Completion:** ~30% (3/11 major modules done)
