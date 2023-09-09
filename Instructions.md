# Instructions


| Instruction | Opcode | Description                                                                                                                                              | Syntax               |
|-------------|--------|----------------------------------------------------------------------------------------------------------------------------------------------------------|----------------------|
| NOP         | 0x00   | Do nothing                                                                                                                                               | NOP                  |
| ADD         | 0x01   | Add operand 2 to the value in the operand Rd                                                                                                             | ADD Rd, Operand2     |
| SUB         | 0x02   | Subtract operand 2 from the value in the operand Rd                                                                                                      | SUB Rd, Operand2     |
| MUL         | 0x03   | Multiply the value in the operand Rd by operand 2                                                                                                        | MUL Rd, Operand2     |
| ADM         | 0x04   | Store sum of memory address at operand 2 and register Rn in register Rd                                                                                  | ADD Rd, Rn, Operand2 |
| SBM         | 0x05   | Store difference of memory address at operand2 and register Rn in register Rd                                                                            | SMB Rd, Rn, Operand2 |
| MLM         | 0x06   | Multiply register Rn by memory address at operand 2 and store in register Rd                                                                             | MLM Rd, Rn, Operand2 |
| ADR         | 0x07   | Store sum of registers Rd and Rn in memory address at operand 2                                                                                          | ADR Rd, Rn, Operand2 |
| SBR         | 0x08   | Store difference of registers Rd and Rn in memory address at operand 2                                                                                   | SBR Rd, Rn, Operand2 |
| MLR         | 0x09   | Multiply registers Rd and Rn and store in memory address at operand 2                                                                                    | MLR Rd, Rn, Operand2 |
| CLZ         | 0x0A   | Count the number of leading zeros in register Rn and store in Rd                                                                                         | CLZ Rd Rn            |
| STO         | 0x0B   | Store operand 2 in the operand Rd                                                                                                                        | STO Rd, Operand2     |
| STM         | 0x0C   | Store the value in the register Rd in the data memory at the operand 2                                                                                   | STM Rd, Operand2     |
| LDM         | 0x0D   | Load the value in the memory at the address in operand 2 into the register Rd                                                                            | LDM Rd, Operand2     |
| PSH         | 0x0E   | Push the value in the register Rn at the specified address onto a stack                                                                                  | PSH Rd, Operand2     |
| POP         | 0x0F   | Pop a value from the stack and store it in the register Rd                                                                                               | POP Rd               |
| BRN         | 0x10   | Branch to value specified in operand 2                                                                                                                   | BRN Label            |
| BRZ         | 0x11   | Branch to value specified in operand 2 if zero flag was set                                                                                              | BRZ Label            |
| BRO         | 0x12   | Branch to value specified in operand 2 if overflow flag was not set                                                                                      | BRO Label            |
| BRR         | 0x13   | Branch to value specified in operand2 if register Rd equals to Rn register                                                                               | BRR Rd, Rn, Label    |
| BNR         | 0x14   | Branch to value specified in operand2 if register Rd does not equal to Rn register                                                                       | BNR Rd, Rn, Label    |
| HLT         | 0x15   | Halt                                                                                                                                                     | HLT                  |
| JSR         | 0x16   | Jump to subroutine at address of operand 1 and 2. Set inSubroutine flag to true.                                                                         | JSR Operand1+2       |
| OSR         | 0x17   | Jump out of subroutine use PC state saved in stack. Set inSubroutine flag to false.                                                                      | OSR                  |
| RSM         | 0x18   | Store the value in register Rd into the data memory at the address indicated by the top 2 bytes of the stack (Pops values from stack, subject to change) | RSM Rd               |
| RLD         | 0x19   | Load value of data memory at the address indicated by the top 2 bytes of the stack (Pops values from stack, subject to change) into register Rd          | RLD Rd               |

| Uses no additional operand | Uses only register operand(s) | Uses at least one register and memory address | Uses only immediate operand | Uses registers and immediate operand |
|----------------------------|-------------------------------|-----------------------------------------------|-----------------------------|--------------------------------------|
| NOP, HLT, SCH              | CLZ, PSH, POP, SWT, KIL       | ADM, SBM, MLM, ADR, SBR, MLR, STM, LDM        | BRN, BRZ, BRO               | ADD, SUB, MUL, STO, BRR, BNR, TSK    |


# Memory Map
Address range is including start address but excluding end address.

| Address Range   | Memory Space                            |
|-----------------|-----------------------------------------|
| 0x0000 - 0x00~~ | Boot Sector (max 256 bytes)             |
| 0x00~~ - 0xEFD7 | Usable Memory (min 61,174 bytes)        |
| 0xEFF0 - 0xEFF1 | Flags                                   |
| 0xEFF1 - 0xEFF3 | Stack Memory                            |
| 0xEFF3 - 0xEFF7 | MMU Control                             |
| 0xEFF7 - 0xEFFF | Peripheral Control                      |
| 0xEFFF - 0xEFFF | Memory Block                            |
| 0xF000 - 0xFFFF | Reserved for Flash Memory (4,097 bytes) |