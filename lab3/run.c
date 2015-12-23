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

void pipe_stall(CPU_State *state) {
    state->IF_stall = TRUE;
    state->IF_ID.stall = TRUE;
}

void pipe_branch_flush(CPU_State *state, int stage) {
    state->PC = CURRENT_STATE.PIPE[stage] + BYTES_PER_WORD;
    state->flush[0] = TRUE;
    state->flush[1] = TRUE;
}

void pipe_jump_flush(CPU_State *state) {
    state->flush[1] = TRUE;
}

void process_IF(CPU_State *state) {
    instruction *inst;
    if (state->IF_stall) {
        state->PIPE[0] = CURRENT_STATE.PIPE[0];
        state->PC = CURRENT_STATE.PC;
        return;
    }

    if (state->update_pc) {
        state->PIPE[0] = state->PC;
    } else {
        state->PIPE[0] = CURRENT_STATE.PC + BYTES_PER_WORD;
        state->PC = CURRENT_STATE.PC + BYTES_PER_WORD;
    }
    inst = get_inst_info(CURRENT_STATE.PC);
    state->IF_ID.Instr = inst;
    state->IF_ID.NPC = CURRENT_STATE.PC + BYTES_PER_WORD;
    state->PIPE[1] = CURRENT_STATE.PIPE[0];
}

void process_ID(CPU_State *state, int nobp_set) {
    instruction *inst;
    if (state->IF_ID.stall) {
        state->PIPE[1] = CURRENT_STATE.PIPE[1];
        return;
    }
    if (!CURRENT_STATE.PIPE[1]) {
        state->PIPE[2] = 0;
        return;
    }

    inst = CURRENT_STATE.IF_ID.Instr;
    state->ID_EX.NPC = CURRENT_STATE.IF_ID.NPC;
    state->ID_EX.RS = RS(inst);
    state->ID_EX.RT = RT(inst);
    state->ID_EX.RD = RD(inst);

    state->ID_EX.REG1 = CURRENT_STATE.REGS[RS(inst)];
    state->ID_EX.REG2 = CURRENT_STATE.REGS[RT(inst)];

    switch (OPCODE(inst)) {
        // (0x001001) ADDIU
        case 0x9:
            state->ID_EX.IMM = (int32_t) IMM(inst);
            // 001001 000 10
            state->ID_EX.CONTROL = 0x122;
            break;
        // (0x001100) ANDI
        case 0xC:
            state->ID_EX.IMM = (uint32_t) (unsigned short) IMM(inst);
            // 001011 000 10
            state->ID_EX.CONTROL = 0x162;
            break;
        // (0x001111) LUI
        case 0xF:
            state->ID_EX.IMM = IMM(inst);
            // 001101 000 10
            state->ID_EX.CONTROL = 0x1A2;
            break;
        // (0x001101) ORI
        case 0xD:
            state->ID_EX.IMM = (uint32_t) (unsigned short) IMM(inst);
            // 001111 000 10
            state->ID_EX.CONTROL = 0x1E2;
            break;
        // (0x001011) SLTIU
        case 0xB:
            state->ID_EX.IMM = (int32_t) IMM(inst);
            // 010001 000 10
            state->ID_EX.CONTROL = 0x222;
            break;
        // (0x100011) LW
        case 0x23:
            state->ID_EX.IMM = (int32_t) IMM(inst);
            // 000001 010 11
            state->ID_EX.CONTROL = 0x2B;
            break;
        // (0x101011) SW
        case 0x2B:
            state->ID_EX.IMM = (int32_t) IMM(inst);
            // x00001 001 0x
            state->ID_EX.CONTROL = 0x24;
            break;
        // (0x000100) BEQ
        case 0x4:
            state->ID_EX.IMM = (int32_t) IMM(inst);
            if (nobp_set) {
                pipe_branch_flush(state, 1);
            } else {
                if (!state->update_pc) {
                    state->update_pc = TRUE;
                    state->PC = CURRENT_STATE.IF_ID.NPC + (IMM(inst) << 2);
                }
                pipe_jump_flush(state);
            }
            // x00010 100 0x
            state->ID_EX.CONTROL = 0x50;
            break;
        // (0x000101) BNE
        case 0x5:
            state->ID_EX.IMM = (int32_t) IMM(inst);
            if (nobp_set) {
                pipe_branch_flush(state, 1);
            } else {
                if (!state->update_pc) {
                    state->update_pc = TRUE;
                    state->PC = CURRENT_STATE.IF_ID.NPC + (IMM(inst) << 2);
                }
                pipe_jump_flush(state);
            }
            // x00110 100 0x
            state->ID_EX.CONTROL = 0xD0;
            break;
        // (0x000010) J
        case 0x2:
            if (!state->update_pc) {
                state->update_pc = TRUE;
                state->PC = TARGET(inst) << 2;
            }
            pipe_jump_flush(state);
            // x10011 000 0x
            state->ID_EX.CONTROL = 0x260;
            break;
        // (0x000011) JAL
        case 0x3:
            if (!state->update_pc) {
                state->update_pc = TRUE;
                state->PC = TARGET(inst) << 2;
            }
            pipe_jump_flush(state);
            // x10101 000 10
            state->ID_EX.CONTROL = 0x2A2;
            break;
        // (0x000000) ADDU, AND, NOR, OR, SLTU, SLL, SRL, SUBU, JR
        case 0x0:
            switch (FUNC(inst)) {
                // SLL
                case 0x0:
                // SRL
                case 0x2:
                    state->ID_EX.IMM = SHAMT(inst);
                    state->ID_EX.FUNC = FUNC(inst);
                    // 100100 000 10
                    state->ID_EX.CONTROL = 0x482;
                    break;
                // JR
                case 0x8:
                    if (!state->update_pc) {
                        state->update_pc = TRUE;
                        state->PC = CURRENT_STATE.REGS[RS(inst)];
                    }
                    pipe_jump_flush(state);
                    state->ID_EX.CONTROL = 0;
                    break;
                default:
                    state->ID_EX.FUNC = FUNC(inst);
                    // 100100 000 10
                    state->ID_EX.CONTROL = 0x482;
                    break;
            }
            break;
        default:
            printf("Unknown instruction type: %d\n", OPCODE(inst));
            break;
    }
    state->PIPE[2] = CURRENT_STATE.PIPE[1];
}

