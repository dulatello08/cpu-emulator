# Instructions

0x00: NOP - Do nothing  
0x01: ADD - Add operand 2 to the value in the specified register  
0x02: SUB - Subtract operand 2 from the value in the specified register  
0x03: MUL - Multiply the value in the specified register by operand 2  
0x04: CLZ - Count the number of leading zeros in operand 2 and store the result in the specified register  
0x05: ADM - Add the value in the data memory at the specified address to the value in the specified register  
0x06: SBM - Subtract the value in the data memory at the specified address from the value in the specified register  
0x07: ASR - Ascii load to register  
0x08: ADR - Increment memory address specified in operand 2 by register specified in operand 1  
0x09: SBR - Subtract register from memory address  
0x0A: MLM - Multiply register by memory  
0x0B: MLR - Multiply memory by register  
0x0C: STO - Store operand 2 in the specified register  
0x0D: STM - Store the value in the specified register in the data memory at the specified address  
0x0E: LDM - Load the value in the data memory at the specified address into the specified register  
0x0F: PSH - Push the value in the data memory at the specified address onto a stack  
0x10: POP - Pop a value from the stack and store it in the data memory at the specified address  
0x11: BRN - Branch to value specified in operand 2  
0x12: BRZ - Branch to value specified in operand 2 if zero flag was set  
0x13: BRO - Branch to value specified in operand 2 if overflow flag was not set  
0x14: HLT - Halt
