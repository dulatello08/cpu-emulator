# Module Reference Documentation

This directory contains detailed documentation for each RTL module in the NeoCore16x32 CPU implementation.

## Module List

### Core Integration
- [core_top.sv](core_top.md) - Top-level CPU integration

### Pipeline Stages
- [fetch_unit.sv](fetch_unit.md) - Instruction fetch with variable-length handling
- [decode_unit.sv](decode_unit.md) - Instruction decoder
- [execute_stage.sv](execute_stage.md) - Execute stage wrapper
- [memory_stage.sv](memory_stage.md) - Memory access handler
- [writeback_stage.sv](writeback_stage.md) - Write-back logic

### Execution Units
- [alu.sv](alu.md) - Arithmetic Logic Unit
- [multiply_unit.sv](multiply_unit.md) - 16x16 multiplier
- [branch_unit.sv](branch_unit.md) - Branch condition evaluator

### Control Logic
- [issue_unit.sv](issue_unit.md) - Dual-issue controller
- [hazard_unit.sv](hazard_unit.md) - Hazard detection and forwarding

### Storage
- [register_file.sv](register_file.md) - 16-entry register file
- [unified_memory.sv](unified_memory.md) - Von Neumann memory
- [pipeline_regs.sv](pipeline_regs.md) - Pipeline registers

### Package
- [neocore_pkg.sv](neocore_pkg.md) - Type definitions and constants

## Documentation Format

Each module documentation includes:
1. **Purpose**: What the module does
2. **Interface**: Input/output ports
3. **Parameters**: Configurable parameters (if any)
4. **Functionality**: Detailed operation description
5. **Timing**: Latency and throughput characteristics
6. **Big-Endian Notes**: Byte ordering specifics
7. **Synthesis Notes**: FPGA considerations
8. **Verification**: Test coverage

## Module Hierarchy

```
core_top
├── fetch_unit
├── decode_unit (x2)
├── issue_unit
├── register_file
├── hazard_unit
├── execute_stage
│   ├── alu (x2)
│   ├── multiply_unit (x2)
│   └── branch_unit (x2)
├── memory_stage
├── writeback_stage
├── unified_memory
└── pipeline_regs
    ├── if_id_reg (x2)
    ├── id_ex_reg (x2)
    ├── ex_mem_reg (x2)
    └── mem_wb_reg (x2)
```

## Related Documentation

- [ARCHITECTURE.md](../ARCHITECTURE.md) - Architecture specification
- [PIPELINE.md](../PIPELINE.md) - Pipeline operation
- [MICROARCHITECTURE.md](../MICROARCHITECTURE.md) - Microarchitecture details