void process_EX(CPU_State *state, int nobp_set, int data_fwd_set) {
    uint16_t control;
    uint32_t reg1;
    uint32_t reg2;
    uint32_t imm;
    uint32_t op1;
    uint32_t op2;
    short write_reg;
    uint32_t forward_data;
    instruction *inst;
    if (!CURRENT_STATE.PIPE[2]) {
        state->PIPE[3] = 0;
        return;
    }

    control = CURRENT_STATE.ID_EX.CONTROL;
    reg1 = CURRENT_STATE.ID_EX.REG1;
    reg2 = CURRENT_STATE.ID_EX.REG2;
    imm = CURRENT_STATE.ID_EX.IMM;
    state->EX_MEM.CONTROL = control & 0x1F;
    state->EX_MEM.WRITE_DATA = CURRENT_STATE.ID_EX.REG2;

    if (data_fwd_set) {
        // MEM_WB
        write_reg = CURRENT_STATE.MEM_WB.WRITE_REG;
        // RegWrite
        if (CURRENT_STATE.MEM_WB.CONTROL & 0x2 && write_reg) {
            // MemToReg
            if (CURRENT_STATE.MEM_WB.CONTROL & 0x1) {
                forward_data = CURRENT_STATE.MEM_WB.MEM_OUT;
            } else {
                forward_data = CURRENT_STATE.MEM_WB.ALU_OUT;
            }
            if (write_reg == CURRENT_STATE.ID_EX.RS) {
                reg1 = forward_data;
            }
            if (write_reg == CURRENT_STATE.ID_EX.RT) {
                reg2 = forward_data;
            }
        }

        // EX_MEM
        write_reg = CURRENT_STATE.EX_MEM.WRITE_REG;
        // RegWrite
        if (CURRENT_STATE.EX_MEM.CONTROL & 0x2 && write_reg) {
            if (!(CURRENT_STATE.EX_MEM.CONTROL & 0x1)) {
                forward_data = CURRENT_STATE.EX_MEM.ALU_OUT;
                if (write_reg == CURRENT_STATE.ID_EX.RS) {
                    reg1 = forward_data;
                }
                if (write_reg == CURRENT_STATE.ID_EX.RT) {
                    reg2 = forward_data;
                }
            }
        }
    }

    // ALUSrc
    op1 = reg1;
    if (control & 0x20) {
        op2 = imm;
    } else {
        op2 = reg2;
    }

    // RegDst
    if (control & 0x400) {
        write_reg = CURRENT_STATE.ID_EX.RD;
    } else {
        write_reg = CURRENT_STATE.ID_EX.RT;
    }
    state->EX_MEM.WRITE_REG = write_reg;

    // ALUOp
    switch ((control >> 6) & 0xF) {
        // LW, SW
        case 0:
            state->EX_MEM.ALU_OUT = op1 + op2;
            break;
        // BEQ
        case 1:
            state->EX_MEM.ALU_OUT = op1 == op2;
            if (nobp_set) {
                pipe_stall(state);
            }
            break;
        // R
        case 2:
            switch (CURRENT_STATE.ID_EX.FUNC) {
                // ADDU
                case 0x21:
                    state->EX_MEM.ALU_OUT = op1 + op2;
                    break;
                // AND
                case 0x24:
                    state->EX_MEM.ALU_OUT = op1 & op2;
                    break;
                // NOR
                case 0x27:
                    state->EX_MEM.ALU_OUT = ~(op1 | op2);
                    break;
                // OR
                case 0x25:
                    state->EX_MEM.ALU_OUT = op1 | op2;
                    break;
                // SLTU
                case 0x2B:
                    state->EX_MEM.ALU_OUT = op1 < op2 ? 1 : 0;
                    break;
                // SLL
                case 0x0:
                    state->EX_MEM.ALU_OUT = op2 << imm;
                    break;
                // SRL
                case 0x2:
                    state->EX_MEM.ALU_OUT = op2 >> imm;
                    break;
                // SUBU
                case 0x23:
                    state->EX_MEM.ALU_OUT = op1 - op2;
                    break;
                default:
                    printf("Unknown function code type: %d\n",
                            CURRENT_STATE.ID_EX.FUNC);
                    break;
            }
            break;
        // BNE
        case 3:
            state->EX_MEM.ALU_OUT = op1 != op2;
            if (nobp_set) {
                pipe_stall(state);
            }
            break;
        // ADDIU
        case 4:
            state->EX_MEM.ALU_OUT = op1 + op2;
            break;
        // ANDI
        case 5:
            state->EX_MEM.ALU_OUT = op1 & op2;
            break;
        // LUI
        case 6:
            state->EX_MEM.ALU_OUT = op2 << 16;
            break;
        // ORI
        case 7:
            state->EX_MEM.ALU_OUT = op1 | op2;
            break;
        // SLTIU
        case 8:
            state->EX_MEM.ALU_OUT = op1 < (uint32_t) op2 ? 1 : 0;
            break;
        // J
        case 9:
            break;
        // JAL
        case 10:
            state->EX_MEM.ALU_OUT = CURRENT_STATE.ID_EX.NPC;
            write_reg = 31;
            state->EX_MEM.WRITE_REG = write_reg;
            break;
    }

    state->EX_MEM.BR_TARGET = CURRENT_STATE.ID_EX.NPC
        + (CURRENT_STATE.ID_EX.IMM << 2);

    // RegWrite
    if (control & 0x2) {
        int hazard = 0;
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
            // (0x100011) LW
            case 0x23:
                if (write_reg == RS(inst)) {
                    hazard = 1;
                }
                break;
            // (0x101011) SW
            case 0x2B:
                if (write_reg == RT(inst)) {
                    hazard = 1;
                }
                break;
            // (0x000100) BEQ
            case 0x4:
            // (0x000101) BNE
            case 0x5:
                if (write_reg == RS(inst) || write_reg == RT(inst)) {
                    hazard = 1;
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
                            hazard = 1;
                        }
                        break;
                    // JR
                    case 0x8:
                        if (write_reg == RS(inst)) {
                            hazard = 1;
                        }
                        break;
                    default:
                        if (write_reg == RS(inst)
                                || write_reg == RT(inst)) {
                            hazard = 1;
                        }
                }
                break;
        }
        if (hazard) {
            // MemToReg
            if ((control & 0x1 || !data_fwd_set) && CURRENT_STATE.PIPE[1]) {
                pipe_stall(state);
            }
        }
    }
    state->PIPE[3] = CURRENT_STATE.PIPE[2];
}

