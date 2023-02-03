# Instructions


| Instruction | Opcode | Description                                                                                                                           |        Syntax        |
|:-----------:|:------:|:--------------------------------------------------------------------------------------------------------------------------------------|:--------------------:|
|     NOP     |  0x00  | Do nothing                                                                                                                            |         NOP          |
|     ADD     |  0x01  | Add operand 2 to the value in the operand Rd                                                                                          |   ADD Rd, Operand2   |
|     SUB     |  0x02  | Subtract operand 2 from the value in the operand Rd                                                                                   |   SUB Rd, Operand2   |
|     MUL     |  0x03  | Multiply the value in the operand Rd by operand 2                                                                                     |   MUL Rd, Operand2   |
|     ADM     |  0x04  | Store sum of memory address at operand 2 and register Rn in register Rd                                                               | ADD Rd, Rn, Operand2 |
|     SBM     |  0x05  | Store difference of memory address at operand2 and register Rn in register Rd                                                         | SMB Rd, Rn, Operand2 |
|     MLM     |  0x06  | Multiply register Rn by memory address at operand 2 and store in register Rd                                                          | MLM Rd, Rn, Operand2 |
|     ADR     |  0x07  | Store sum of registers Rd and Rn in memory address at operand 2                                                                       | ADR Rd, Rn, Operand2 |
|     SBR     |  0x08  | Store difference of registers Rd and Rn in memory address at operand 2                                                                | SBR Rd, Rn, Operand2 |
|     MLR     |  0x09  | Multiply registers Rd and Rn and store in memory address at operand 2                                                                 | MLR Rd, Rn, Operand2 |
|     CLZ     |  0x0A  | Count the number of leading zeros in register Rn and store in Rd                                                                      |      CLZ Rd Rn       |
|     STO     |  0x0B  | Store operand 2 in the operand Rd                                                                                                     |   STO Rd, Operand2   |
|     STM     |  0x0C  | Store the value in the register Rd in the data memory at the operand 2                                                                |   STM Rd, Operand2   |
|     LDM     |  0x0D  | Load the value in the memory at the address in operand 2 into the register Rd                                                         |   LDM Rd, Operand2   |
|     PSH     |  0x0E  | Push the value in the register Rn at the specified address onto a stack                                                               |   PSH Rd, Operand2   |
|     POP     |  0x0F  | Pop a value from the stack and store it in the register Rd                                                                            |        POP Rd        |
|     PRT     |  0x10  | Print string of ASCII characters from memory with start address from register Rn and end until null terminator (0x80 is terminator)   |        PRT Rd        |
|     RDM     |  0x11  | Read flash memory to data memory, where Rd has address in data memory, Rn address to flash memory, copies single byte to data memory  | RDM Rd, Rn, Operand2 |
|     RNM     |  0x12  | Read data memory to flash memory, where Rd has address in flash memory, Rn address to data memory, copies single byte to flash memory | RNM Rd, Rn, Operand2 |
|     BRN     |  0x13  | Branch to value specified in operand 2                                                                                                |      BRN Label       |
|     BRZ     |  0x14  | Branch to value specified in operand 2 if zero flag was set                                                                           |      BRZ Label       |
|     BRO     |  0x15  | Branch to value specified in operand 2 if overflow flag was set                                                                   |      BRO Label       |
|     BRR     |  0x16  | Branch to value specified in operand2 if register Rd equals to Rn register                                                            |  BRR Rd, Rn, Label   |
|     BNR     |  0x17  | Branch to value specified in operand2 if register Rd does not equal to Rn register                                                    |  BNR Rd, Rn, Label   |
|     HLT     |  0x18  | Halt                                                                                                                                  |         HLT          |
# Memory Map

| Address Range | Description              |
|---------------|--------------------------|
| 0x00 - 0xFD   | Data Memory              |
| 0xFE          | Input                    |
| 0xFF          | Output                   |