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
void process_instruction(int nobp_set, int data_fwd_set) {
    instruction *inst;
    int i;              // for loop
    uint32_t new_pc = CURRENT_STATE.PC + BYTES_PER_WORD;

    /* pipeline */
    for (i = PIPE_STAGE - 1; i > 0; i--) {
        CURRENT_STATE.PIPE[i] = CURRENT_STATE.PIPE[i - 1];
    }
    CURRENT_STATE.PIPE[0] = CURRENT_STATE.PC;

    /* WB stage */
    if (CURRENT_STATE.MEM_WB.stalls > 0) {
        CURRENT_STATE.MEM_WB.stalls--;
    } else {
        unsigned char control = CURRENT_STATE.MEM_WB.CONTROL;
        // RegWrite
        if (control & 0x2) {
            // MemToReg
            if (control & 0x1) {
                CURRENT_STATE.REGS[CURRENT_STATE.MEM_WB.WRITE_REG] =
                    CURRENT_STATE.MEM_WB.MEM_OUT;
            } else {
                CURRENT_STATE.REGS[CURRENT_STATE.MEM_WB.WRITE_REG] =
                    CURRENT_STATE.MEM_WB.ALU_OUT;
            }
        }
    }

    /* MEM stage */
    if (CURRENT_STATE.EX_MEM.stalls > 0) {
        CURRENT_STATE.EX_MEM.stalls--;
        CURRENT_STATE.MEM_WB.CONTROL = 0;
    } else {
        unsigned char control = CURRENT_STATE.EX_MEM.CONTROL;
        CURRENT_STATE.MEM_WB.CONTROL = CURRENT_STATE.EX_MEM.CONTROL & 0x3;
        CURRENT_STATE.MEM_WB.ALU_OUT = CURRENT_STATE.EX_MEM.ALU_OUT;
        CURRENT_STATE.MEM_WB.WRITE_REG = CURRENT_STATE.EX_MEM.WRITE_REG;

        // Branch
        if (control & 0x10) {
            if (CURRENT_STATE.EX_MEM.ALU_OUT) {
                new_pc = CURRENT_STATE.EX_MEM.BR_TARGET;
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
    if (CURRENT_STATE.ID_EX.stalls > 0) {
        CURRENT_STATE.ID_EX.stalls--;
        CURRENT_STATE.EX_MEM.CONTROL = 0;
    } else {
        uint16_t control = CURRENT_STATE.ID_EX.CONTROL;
        uint32_t op1 = CURRENT_STATE.ID_EX.REG1;
        uint32_t op2;
        short write_reg;
        CURRENT_STATE.EX_MEM.CONTROL = control & 0x1F;
        CURRENT_STATE.EX_MEM.WRITE_DATA = CURRENT_STATE.ID_EX.REG2;

        // ALUSrc
        if (control & 0x20) {
            op2 = (int32_t) CURRENT_STATE.ID_EX.IMM;
        } else {
            op2 = CURRENT_STATE.ID_EX.REG2;
        }

        if (data_fwd_set) {
            // MEM/WB RegWrite
            if (CURRENT_STATE.MEM_WB.CONTROL & 0x2) {
                uint32_t forward_data;
                // MemToReg
                if (CURRENT_STATE.MEM_WB.CONTROL & 0x1) {
                    forward_data = CURRENT_STATE.MEM_WB.MEM_OUT;
                } else {
                    forward_data = CURRENT_STATE.MEM_WB.ALU_OUT;
                }

                // ALUSrc
                if (!(control & 0x20)) {
                    if (CURRENT_STATE.ID_EX.RT ==
                            CURRENT_STATE.MEM_WB.WRITE_REG) {
                        op2 = forward_data;
                    }
                }
                if ((control >> 6) & 0xF == 2
                        && (CURRENT_STATE.ID_EX.IMM == 0x0
                            || CURRENT_STATE.ID_EX.IMM == 0x2)) {
                    // SLL, SRL
                    if (CURRENT_STATE.ID_EX.RT ==
                            CURRENT_STATE.MEM_WB.WRITE_REG) {
                        op1 = forward_data;
                    }
                } else if ((control >> 6) & 0xF != 6) {
                    // Not LUI
                    if (CURRENT_STATE.ID_EX.RS ==
                            CURRENT_STATE.MEM_WB.WRITE_REG) {
                        op1 = forward_data;
                    }
                }
            }

            // EX_MEM RegWrite
            if (CURRENT_STATE.EX_MEM.CONTROL & 0x2) {
                uint32_t forward_data = CURRENT_STATE.EX_MEM.ALU_OUT;

                // ALUSrc
                if (!(control & 0x20)) {
                    if (CURRENT_STATE.ID_EX.RT ==
                            CURRENT_STATE.EX_MEM.WRITE_REG) {
                        op2 = forward_data;
                    }
                }
                if ((control >> 6) & 0xF == 2
                        && (CURRENT_STATE.ID_EX.IMM == 0x0
                            || CURRENT_STATE.ID_EX.IMM == 0x2)) {
                    // SLL, SRL
                    if (CURRENT_STATE.ID_EX.RT ==
                            CURRENT_STATE.EX_MEM.WRITE_REG) {
                        op1 = forward_data;
                    }
                } else if ((control >> 6) & 0xF != 6) {
                    // Not LUI
                    if (CURRENT_STATE.ID_EX.RS ==
                            CURRENT_STATE.EX_MEM.WRITE_REG) {
                        op1 = forward_data;
                    }
                }
            }
        }

        // RegDst
        if (control & 0x400) {
            write_reg = CURRENT_STATE.ID_EX.RD;
        } else {
            write_reg = CURRENT_STATE.ID_EX.RT;
        }
        CURRENT_STATE.EX_MEM.WRITE_REG = write_reg;

        // ALUOp
        switch ((control >> 6) & 0xF) {
            // LW, SW
            case 0:
                CURRENT_STATE.EX_MEM.ALU_OUT = op1 + op2;
                break;
            // BEQ
            case 1:
                CURRENT_STATE.EX_MEM.ALU_OUT = op1 == op2;
                if (!nobp_set && !CURRENT_STATE.EX_MEM.ALU_OUT) {
                    new_pc = CURRENT_STATE.PIPE[2] + BYTES_PER_WORD;
                    CURRENT_STATE.IF_ID.stalls = 3;
                    CURRENT_STATE.ID_EX.stalls = 3;
                    CURRENT_STATE.EX_MEM.stalls = 3;
                }
                break;
            // R
            case 2:
                switch (CURRENT_STATE.ID_EX.FUNC) {
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
                        op2 = CURRENT_STATE.ID_EX.IMM;
                        CURRENT_STATE.EX_MEM.ALU_OUT = op1 << op2;
                        break;
                    // SRL
                    case 0x2:
                        op2 = CURRENT_STATE.ID_EX.IMM;
                        CURRENT_STATE.EX_MEM.ALU_OUT = op1 >> op2;
                        break;
                    // SUBU
                    case 0x23:
                        CURRENT_STATE.EX_MEM.ALU_OUT = op1 - op2;
                        break;
                    default:
                        printf("Unknown function code type: %d\n",
                                CURRENT_STATE.ID_EX.IMM);
                        break;
                }
                break;
            // BNE
            case 3:
                CURRENT_STATE.EX_MEM.ALU_OUT = op1 != op2;
                if (!nobp_set && !CURRENT_STATE.EX_MEM.ALU_OUT) {
                    new_pc = CURRENT_STATE.PIPE[2] + BYTES_PER_WORD;
                    CURRENT_STATE.IF_ID.stalls = 3;
                    CURRENT_STATE.ID_EX.stalls = 3;
                    CURRENT_STATE.EX_MEM.stalls = 3;
                }
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

        // RegWrite
        if (control & 0x2) {
            inst = CURRENT_STATE.IF_ID.Instr;
            switch (OPCODE(inst)) {
                // (0x001001) ADDIU
                case 0x9:
                // (0x001100) ANDI
                case 0xC:
                // (0x001111) LUI
                case 0xF:
                // (0x001101) ORI
                case 0xD:
                // (0x001011) SLTIU
                case 0xB:
                // (0x101011) SW
                case 0x2B:
                    if (write_reg == RS(inst)) {
                        if (!data_fwd_set) {
                            CURRENT_STATE.IF_stalls = 2;
                            CURRENT_STATE.IF_ID.stalls = 2;
                            CURRENT_STATE.ID_EX.stalls = 2;
                        }
                    }
                    break;
                // (0x100011) LW
                case 0x23:
                    if (write_reg == RS(inst)) {
                        if (!data_fwd_set) {
                            CURRENT_STATE.IF_stalls = 3;
                            CURRENT_STATE.IF_ID.stalls = 3;
                            CURRENT_STATE.ID_EX.stalls = 3;
                        } else {
                            CURRENT_STATE.IF_stalls = 1;
                            CURRENT_STATE.IF_ID.stalls = 1;
                            CURRENT_STATE.ID_EX.stalls = 1;
                        }
                    }
                    break;
                // (0x000100) BEQ
                case 0x4:
                // (0x000101) BNE
                case 0x5:
                    if (write_reg == RS(inst) || write_reg == RT(inst)) {
                        if (!data_fwd_set) {
                            CURRENT_STATE.IF_stalls = 2;
                            CURRENT_STATE.IF_ID.stalls = 2;
                            CURRENT_STATE.ID_EX.stalls = 2;
                        }
                    }
                    break;
                // (0x000000) ADDU, AND, NOR, OR, SLTU, SLL, SRL, SUBU, JR
                case 0x0:
                    switch (FUNC(inst)) {
                        // SLL
                        case 0x0:
                        // SRL
                        case 0x2:
                            if (write_reg == RT(inst)) {
                                if (!data_fwd_set) {
                                    CURRENT_STATE.IF_stalls = 2;
                                    CURRENT_STATE.IF_ID.stalls = 2;
                                    CURRENT_STATE.ID_EX.stalls = 2;
                                }
                            }
                            break;
                        // JR
                        case 0x8:
                            if (write_reg == RS(inst)) {
                                if (!data_fwd_set) {
                                    CURRENT_STATE.IF_stalls = 2;
                                    CURRENT_STATE.IF_ID.stalls = 2;
                                    CURRENT_STATE.ID_EX.stalls = 2;
                                }
                            }
                            break;
                        default:
                            if (write_reg == RS(inst)
                                    || write_reg == RT(inst)) {
                                if (!data_fwd_set) {
                                    CURRENT_STATE.IF_stalls = 2;
                                    CURRENT_STATE.IF_ID.stalls = 2;
                                    CURRENT_STATE.ID_EX.stalls = 2;
                                }
                            }
                    }
                    break;
            }
        }
    }

    /* ID stage */
    if (CURRENT_STATE.IF_ID.stalls > 0) {
        CURRENT_STATE.IF_ID.stalls--;
        CURRENT_STATE.ID_EX.CONTROL = 0;
    } else if (CURRENT_STATE.IF_ID.valid) {
        inst = CURRENT_STATE.IF_ID.Instr;
        CURRENT_STATE.ID_EX.NPC = CURRENT_STATE.IF_ID.NPC;
        CURRENT_STATE.ID_EX.RS = RS(inst);
        CURRENT_STATE.ID_EX.RT = RT(inst);
        CURRENT_STATE.ID_EX.RD = RD(inst);

        CURRENT_STATE.ID_EX.REG1 = CURRENT_STATE.REGS[RS(inst)];
        CURRENT_STATE.ID_EX.REG2 = 0;
        CURRENT_STATE.ID_EX.IMM = (int32_t) IMM(inst);

        switch (OPCODE(inst)) {
            // (0x001001) ADDIU
            case 0x9:
                // 001001 000 10
                CURRENT_STATE.ID_EX.CONTROL = 0x122;
                break;
            // (0x001100) ANDI
            case 0xC:
                // 001011 000 10
                CURRENT_STATE.ID_EX.CONTROL = 0x162;
                break;
            // (0x001111) LUI
            case 0xF:
                // 001101 000 10
                CURRENT_STATE.ID_EX.CONTROL = 0x1A2;
                break;
            // (0x001101) ORI
            case 0xD:
                // 001111 000 10
                CURRENT_STATE.ID_EX.CONTROL = 0x1E2;
                break;
            // (0x001011) SLTIU
            case 0xB:
                // 010001 000 10
                CURRENT_STATE.ID_EX.CONTROL = 0x222;
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
                if (!nobp_set) {
                    new_pc = CURRENT_STATE.IF_ID.NPC + (IMM(inst) << 2);
                }
                // x00010 100 0x
                CURRENT_STATE.ID_EX.CONTROL = 0x50;
                break;
            // (0x000101) BNE
            case 0x5:
                if (!nobp_set) {
                    new_pc = CURRENT_STATE.IF_ID.NPC + (IMM(inst) << 2);
                }
                // x00110 100 0x
                CURRENT_STATE.ID_EX.CONTROL = 0xD0;
                break;
            // (0x000010) J
            case 0x2:
                new_pc = CURRENT_STATE.IF_ID.NPC + (IMM(inst) << 2);
                CURRENT_STATE.ID_EX.CONTROL = 0;
                break;
            // (0x000011) JAL
            case 0x3:
                CURRENT_STATE.REGS[31] = CURRENT_STATE.IF_ID.NPC;
                new_pc = IMM(inst) << 2;
                CURRENT_STATE.ID_EX.CONTROL = 0;
                break;
            // (0x000000) ADDU, AND, NOR, OR, SLTU, SLL, SRL, SUBU, JR
            case 0x0:
                switch (FUNC(inst)) {
                    // SLL
                    case 0x0:
                    // SRL
                    case 0x2:
                        CURRENT_STATE.ID_EX.REG1 = CURRENT_STATE.REGS[RT(inst)];
                        CURRENT_STATE.ID_EX.IMM = SHAMT(inst);
                        CURRENT_STATE.ID_EX.FUNC = FUNC(inst);
                        // 100100 000 10
                        CURRENT_STATE.ID_EX.CONTROL = 0x482;
                        break;
                    // JR
                    case 0x8:
                        new_pc = CURRENT_STATE.REGS[RS(inst)];
                        CURRENT_STATE.ID_EX.CONTROL = 0;
                        break;
                    default:
                        CURRENT_STATE.ID_EX.REG2 = CURRENT_STATE.REGS[RT(inst)];
                        CURRENT_STATE.ID_EX.FUNC = FUNC(inst);
                        // 100100 000 10
                        CURRENT_STATE.ID_EX.CONTROL = 0x482;
                        break;
                }
                break;
            default:
                printf("Unknown instruction type: %d\n", OPCODE(inst));
                break;
        }
    }

    /* IF stage */
    if (CURRENT_STATE.IF_stalls > 0) {
        CURRENT_STATE.IF_stalls--;
        CURRENT_STATE.IF_ID.valid = FALSE;
    } else {
        inst = get_inst_info(CURRENT_STATE.PC);
        CURRENT_STATE.IF_ID.valid = TRUE;
        CURRENT_STATE.IF_ID.Instr = inst;
        CURRENT_STATE.PC = new_pc;
        CURRENT_STATE.IF_ID.NPC = CURRENT_STATE.PC + BYTES_PER_WORD;

        switch (OPCODE(inst)) {
            // (0x000100) BEQ
            case 0x4:
                if (nobp_set) {
                    CURRENT_STATE.IF_stalls = 3;
                } else {
                    CURRENT_STATE.IF_stalls = 1;
                }
                break;
            // (0x000101) BNE
            case 0x5:
                if (nobp_set) {
                    CURRENT_STATE.IF_stalls = 3;
                } else {
                    CURRENT_STATE.IF_stalls = 1;
                }
                break;
            // (0x000010) J
            case 0x2:
            // (0x000011) JAL
            case 0x3:
                CURRENT_STATE.IF_stalls = 1;
                break;
            // (0x000000) ADDU, AND, NOR, OR, SLTU, SLL, SRL, SUBU, JR
            case 0x0:
                // JR
                if (FUNC(inst) == 0x8) {
                    CURRENT_STATE.IF_stalls = 1;
                }
                break;
        }
    }

    if (CURRENT_STATE.PC < MEM_REGIONS[0].start || CURRENT_STATE.PC >= (MEM_REGIONS[0].start + (NUM_INST * 4))) {
        RUN_BIT = FALSE;
    }
}