void process_MEM(CPU_State *state, int nobp_set, int data_fwd_set) {
    unsigned char control;
    short write_reg;
    instruction *inst;
    if (!CURRENT_STATE.PIPE[3]) {
        state->PIPE[4] = 0;
        return;
    }

    control = CURRENT_STATE.EX_MEM.CONTROL;
    state->MEM_WB.CONTROL = CURRENT_STATE.EX_MEM.CONTROL & 0x3;
    state->MEM_WB.ALU_OUT = CURRENT_STATE.EX_MEM.ALU_OUT;
    state->MEM_WB.WRITE_REG = CURRENT_STATE.EX_MEM.WRITE_REG;

    // Branch
    if (control & 0x10) {
        if (nobp_set) {
            if (CURRENT_STATE.EX_MEM.ALU_OUT) {
                state->PC = CURRENT_STATE.EX_MEM.BR_TARGET;
            } else {
                state->PC = CURRENT_STATE.PIPE[3] + BYTES_PER_WORD;
            }
            state->update_pc = TRUE;
        } else {
            if (!CURRENT_STATE.EX_MEM.ALU_OUT) {
                state->PC = CURRENT_STATE.PIPE[3] + BYTES_PER_WORD;
                state->update_pc = TRUE;
                state->flush[1] = TRUE;
                state->flush[2] = TRUE;
            }
        }
    }

    // MemRead
    if (control & 0x8) {
        state->MEM_WB.MEM_OUT = mem_read_32(CURRENT_STATE.EX_MEM.ALU_OUT);
    }

    // MemWrite
    if (control & 0x4) {
        mem_write_32(CURRENT_STATE.EX_MEM.ALU_OUT,
                CURRENT_STATE.EX_MEM.WRITE_DATA);
    }

    // RegWrite
    if (control & 0x2) {
        int hazard = 0;
        write_reg = CURRENT_STATE.EX_MEM.WRITE_REG;
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
            // (0x100011) LW
            case 0x23:
                if (write_reg == RS(inst)) {
                    hazard = 1;
                }
                break;
            // (0x101011) SW
            case 0x2B:
                if (write_reg == RT(inst)) {
                    hazard = 1;
                }
                break;
            // (0x000100) BEQ
            case 0x4:
            // (0x000101) BNE
            case 0x5:
                if (write_reg == RS(inst) || write_reg == RT(inst)) {
                    hazard = 1;
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
                            hazard = 1;
                        }
                        break;
                    // JR
                    case 0x8:
                        if (write_reg == RS(inst)) {
                            hazard = 1;
                        }
                        break;
                    default:
                        if (write_reg == RS(inst)
                                || write_reg == RT(inst)) {
                            hazard = 1;
                        }
                }
                break;
        }
        if (hazard) {
            if (!data_fwd_set && CURRENT_STATE.PIPE[1]) {
                pipe_stall(state);
            }
        }
    }
    state->PIPE[4] = CURRENT_STATE.PIPE[3];
}

