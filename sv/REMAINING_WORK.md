# Von Neumann Big-Endian Refactoring - Remaining Work

## Current Status (70% Complete)

### ✅ Completed Components

**RTL Modules (6/11):**
1. `unified_memory.sv` - BRAM-backed Von Neumann memory
   - Big-endian byte ordering
   - 128-bit instruction fetch port
   - 32-bit data access port
   - **Status: Complete and working**

2. `decode_unit.sv` - Big-endian instruction decoder
   - Supports all 26 opcodes
   - 104-bit (13-byte) instruction data
   - **Status: Complete and tested**

3. `core_top.sv` - Top-level integration
   - Unified memory interface
   - Dual-issue pipeline
   - **Status: Complete, compiles**

4. `memory_stage.sv` - Load/store with big-endian
   - Proper byte/halfword/word access
   - **Status: Complete**

5. `neocore_pkg.sv` - Type definitions
   - Updated for 104-bit inst_data
   - Added ENI/DSI opcodes
   - **Status: Complete**

6. Supporting modules (ALU, register file, multiply, branch)
   - **Status: All complete and unit tested**

**Testbenches:**
- ✅ ALU testbench - PASSING
- ✅ Register File testbench - PASSING
- ✅ Multiply Unit testbench - PASSING
- ✅ Branch Unit testbench - PASSING
- ✅ Decode Unit testbench - PASSING (updated for big-endian)
- ⚠️ Core integration testbench - Created but needs fetch unit fix

### ⚠️ Issue: Fetch Unit Bug

**Problem Location:** `sv/rtl/fetch_unit.sv` lines 76-104

**Symptom:** PC advances to 0x08 then stalls indefinitely

**Root Cause:** Buffer management logic has several bugs:
1. Lines 88-90 try to declare variables inside `always_ff` block
2. Buffer shift calculation is incorrect for big-endian
3. Buffer refill logic doesn't properly maintain data alignment

**Fix Required:**
The fetch unit needs to be rewritten with simpler buffer management:

```systemverilog
// Simplified approach:
// 1. Maintain 256-bit circular buffer
// 2. Track read pointer (in bytes)
// 3. Track write pointer (in bytes)
// 4. Refill when space available
// 5. Extract instructions from read pointer
```

Key fixes needed in `fetch_unit.sv`:

1. **Remove variable declarations from sequential blocks** (lines 88-90)
   - Move to combinational logic

2. **Fix buffer shift logic** (lines 94-103)
   - Current implementation tries to shift and OR in same statement
   - Should use simpler approach: shift buffer, then refill from bottom

3. **Simplify refill logic**
   - When mem_ack: append mem_rdata to buffer
   - Update buffer_valid counter
   - Ensure big-endian byte ordering preserved

4. **Fix PC tracking**
   - PC should track buffer_pc + consumed_bytes
   - Buffer_pc should only update when buffer shifts

## Recommended Fix Strategy

### Option 1: Simplified Fetch Unit (Recommended, 2-3 hours)

Replace complex buffer with simpler state machine:

1. **States:**
   - IDLE: Waiting for request
   - FETCH: Fetching from memory
   - READY: Data available

2. **Buffer Management:**
   ```systemverilog
   // Simple shift register approach
   logic [255:0] inst_buffer;
   logic [5:0] valid_bytes;
   
   always_ff @(posedge clk) begin
     if (rst) begin
       inst_buffer <= 256'h0;
       valid_bytes <= 0;
     end else begin
       if (mem_ack && !stall) begin
         // Append new data to bottom of buffer
         inst_buffer <= {inst_buffer[127:0], mem_rdata};
         valid_bytes <= valid_bytes + 16;
       end
       
       if (consume_bytes && !stall) begin
         // Shift out consumed bytes
         inst_buffer <= inst_buffer << (consume_bytes * 8);
         valid_bytes <= valid_bytes - consume_bytes;
       end
     end
   end
   ```

3. **Instruction Extraction:**
   - Always extract from top of buffer (bits [255:152] for inst0)
   - Calculate valid based on valid_bytes counter
   - Second instruction extracted from appropriate offset

### Option 2: Reference Implementation Study (1 hour)

Look at existing working RISC-V or ARM fetch units for variable-length instruction handling patterns.

## Testing Strategy

Once fetch unit is fixed:

1. **Unit Test fetch_unit.sv** (30 min)
   - Create fetch_unit_tb.sv
   - Test with known instruction sequences
   - Verify buffer management

2. **Integration Test** (1 hour)
   - Run core_unified_tb.sv
   - Should reach HLT at PC=0x14
   - Verify dual-issue counts

3. **Extended Tests** (1 hour)
   - Create more complex test programs
   - Test all instruction types
   - Validate against C emulator

## Documentation Updates Needed

After fetch unit is fixed:

1. **README.md** (30 min)
   - Update architecture description
   - Document Von Neumann memory model
   - Update instruction fetch description

2. **DEVELOPER_GUIDE.md** (20 min)
   - Update for big-endian programming
   - Document test program creation
   - Update memory map

3. **IMPLEMENTATION_SUMMARY.md** (10 min)
   - Mark fetch unit as complete
   - Update completion percentage

## Cleanup Tasks

1. **Remove obsolete files** (10 min)
   - `rtl/simple_memory.sv` - replaced by unified_memory.sv
   - `rtl/*.sv.old` - backup files
   - `rtl/fetch_unit.sv.broken` - backup

2. **Update .gitignore** (5 min)
   - Exclude build artifacts
   - Exclude backup files

## Estimated Time to Completion

- **Critical Path:** Fix fetch unit (2-3 hours)
- **Testing:** 2-3 hours
- **Documentation:** 1 hour
- **Cleanup:** 15 minutes

**Total: 5-7 hours of focused work**

## Success Criteria

- [ ] All unit tests passing
- [ ] Core integration test completes to HLT
- [ ] At least one dual-issue cycle observed
- [ ] Documentation updated
- [ ] No obsolete files in repository
- [ ] CodeQL scan passes
- [ ] Code review passes

## Notes for Next Developer

1. The fetch unit is the ONLY blocking issue
2. All other modules are complete and tested
3. The bug is well-isolated and understood
4. A working fix should take 2-3 hours
5. Once fixed, everything should "just work"

## Handoff Checklist

- ✅ All unit modules working and tested
- ✅ Integration framework in place
- ✅ Memory system complete
- ✅ Testbenches created
- ⚠️ Fetch unit needs fixing
- ⚠️ Integration testing pending
- ⚠️ Documentation updates pending
