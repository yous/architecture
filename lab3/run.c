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
/* Procedure: get_inst_info                                    */
/*                                                             */
/* Purpose: Read insturction information                       */
/*                                                             */
/***************************************************************/
instruction *get_inst_info(uint32_t pc) {
    return &INST_INFO[(pc - MEM_TEXT_START) >> 2];
}

/***************************************************************/
/*                                                             */
/* Procedure: process_instruction                              */
/*                                                             */
/* Purpose: Process one instrction                             */
/*                                                             */
/***************************************************************/
void process_instruction() {
    instruction *inst;
    int i;              // for loop

    /* pipeline */
    for (i = PIPE_STAGE - 1; i > 0; i--) {
        CURRENT_STATE.PIPE[i] = CURRENT_STATE.PIPE[i - 1];
    }
    CURRENT_STATE.PIPE[0] = CURRENT_STATE.PC;

    /* WB stage */
    if (CURRENT_STATE.MEM_WB.CONTROL) {
        unsigned char control = CURRENT_STATE.MEM_WB.CONTROL;
        if (control & 0x2) {
            if (control & 0x1) {
                CURRENT_STATE.REGS[CURRENT_STATE.MEM_WB.WRITE_REG] =
                    CURRENT_STATE.MEM_WB.ALU_OUT;
            } else {
                CURRENT_STATE.REGS[CURRENT_STATE.MEM_WB.WRITE_REG] =
                    CURRENT_STATE.MEM_WB.MEM_OUT;
            }
        }
    }

    /* MEM stage */
    if (CURRENT_STATE.EX_MEM.CONTROL) {
        unsigned char control = CURRENT_STATE.EX_MEM.CONTROL;
        CURRENT_STATE.MEM_WB.CONTROL = CURRENT_STATE.EX_MEM.CONTROL & 0x3;
        CURRENT_STATE.MEM_WB.ALU_OUT = CURRENT_STATE.EX_MEM.ALU_OUT;
        CURRENT_STATE.MEM_WB.WRITE_REG = CURRENT_STATE.EX_MEM.WRITE_REG;

        // Branch
        if (control & 0x10) {
            if (CURRENT_STATE.EX_MEM.ALU_OUT) {
                CURRENT_STATE.PC = CURRENT_STATE.EX_MEM.BR_TARGET;
            }
        }

        // MemRead
        if (control & 0x8) {
            CURRENT_STATE.MEM_WB.MEM_OUT = mem_read_32(
                    CURRENT_STATE.EX_MEM.ALU_OUT);
        }

        // MemWrite
        if (control & 0x4) {
            mem_write_32(
                    CURRENT_STATE.EX_MEM.ALU_OUT,
                    CURRENT_STATE.EX_MEM.WRITE_DATA);
        }
    }

    /* EX stage */
    if (CURRENT_STATE.ID_EX.CONTROL) {
        uint16_t control = CURRENT_STATE.ID_EX.CONTROL;
        uint32_t op1 = CURRENT_STATE.ID_EX.REG1;
        uint32_t op2;
        CURRENT_STATE.EX_MEM.CONTROL = control & 0x1F;
        CURRENT_STATE.EX_MEM.WRITE_DATA = CURRENT_STATE.ID_EX.REG2;

        // ALUSrc
        if (control & 0x20) {
            op2 = (int32_t) CURRENT_STATE.ID_EX.IMM;
        } else {
            op2 = CURRENT_STATE.ID_EX.REG2;
        }

        // RegDst
        if (control & 0x100) {
            CURRENT_STATE.EX_MEM.WRITE_REG = CURRENT_STATE.ID_EX.RD;
        } else {
            CURRENT_STATE.EX_MEM.WRITE_REG = CURRENT_STATE.ID_EX.RT;
        }

        // ALUOp
        switch ((control >> 6) & 0xF) {
            // LW, SW
            case 0:
                CURRENT_STATE.EX_MEM.ALU_OUT = op1 + op2;
                break;
            // BEQ
            case 1:
                CURRENT_STATE.EX_MEM.ALU_OUT = op1 == op2;
                break;
            // R
            case 2:
                switch (CURRENT_STATE.ID_EX.IMM & 0x3F) {
                    // ADDU
                    case 0x21:
                        CURRENT_STATE.EX_MEM.ALU_OUT = op1 + op2;
                        break;
                    // AND
                    case 0x24:
                        CURRENT_STATE.EX_MEM.ALU_OUT = op1 & op2;
                        break;
                    // NOR
                    case 0x27:
                        CURRENT_STATE.EX_MEM.ALU_OUT = ~(op1 | op2);
                        break;
                    // OR
                    case 0x25:
                        CURRENT_STATE.EX_MEM.ALU_OUT = op1 | op2;
                        break;
                    // SLTU
                    case 0x2B:
                        CURRENT_STATE.EX_MEM.ALU_OUT = op1 < op2 ? 1 : 0;
                        break;
                    // SLL
                    case 0x0:
                        CURRENT_STATE.EX_MEM.ALU_OUT = op1 << op2;
                        break;
                    // SRL
                    case 0x2:
                        CURRENT_STATE.EX_MEM.ALU_OUT = op1 >> op2;
                        break;
                    // SUBU
                    case 0x23:
                        CURRENT_STATE.EX_MEM.ALU_OUT = op1 - op2;
                        break;
                    // JR
                    case 0x8:
                        break;
                    default:
                        printf("Unknown function code type: %d\n", FUNC(inst));
                        break;
                }
                break;
            // BNE
            case 3:
                CURRENT_STATE.EX_MEM.ALU_OUT = op1 != op2;
                break;
            // ADDIU
            case 4:
                CURRENT_STATE.EX_MEM.ALU_OUT = op1 + op2;
                break;
            // ANDI
            case 5:
                CURRENT_STATE.EX_MEM.ALU_OUT = op1 & op2;
                break;
            // LUI
            case 6:
                CURRENT_STATE.EX_MEM.ALU_OUT = op2 << 16;
                break;
            // ORI
            case 7:
                CURRENT_STATE.EX_MEM.ALU_OUT = op1 | op2;
                break;
            // SLTIU
            case 8:
                CURRENT_STATE.EX_MEM.ALU_OUT = op1 < op2 ? 1 : 0;
                break;
        }

        CURRENT_STATE.EX_MEM.BR_TARGET = CURRENT_STATE.ID_EX.NPC + (CURRENT_STATE.ID_EX.IMM << 2);
    }

    /* ID stage */
    if (CURRENT_STATE.IF_ID.valid) {
        inst = CURRENT_STATE.IF_ID.Instr;
        CURRENT_STATE.ID_EX.NPC = CURRENT_STATE.IF_ID.NPC;
        CURRENT_STATE.ID_EX.RT = inst->r_t.r_i.rt;
        CURRENT_STATE.ID_EX.RD = inst->r_t.r_i.r_i.r.rd;

        CURRENT_STATE.ID_EX.REG1 = CURRENT_STATE.REGS[inst->r_t.r_i.rs];
        CURRENT_STATE.ID_EX.REG2 = 0;
        CURRENT_STATE.ID_EX.IMM = (int32_t) inst->r_t.r_i.r_i.imm;

        switch (OPCODE(inst)) {
            // (0x001001) ADDIU
            case 0x9:
                // 001001 000 11
                CURRENT_STATE.ID_EX.CONTROL = 0x123;
                break;
            // (0x001100) ANDI
            case 0xC:
                // 001011 000 11
                CURRENT_STATE.ID_EX.CONTROL = 0x163;
                break;
            // (0x001111) LUI
            case 0xF:
                // 001101 000 11
                CURRENT_STATE.ID_EX.CONTROL = 0x1A3;
                break;
            // (0x001101) ORI
            case 0xD:
                // 001111 000 11
                CURRENT_STATE.ID_EX.CONTROL = 0x1E3;
                break;
            // (0x001011) SLTIU
            case 0xB:
                // 010001 000 11
                CURRENT_STATE.ID_EX.CONTROL = 0x223;
                break;
            // (0x100011) LW
            case 0x23:
                // 000001 010 11
                CURRENT_STATE.ID_EX.CONTROL = 0x2B;
                break;
            // (0x101011) SW
            case 0x2B:
                // x00001 001 0x
                CURRENT_STATE.ID_EX.CONTROL = 0x24;
                break;
            // (0x000100) BEQ
            case 0x4:
                // x00010 100 0x
                CURRENT_STATE.ID_EX.CONTROL = 0x50;
                break;
            // (0x000101) BNE
            case 0x5:
                // x00110 100 0x
                CURRENT_STATE.ID_EX.CONTROL = 0xD0;
                break;
            // (0x000000) ADDU, AND, NOR, OR, SLTU, SLL, SRL, SUBU, JR
            case 0x0:
                CURRENT_STATE.ID_EX.REG2 = CURRENT_STATE.REGS[inst->r_t.r_i.rt];
                CURRENT_STATE.ID_EX.IMM = 0;
                // 100100 000 10
                CURRENT_STATE.ID_EX.CONTROL = 0x482;
                break;
            default:
                printf("Unknown instruction type: %d\n", OPCODE(inst));
                break;
        }
    }

    /* IF stage */
    CURRENT_STATE.IF_ID.Instr = get_inst_info(CURRENT_STATE.PC);
    CURRENT_STATE.IF_ID.valid = 1;
    CURRENT_STATE.PC += BYTES_PER_WORD;
    CURRENT_STATE.IF_ID.NPC = CURRENT_STATE.PC;

    if (CURRENT_STATE.PC < MEM_REGIONS[0].start || CURRENT_STATE.PC >= (MEM_REGIONS[0].start + (NUM_INST * 4))) {
        RUN_BIT = FALSE;
    }
}
