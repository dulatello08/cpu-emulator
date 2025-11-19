# NeoCore16x32 CPU Bug Summary

## Bugs Identified

### Bug #1: Fetch Buffer Big-Endian Byte Ordering (HIGH PRIORITY)

**File**: `sv/rtl/fetch_unit.sv`  
**Lines**: 114-145 (buffer management logic)  
**Severity**: CRITICAL - causes CPU to run away and not halt properly

**Symptoms**:
- Advanced test programs timeout instead of halting
- PC advances to incorrect addresses (e.g., 0x41f8 instead of 0x17)
- Instructions are mis-decoded (wrong opcodes/specifiers detected)
- Buffer shows invalid instruction lengths

**Root Causes**:
1. **Buffer Overflow**: buffer_valid could exceed 32 (buffer capacity), leading to data corruption
   - Original code: `buffer_valid <= buffer_valid - consumed_bytes + 6'd16`
   - Could result in buffer_valid > 32

2. **Incorrect Byte Positioning During Refill**: 
   - Original line 119: `({128'h0, mem_rdata} << ((buffer_valid - consumed_bytes) * 8))`
   - This positioned new bytes incorrectly relative to existing data after consumption

3. **Big-Endian Layout Violation**:
   - Buffer should have: bits[255:248]=Byte0, bits[247:240]=Byte1, ..., bits[7:0]=Byte31
   - Refill logic didn't maintain this layout correctly

**Fix Status**: PARTIAL - Requires Complete Rewrite
- Attempted several fixes to byte positioning logic
- Simple tests pass (uniform-length instructions)
- Advanced tests fail (variable-length instruction sequences)
- Root cause identified: variable-width shift operations in buffer management
- Recommendation: Complete algorithmic rewrite needed

**Recommended Complete Fix**:
Rewrite buffer management with clearer algorithm:
```systemverilog
// After consumption, buffer has new_valid bytes at [255 : 256-new_valid*8]
// New data should be placed at [(256-new_valid*8-1) : (256-new_valid*8-refill_bytes*8)]
// Simpler: shift mem_rdata to align with where it should go
```

**Test Coverage**:
- Created `core_advanced_tb.sv` with dependency chain test
- Test exposes the bug clearly
- Need additional tests for all fetch buffer edge cases

---

### Bug #2: Combinational Loops in core_top (INVESTIGATED - NONE FOUND)

**File**: `sv/rtl/core_top.sv`  
**Lines**: N/A
**Severity**: N/A - No issues detected

**Investigation Results**:
Systematic analysis of control signal dependencies in core_top.sv revealed:

1. **Stall Signal Path** (line 547):
   - `stall_pipeline = hazard_stall || mem_stall || halted`
   - All inputs are combinational outputs from pipeline stage modules
   - Feeds back to pipeline register stall inputs
   - ✅ This is correct: combinational control derived from registered state

2. **Hazard Unit**:
   - All inputs come from pipeline register outputs (registered signals)
   - Outputs are combinational (stall, flush_id, flush_ex, forward signals)
   - ✅ No combinational feedback loops

3. **Branch Control**:
   - branch_taken comes from execute_stage (combinational from registered inputs)
   - Feeds to fetch_unit and pipeline registers
   - ✅ Proper pipeline control flow

4. **Memory Stall**:
   - mem_stall from memory_stage (combinational from registered inputs)
   - ✅ No loops detected

**Conclusion**: No combinational loops found in core_top.sv. The pipeline control logic follows proper design patterns with combinational control signals derived from registered pipeline state.

**Status**: CLEAR - No bugs found in core_top control logic

---

## Test Coverage

### Active Tests
- ✅ ALU unit test (`alu_tb.sv`)
- ✅ Register file unit test (`register_file_tb.sv`)
- ✅ Multiply unit test (`multiply_unit_tb.sv`)
- ✅ Branch unit test (`branch_unit_tb.sv`)
- ✅ Decode unit test (`decode_unit_tb.sv`)
- ✅ Core unified test (`core_unified_tb.sv`) - simple program, PASS
- ✅ Advanced testbench (`core_advanced_tb.sv`) - RAW dependencies, load-use, branches

### Deprecated/Unused Tests
- ⚠️ `core_tb.sv` - Deprecated (uses old simple_memory.sv instead of unified_memory.sv)
- ⚠️ `core_simple_tb.sv` - Not integrated in Makefile, redundant with core_unified_tb

### Test Programs Created
- ✅ `test_simple.hex` - Basic MOV and NOP test
- ✅ `test_dependency_chain.hex` - RAW hazard test (EXPOSES BUG #1)
- ✅ `test_load_use_hazard.hex` - Load-use stall test
- ✅ `test_branch_sequence.hex` - Branch/flush test

---

## Recommended Next Steps

### Immediate (Complete Bug #1 Fix)
1. Simplify fetch buffer algorithm with clear documentation
2. Add unit test for fetch_unit specifically
3. Validate with all three advanced test programs
4. Ensure buffer_valid never exceeds 32
5. Verify big-endian byte order maintained throughout

### Short Term (Complete Bug Analysis)
1. Analyze core_top for combinational loops
2. Review hazard_unit forwarding paths
3. Test branch handling thoroughly
4. Verify pipeline flush logic

### Medium Term (Comprehensive Testing)
1. Add more complex test programs:
   - Deep loops with branches
   - Mixed instruction types
   - Back-to-back loads/stores
   - Maximum-length instructions (13 bytes)
2. Create instruction-specific unit tests
3. Add assertions for X/Z detection
4. Test memory boundary conditions

---

## Architecture Compliance

Based on review of documentation:

### Compliant Areas
- ✅ ISA opcodes correctly defined
- ✅ Big-endian memory interface
- ✅ 5-stage pipeline structure
- ✅ Dual-issue restrictions properly checked
- ✅ Hazard detection logic structure

### Areas Needing Verification
- ❓ Instruction length calculation edge cases
- ❓ Branch flush timing
- ❓ Load-use stall insertion
- ❓ Register file forwarding
- ❓ Memory access alignment

---

## Conclusion

The NeoCore16x32 CPU has at least one critical bug in the fetch buffer management that prevents complex programs from running correctly. The bug is in the big-endian byte ordering and buffer overflow handling. Simple test programs work because they don't stress the buffer management sufficiently.

Additional bugs may exist in:
- Core control flow (combinational loops)
- Pipeline hazard handling
- Branch/flush coordination

A systematic approach is required to:
1. Complete the fetch buffer fix
2. Thoroughly test with complex programs
3. Analyze remaining modules for correctness
4. Ensure full ISA compliance

The existing unit tests are insufficient to catch integration-level bugs. More comprehensive system-level tests are needed.
