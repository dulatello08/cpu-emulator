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

**Fix Status**: PARTIAL
- Added buffer overflow protection
- Corrected some byte positioning issues
- Still has edge cases causing corruption

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

### Bug #2: Potential Combinational Loops in core_top (SUSPECTED)

**File**: `sv/rtl/core_top.sv`  
**Lines**: To be investigated  
**Severity**: HIGH - can cause simulation hangs or X-propagation

**Symptoms**: Not yet observed in tests (simple tests pass)

**Suspected Areas**:
- Stall/ready/valid signal dependencies between:
  - `fetch_unit` ← stall signal
  - `hazard_unit` → stall output
  - `issue_unit` → dual_issue
  - Pipeline registers with stall/flush

**Analysis Needed**:
1. Draw signal dependency graph
2. Identify any combinational feedback paths
3. Ensure all loops broken by registers
4. Check reset completeness

**Status**: NOT YET INVESTIGATED

---

## Test Coverage

### Existing Tests (ALL PASS)
- ✅ ALU unit test
- ✅ Register file unit test  
- ✅ Multiply unit test
- ✅ Branch unit test
- ✅ Decode unit test
- ✅ Core unified test (simple program)

### New Tests Created
- ✅ Advanced testbench (`core_advanced_tb.sv`)
  - RAW dependency chain test (EXPOSES BUG #1)
  - Load-use hazard test
  - Branch sequence test

### Test Programs Created
- ✅ `test_dependency_chain.hex`
- ✅ `test_load_use_hazard.hex`
- ✅ `test_branch_sequence.hex`

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
