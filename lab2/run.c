/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   run.c                                                     */
/*                                                             */
/***************************************************************/

#include <stdio.h>
#include "util.h"
#include "run.h"

/***************************************************************/
/*                                                             */
/* Procedure: process_instruction                              */
/*                                                             */
/* Purpose: Process one instrction                             */
/*                                                             */
/***************************************************************/
void process_instruction() {
    instruction instr = INST_INFO[(CURRENT_STATE.PC - MEM_TEXT_START) / 4];
    unsigned char rs;
    unsigned char rt;
    unsigned char rd;
    unsigned char shamt;
    short imm;
    uint32_t target;
    int instr_index;

    CURRENT_STATE.PC += 4;
    switch (instr.opcode) {
        // R-type
        case 0:
            rs = instr.r_t.r_i.rs;
            rt = instr.r_t.r_i.rt;
            rd = instr.r_t.r_i.r_i.r.rd;
            shamt = instr.r_t.r_i.r_i.r.shamt;

            switch (instr.func_code) {
                // ADDU
                case 33:
                    CURRENT_STATE.REGS[rd] =
                        CURRENT_STATE.REGS[rs] + CURRENT_STATE.REGS[rt];
                    break;
                // AND
                case 36:
                    CURRENT_STATE.REGS[rd] =
                        CURRENT_STATE.REGS[rs] & CURRENT_STATE.REGS[rt];
                    break;
                // JR
                case 8:
                    CURRENT_STATE.PC = CURRENT_STATE.REGS[rs];
                    break;
                // NOR
                case 39:
                    CURRENT_STATE.REGS[rd] =
                        ~(CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt]);
                    break;
                // OR
                case 37:
                    CURRENT_STATE.REGS[rd] =
                        CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt];
                    break;
                // SLTU
                case 43:
                    CURRENT_STATE.REGS[rd] =
                        (CURRENT_STATE.REGS[rs] < CURRENT_STATE.REGS[rt])
                        ? 1 : 0;
                    break;
                // SLL
                case 0:
                    CURRENT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] << shamt;
                    break;
                // SRL
                case 2:
                    CURRENT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] >> shamt;
                    break;
                // SUBU
                case 35:
                    CURRENT_STATE.REGS[rd] =
                        CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
                    break;
                default:
                    printf("Unknown instruction type: %u\n", instr.value);
            }
            break;
        // J-type
        // J
        case 2:
            target = instr.r_t.target;
            CURRENT_STATE.PC =
                (CURRENT_STATE.PC & 0xF0000000) | (target << 2);
            break;
        // JAL
        case 3:
            target = instr.r_t.target;
            CURRENT_STATE.REGS[31] = CURRENT_STATE.PC + 4;
            CURRENT_STATE.PC =
                (CURRENT_STATE.PC & 0xF0000000) | (target << 2);
            break;
        // I-type
        default:
            rs = instr.r_t.r_i.rs;
            rt = instr.r_t.r_i.rt;
            imm = instr.r_t.r_i.r_i.imm;

            switch (instr.opcode) {
                // ADDIU
                case 9:
                    CURRENT_STATE.REGS[rt] =
                        CURRENT_STATE.REGS[rs] + (int32_t) imm;
                    break;
                // ANDI
                case 12:
                    CURRENT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] & imm;
                    break;
                // BEQ
                case 4:
                    if (CURRENT_STATE.REGS[rs] == CURRENT_STATE.REGS[rt]) {
                        CURRENT_STATE.PC += imm << 2;
                    }
                    break;
                // BNE
                case 5:
                    if (CURRENT_STATE.REGS[rs] != CURRENT_STATE.REGS[rt]) {
                        CURRENT_STATE.PC += imm << 2;
                    }
                    break;
                // LUI
                case 15:
                    CURRENT_STATE.REGS[rt] = imm << 16;
                    break;
                // LW
                case 35:
                    CURRENT_STATE.REGS[rt] =
                        mem_read_32(CURRENT_STATE.REGS[rs] + (int32_t) imm);
                    break;
                // ORI
                case 13:
                    CURRENT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] | imm;
                    break;
                // SLTIU
                case 11:
                    CURRENT_STATE.REGS[rt] =
                        (CURRENT_STATE.REGS[rs] < (uint32_t) imm) ? 1 : 0;
                    break;
                // SW
                case 43:
                    mem_write_32(
                            CURRENT_STATE.REGS[rs] + (int32_t) imm,
                            CURRENT_STATE.REGS[rt]);
                    break;
                default:
                    printf("Unknown instruction type: %u\n", instr.value);
            }
            break;
    }

    instr_index = (CURRENT_STATE.PC - MEM_TEXT_START) / 4;
    if (instr_index >= NUM_INST) {
        RUN_BIT = FALSE;
        printf("Run bit unset pc: %x\n", CURRENT_STATE.PC);
    }
}
