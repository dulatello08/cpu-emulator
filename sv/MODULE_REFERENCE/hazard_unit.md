# Hazard Unit Module Reference

## Overview
The Hazard Unit detects data hazards and structural hazards in the pipeline, generating stall signals to prevent incorrect execution.

## Module: `hazard_unit`

### Ports

Inputs from ID/EX, EX/MEM, and MEM/WB stages:
- Register addresses (source and destination)
- Valid flags
- Instruction types

Outputs:
- `hazard_stall`: Pipeline stall signal
- Forwarding control signals (if implemented)

### Hazard Types Detected

1. **Load-Use Hazard**: Instruction in EX is a load, instruction in ID needs the loaded value
2. **RAW (Read-After-Write)**: Instruction reads register that previous instruction writes
3. **Structural Hazard**: Resource conflicts (handled mainly by issue_unit)

### Stall Logic

The hazard unit generates a stall when:
- Load instruction in EX/MEM stage
- Following instruction in ID/EX needs the load result
- No forwarding path available (or forwarding insufficient)

```systemverilog
load_use_hazard = (mem_valid && mem_mem_read &&
                   ((id_rs1_addr != 0 && id_rs1_addr == mem_rd_addr) ||
                    (id_rs2_addr != 0 && id_rs2_addr == mem_rd_addr)));

hazard_stall = load_use_hazard;
```

### Forwarding Detection

(If implemented) Detects when data can be forwarded from:
- EX/MEM stage to EX stage (MEM forwarding)
- MEM/WB stage to EX stage (WB forwarding)

### Usage Example

```systemverilog
hazard_unit hazards (
  .clk(clk),
  .rst(rst),
  .id_rs1_addr_0(id_ex_out_0.rs1_addr),
  .id_rs2_addr_0(id_ex_out_0.rs2_addr),
  .id_valid_0(id_ex_out_0.valid),
  // ... other ID/EX inputs
  .mem_rd_addr_0(ex_mem_out_0.rd_addr),
  .mem_rd_we_0(ex_mem_out_0.rd_we),
  .mem_valid_0(ex_mem_out_0.valid),
  .mem_mem_read_0(ex_mem_out_0.mem_read),
  // ... MEM/WB inputs
  .hazard_stall(hazard_stall),
  // ... forwarding outputs
);
```

### Implementation Notes

1. **Conservative**: May stall more than strictly necessary
2. **No R0 Hazards**: R0 reads don't cause hazards (hardwired to 0)
3. **Dual-Issue Aware**: Checks hazards for both instruction slots

### Related Modules
- `core_top.sv`: Uses hazard_stall in stall_pipeline logic
- `issue_unit.sv`: Prevents dual-issue when hazards exist
- `execute_stage.sv`: May use forwarding signals
