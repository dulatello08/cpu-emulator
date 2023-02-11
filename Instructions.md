# Instructions

| Instruction | Opcode | Description                                                                                                                                                 | Syntax               |
|-------------|--------|-------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------------|
| NOP         | 0x00   | Do nothing                                                                                                                                                  | NOP                  |
| ADD         | 0x01   | Add operand 2 to the value in the operand Rd                                                                                                                | ADD Rd, Operand2     |
| SUB         | 0x02   | Subtract operand 2 from the value in the operand Rd                                                                                                         | SUB Rd, Operand2     |
| MUL         | 0x03   | Multiply the value in the operand Rd by operand 2                                                                                                           | MUL Rd, Operand2     |
| ADM         | 0x04   | Store sum of memory address at operand 2 and register Rn in register Rd                                                                                     | ADD Rd, Rn, Operand2 |
| SBM         | 0x05   | Store difference of memory address at operand2 and register Rn in register Rd                                                                               | SMB Rd, Rn, Operand2 |
| MLM         | 0x06   | Multiply register Rn by memory address at operand 2 and store in register Rd                                                                                | MLM Rd, Rn, Operand2 |
| ADR         | 0x07   | Store sum of registers Rd and Rn in memory address at operand 2                                                                                             | ADR Rd, Rn, Operand2 |
| SBR         | 0x08   | Store difference of registers Rd and Rn in memory address at operand 2                                                                                      | SBR Rd, Rn, Operand2 |
| MLR         | 0x09   | Multiply registers Rd and Rn and store in memory address at operand 2                                                                                       | MLR Rd, Rn, Operand2 |
| CLZ         | 0x0A   | Count the number of leading zeros in register Rn and store in Rd                                                                                            | CLZ Rd Rn            |
| STO         | 0x0B   | Store operand 2 in the operand Rd                                                                                                                           | STO Rd, Operand2     |
| STM         | 0x0C   | Store the value in the register Rd in the data memory at the operand 2                                                                                      | STM Rd, Operand2     |
| LDM         | 0x0D   | Load the value in the memory at the address in operand 2 into the register Rd                                                                               | LDM Rd, Operand2     |
| PSH         | 0x0E   | Push the value in the register Rn at the specified address onto a stack                                                                                     | PSH Rd, Operand2     |
| POP         | 0x0F   | Pop a value from the stack and store it in the register Rd                                                                                                  | POP Rd               |
| PRT         | 0x10   | Print string of ASCII characters from memory with start address from register Rn and end until null terminator (0x80 is terminator)                         | PRT Rd               |
| BRN         | 0x11   | Branch to value specified in operand 2                                                                                                                      | BRN Label            |
| BRZ         | 0x12   | Branch to value specified in operand 2 if zero flag was set                                                                                                 | BRZ Label            |
| BRO         | 0x13   | Branch to value specified in operand 2 if overflow flag was not set                                                                                         | BRO Label            |
| BRR         | 0x14   | Branch to value specified in operand2 if register Rd equals to Rn register                                                                                  | BRR Rd, Rn, Label    |
| BNR         | 0x15   | Branch to value specified in operand2 if register Rd does not equal to Rn register                                                                          | BNR Rd, Rn, Label    |
| HLT         | 0x16   | Halt                                                                                                                                                        | HLT                  |
| TSK         | 0x17   | Create a new task, takes argument of memory address of the task's entry point. Insert the task into the task queue. Store task id in Rd.                    | TSK Rd, Operand2     |
| SCH         | 0x18   | Start the scheduler, should initialize the task queue, set the current task to the first task in the queue with kernel mode, and begin the scheduling loop. | SCH                  |
| SWT         | 0x19   | Switch to a specific task, takes argument of task's unique id. Update the task queue accordingly.                                                           | SWT Rd               |
| KIL         | 0x1A   | Kill a specific task, takes argument of task's unique id. Remove the task from the task queue and free the memory allocated for the task.                   | KIL Rd               |

# Memory Map

| Memory Address  | Usage               |
|-----------------|---------------------|
| 0x0000 - 0x00~~ | Program memory      |
| 0x00~~ - 0xeffe | Usable memory       |
| 0xeffe          | Flash IO Port       |
| 0xefff - 0xFFFF | Current flash block |