## Project Lab3 - Simulating Pipelined Execution

Team 8

## File Description

1. cs311.c

    This contains main function. It uploads instruction and data into memory
    regions, and parse and execute them by using functions.

2. util.c & util.h

    This contains all the data structures. This also contains several useful
    functions. (Read and write data into the memory region, initialize the
    memory region and etc...)

3. parse.c & parse.h

    This contains variables and functions to parse the instruction and data.

4. run.c & run.h

    This contains variables and functions to execute instructions.

5. sample_input

    This directory contains serveral sample input files.

## Compile & Execution

Just type "make" to compile your code.
You can execute your program using this command:
`./cs311sim [-nobp] [-f] [-m addr1:addr2] [-d] [-p] [-n num_instr] inputBinary`

- `-nobp`: Branch prediction is disabled. All conditional branches must add three-cycles bubbles.
- `-f`: Data forwarding support is on.
- `-m`: Dump the memory content form addr1 to addr2 at the end of simulation
- `-d`: Print the register file content and the current PC for each instruction execution. Print memory content too if -m option is enabled.
- `-p`: Print the PCs of instructions in each pipeline stage for each instruction execution.
- `-n`: Number of instructions to execute. Default value is 100.

## Pipeline Register States

### IF_ID

- `instruction *Instr`: 32-bit instruction
- `uint32_t NPC`: 32-bit next PC (PC + 4)
- `unsigned char stall`: Indicates whether the ID stage should be stalled or not

### ID_EX

- `uint32_t NPC`: 32-bit next PC (PC + 4)
- `uint32_t REG1`: REG1 value
- `uint32_t REG2`: REG2 value
- `uint32_t IMM`: Immediate value
- `uint8_t FUNC`: Function code of R-type instruction
- `short RS`: The number of register RS
- `short RT`: The number of register RT
- `short RD`: The number of register RD
- `uint16_t CONTROL`: Control bits for pipeline

### EX_MEM

- `uint32_t ALU_OUT`: ALU output
- `uint32_t BR_TARGET`: Branch target address
- `uint32_t WRITE_DATA`: Output from ALU or memory to write into a register
- `short WRITE_REG`: The number of register to write
- `unsigned char CONTROL`: Control bits for pipeline

### MEM_WB

- `uint32_t ALU_OUT`: ALU output
- `uint32_t MEM_OUT`: Memory output
- `short WRITE_REG`: The number of register to write
- `unsigned char CONTROL`: Control bits for pipeline

## Pipeline Control Bits

|      | EX Stage                                            | MEM Stage                   | WB Stage            |
|------|-----------------------------------------------------|-----------------------------|---------------------|
|      | RegDst | ALUOp3 | ALUOp2 | ALUOp1 | ALUOp0 | ALUSrc | Branch | MemRead | MemWrite | RegWrite | MemToReg |
| Bits | X      | X      | X      | X      | X      | X      | X      | X       | X        | X        | X        |
