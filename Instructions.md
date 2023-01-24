# Instructions


| Instruction | Opcode | Description                                                                                                                                                |        Syntax        |
|:-----------:|:------:|:-----------------------------------------------------------------------------------------------------------------------------------------------------------|:--------------------:|
|     NOP     |  0x00  | Do nothing                                                                                                                                                 |         NOP          |
|     ADD     |  0x01  | Add operand 2 to the value in the operand Rd                                                                                                               |   ADD Rd, Operand2   |
|     SUB     |  0x02  | Subtract operand 2 from the value in the operand Rd                                                                                                        |   SUB Rd, Operand2   |
|     MUL     |  0x03  | Multiply the value in the operand Rd by operand 2                                                                                                          |   MUL Rd, Operand2   |
|     ADM     |  0x04  | Store sum of memory address at operand 2 and register Rn in register Rd                                                                                    | ADD Rd, Rn, Operand2 |
|     SBM     |  0x05  | Store difference of memory address at operand2 and register Rn in register Rd                                                                              | SMB Rd, Rn, Operand2 |
|     MLM     |  0x06  | Multiply register Rn by memory address at operand 2 and store in register Rd                                                                               | MLM Rd, Rn, Operand2 |
|     ADR     |  0x07  | Store sum of registers Rd and Rn in memory address at operand 2                                                                                            |                      |
|     SBR     |  0x08  | Store sum of registers Rd and Rn in memory address at operand 2                                                                                            |                      |
|     MLR     |  0x09  | Multiply registers Rd and Rn and store in memory address at operand 2                                                                                      |                      |
|     CLZ     |  0x0A  | Count the number of leading zeros at register Rn and store at Rd                                                                                           |                      |
|     STO     |  0x0B  | Store operand 2 in the operand Rd                                                                                                                          |                      |
|     STM     |  0x0C  | Store the value in the register Rd in the data memory at the operand 2                                                                                     |                      |
|     LDM     |  0x0D  | Load the value in the memory at the address in operand 2 into the register Rd                                                                              |                      |
|     PSH     |  0x0E  | Push the value in the register Rn at the specified address onto a stack                                                                                    |                      |
|     POP     |  0x0F  | Pop a value from the stack and store it in the register Rd                                                                                                 |                      |
|     MOV     |  0x10  | Move data from one memory address to other, source: Rd, destination: Operand2, print if destination 0xFF                                                   |   PRT Rd, Operand2   |
|     RDM     |  0x11  | Read non-volatile memory and store in data memory using addresses from operands 2 and Rn, starting at address in Rd                                        | RDM Rd, Rn, Operand2 |
|     RNM     |  0x12  | Read data memory and store in non-volatile memory using addresses from registers Rd and Rn, starting at address in Operand 2                               | RNM Rd, Rn, Operand2 |
|     BRN     |  0x13  | Branch to value specified in operand 2                                                                                                                     |                      |
|     BRZ     |  0x14  | Branch to value specified in operand 2 if zero flag was set                                                                                                |                      |
|     BRO     |  0x15  | Branch to value specified in operand 2 if overflow flag was not set                                                                                        |                      |
|     BRR     |  0x16  | Branch to value specified in operand2 if register Rd equals to Rn register                                                                                 |                      |
|     BNR     |  0x17  | Branch to value specified in operand2 if register Rd does not equal to Rn register                                                                         |  BNR Rd, Rn, Label   |
|     HLT     |  0x18  | Halt                                                                                                                                                       |                      |
|     TSK     |  0x19  | Create a new task, takes argument of memory address of the task's entry point. Insert the task into the task queue.                                        |     TSK Operand2     |
|     SCH     |  0x1A  | Start the scheduler, should initialize the task queue, set the current task to the first task in the queue with kernel mode, and begin the scheduling loop |         SCH          |
|     SWT     |  0x1B  | Switch to a specific task, takes argument of task's unique id. Update the task queue accordingly.                                                          |     SWT Operand2     |
|     KIL     |  0x1C  | Kill a specific task, takes argument of task's unique id. Remove the task from the task queue and free the memory allocated for the task.                  |     KIL Operand2     |
|     WTG     |  0x1E  | Wait for a specific event, takes an argument of event id. Block the current task and wait for that event to occur                                          |     WTG Operand2     |
|     EVT     |  0x1F  | Trigger a specific event, takes an argument of event id. Unblock all the tasks that were blocked by the event with the specified id                        |     EVT Operand2     |

# Memory Map

| Address Range | Description              |
|---------------|--------------------------|
| 0x00 - 0xFD   | Data Memory              |
| 0xFE          | Input                    |
| 0xFF          | Output                   |