void process_WB(CPU_State *state) {
    unsigned char control;
    if (!CURRENT_STATE.PIPE[4]) {
        return;
    }

    control = CURRENT_STATE.MEM_WB.CONTROL;
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
    FINISHED_INSTRUCTION_COUNT++;
}

void update_latch(CPU_State *state) {
    int i;

    if (state->PC < MEM_REGIONS[0].start || state->PC >= (MEM_REGIONS[0].start + (NUM_INST * 4))) {
        state->PIPE[0] = 0;
    }

    if (!state->IF_stall && CURRENT_STATE.PIPE[0]) {
        CURRENT_STATE.PC = state->PC;
        CURRENT_STATE.IF_ID.Instr = state->IF_ID.Instr;
        CURRENT_STATE.IF_ID.NPC = state->IF_ID.NPC;
    } else if (state->update_pc) {
        CURRENT_STATE.PC = state->PC;
    }

    if (!state->IF_ID.stall && CURRENT_STATE.PIPE[1]) {
        CURRENT_STATE.ID_EX.NPC = state->ID_EX.NPC;
        CURRENT_STATE.ID_EX.REG1 = state->ID_EX.REG1;
        CURRENT_STATE.ID_EX.REG2 = state->ID_EX.REG2;
        CURRENT_STATE.ID_EX.IMM = state->ID_EX.IMM;
        CURRENT_STATE.ID_EX.FUNC = state->ID_EX.FUNC;
        CURRENT_STATE.ID_EX.RS = state->ID_EX.RS;
        CURRENT_STATE.ID_EX.RT = state->ID_EX.RT;
        CURRENT_STATE.ID_EX.RD = state->ID_EX.RD;
        CURRENT_STATE.ID_EX.CONTROL = state->ID_EX.CONTROL;
    }

    if (CURRENT_STATE.PIPE[2]) {
        CURRENT_STATE.EX_MEM.ALU_OUT = state->EX_MEM.ALU_OUT;
        CURRENT_STATE.EX_MEM.BR_TARGET = state->EX_MEM.BR_TARGET;
        CURRENT_STATE.EX_MEM.WRITE_DATA = state->EX_MEM.WRITE_DATA;
        CURRENT_STATE.EX_MEM.WRITE_REG = state->EX_MEM.WRITE_REG;
        CURRENT_STATE.EX_MEM.CONTROL = state->EX_MEM.CONTROL;
    }

    if (CURRENT_STATE.PIPE[3]) {
        CURRENT_STATE.MEM_WB.ALU_OUT = state->MEM_WB.ALU_OUT;
        CURRENT_STATE.MEM_WB.MEM_OUT = state->MEM_WB.MEM_OUT;
        CURRENT_STATE.MEM_WB.WRITE_REG = state->MEM_WB.WRITE_REG;
        CURRENT_STATE.MEM_WB.CONTROL = state->MEM_WB.CONTROL;
    }

    for (i = 0; i < PIPE_STAGE; i++) {
        CURRENT_STATE.PIPE[i] = state->PIPE[i];
    }

    if (state->flush[2]) {
        CURRENT_STATE.EX_MEM.WRITE_REG = 0;
    }
    if (state->flush[3]) {
        CURRENT_STATE.MEM_WB.WRITE_REG = 0;
    }
    for (i = 0; i < PIPE_STAGE; i++) {
        if (state->flush[i]) {
            CURRENT_STATE.PIPE[i] = 0;
        }
    }
}

/***************************************************************/
/*                                                             */
/* Procedure: process_instruction                              */
/*                                                             */
/* Purpose: Process one instrction                             */
/*                                                             */
/***************************************************************/
void process_instruction(int num_cycles, int nobp_set, int data_fwd_set,
        int pipe_dump_set) {
    CPU_State next_state = {0};
    int i;              // for loop

    process_WB(&next_state);
    process_MEM(&next_state, nobp_set, data_fwd_set);
    process_EX(&next_state, nobp_set, data_fwd_set);
    process_ID(&next_state, nobp_set);
    process_IF(&next_state);

    if (pipe_dump_set) {
        pdump();
    }

    update_latch(&next_state);

    if (FINISHED_INSTRUCTION_COUNT >= num_cycles) {
        RUN_BIT = FALSE;
    }
    if (CURRENT_STATE.PC < MEM_REGIONS[0].start || CURRENT_STATE.PC >= (MEM_REGIONS[0].start + (NUM_INST * 4))) {
        int running = 0;
        for (i = 0; i < PIPE_STAGE; i++) {
            running |= CURRENT_STATE.PIPE[i];
        }
        if (!running) {
            RUN_BIT = FALSE;
        }
    }
}
