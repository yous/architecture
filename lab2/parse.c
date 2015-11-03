/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   parse.c                                                   */
/*                                                             */
/***************************************************************/

#include <stdio.h>
#include "util.h"
#include "parse.h"

int text_size;
int data_size;

/**************************************************************/
/*                                                            */
/* Procedure : parsing_instr                                  */
/*                                                            */
/* Purpose   : parse binary format instrction and return the  */
/*             instrcution data                               */
/*                                                            */
/* Argument  : buffer - binary string for the instruction     */
/*             index  - order of the instruction              */
/*                                                            */
/**************************************************************/
instruction parsing_instr(const char *buffer, const int index) {
    instruction instr = INST_INFO[index];
    uint32_t inst_binary = fromBinary((char *) buffer);

    instr.opcode = inst_binary >> 26;
    switch (instr.opcode) {
        // R-type
        case 0:
            instr.func_code = inst_binary & 0x3F;
            switch (instr.func_code) {
                // JR
                case 8:
                    instr.r_t.r_i.rs = (inst_binary >> 21) & 0x1F;
                    break;
                // SLL
                case 0:
                // SRL
                case 2:
                    instr.r_t.r_i.rt = (inst_binary >> 16) & 0x1F;
                    instr.r_t.r_i.r_i.r.rd = (inst_binary >> 11) & 0x1F;
                    instr.r_t.r_i.r_i.r.shamt = (inst_binary >> 6) & 0x1F;
                    break;
                // ADDU, AND, NOR, OR, SLTU, SUBU
                default:
                    instr.r_t.r_i.rs = (inst_binary >> 21) & 0x1F;
                    instr.r_t.r_i.rt = (inst_binary >> 16) & 0x1F;
                    instr.r_t.r_i.r_i.r.rd = (inst_binary >> 11) & 0x1F;
                    break;
            }
            break;
        // J-type
        case 2:
        case 3:
            instr.r_t.target = inst_binary & 0x3FFFFFF;
            break;
        // I-type
        default:
            instr.r_t.r_i.rt = (inst_binary >> 16) & 0x1F;
            instr.r_t.r_i.r_i.imm = inst_binary & 0xFFFF;
            // Not LUI
            if (instr.opcode != 15) {
                instr.r_t.r_i.rs = (inst_binary >> 21) & 0x1F;
            }
            break;
    }

    return instr;
}

/**************************************************************/
/*                                                            */
/* Procedure : parsing_data                                   */
/*                                                            */
/* Purpose   : parse binary format data and store data into   */
/*             the data region                                */
/*                                                            */
/* Argument  : buffer - binary string for data                */
/*             index  - order of data                         */
/*                                                            */
/**************************************************************/
void parsing_data(const char *buffer, const int index) {
    mem_write_32(MEM_DATA_START + index, fromBinary((char *) buffer));
}
