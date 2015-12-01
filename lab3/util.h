/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   util.h                                                    */
/*                                                             */
/***************************************************************/

#ifndef _UTIL_H_
#define _UTIL_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define FALSE 0
#define TRUE  1

/* Basic Information */
#define MEM_TEXT_START  0x00400000
#define MEM_TEXT_SIZE   0x00100000
#define MEM_DATA_START  0x10000000
#define MEM_DATA_SIZE   0x00100000
#define MIPS_REGS       32
#define BYTES_PER_WORD  4
#define PIPE_STAGE      5

typedef struct inst_s {
    short opcode;

    /* R-type */
    short func_code;

    union {
        /* R-type or I-type: */
        struct {
            unsigned char rs;
            unsigned char rt;

            union {
                short imm;

                struct {
                    unsigned char rd;
                    unsigned char shamt;
                } r;
            } r_i;
        } r_i;
        /* J-type: */
        uint32_t target;
    } r_t;

    uint32_t value;

    // int32 encoding;
    // imm_expr *expr;
    // char *source_line;
} instruction;

typedef struct CPU_State_Struct {
    uint32_t PC;                /* program counter */
    uint32_t REGS[MIPS_REGS];   /* register file */
    uint32_t PIPE[PIPE_STAGE];  /* pipeline stage */

    struct {
        unsigned char valid;
        instruction *Instr;
        uint32_t NPC;
    } IF_ID;

    struct {
        uint32_t NPC;
        uint32_t REG1;
        uint32_t REG2;
        uint32_t IMM;
        short RT;
        short RD;
        uint16_t CONTROL;
    } ID_EX;

    struct {
        uint32_t ALU_OUT;
        uint32_t BR_TARGET;
        uint32_t WRITE_DATA;
        short WRITE_REG;
        unsigned char CONTROL;
    } EX_MEM;

    struct {
        uint32_t ALU_OUT;
        uint32_t MEM_OUT;
        short WRITE_REG;
        unsigned char CONTROL;
    } MEM_WB;
} CPU_State;

typedef struct {
    uint32_t start, size;
    uint8_t *mem;
} mem_region_t;

/* For PC * Registers */
extern CPU_State CURRENT_STATE;

/* For Instructions */
extern instruction *INST_INFO;
extern int NUM_INST;

/* For Memory Regions */
extern mem_region_t MEM_REGIONS[2];

/* For Execution */
extern int RUN_BIT;     /* run bit */
extern int INSTRUCTION_COUNT;

/* Functions */
char **str_split(char *a_str, const char a_delim);
int fromBinary(char *s);
uint32_t mem_read_32(uint32_t address);
void mem_write_32(uint32_t address, uint32_t value);
void cycle();
void run(int num_cycles);
void go();
void mdump(int start, int stop);
void rdump();
void pdump();
void init_memory();
void init_inst_info();

/* YOU IMPLEMENT THIS FUNCTION */
void process_instruction();

#endif
