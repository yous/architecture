import sys
import getopt
import os
import re

def bin_str(n, bit):
    return ''.join(str(1 & n >> i) for i in range(bit)[::-1])

def format_reg(reg):
    return bin_str(int(reg[1:]), 5)

def format_imm(imm, bit):
    if not isinstance(imm, int):
        if imm.startswith('0x') or imm.startswith('-0x'):
            imm = int(imm, 16)
        else:
            imm = int(imm)
    return bin_str(imm, bit)

class Instruction:
    class R:
        FORMAT_MAP = {
            'addu': '000000%s%s%s00000100001',
            'and': '000000%s%s%s00000100100',
            'jr': '000000%s000000000000000001000',
            'nor': '000000%s%s%s00000100111',
            'or': '000000%s%s%s00000100101',
            'sltu': '000000%s%s%s00000101011',
            'sll': '00000000000%s%s%s000000',
            'srl': '00000000000%s%s%s000010',
            'subu': '000000%s%s%s00000100011'
        }

        def __init__(self, tokens):
            self.tokens = tokens

        def __str__(self):
            instr = self.tokens[0]
            format_str = self.FORMAT_MAP[instr]
            if instr == 'jr':
                rs = self.tokens[1]
                return format_str % format_reg(rs)
            elif instr == 'sll' or instr == 'srl':
                rd, rt, imm = self.tokens[1:]
                return format_str % (
                    format_reg(rt),
                    format_reg(rd),
                    format_imm(imm, 5)
                )
            else:
                rd, rs, rt = self.tokens[1:]
                return format_str % (
                    format_reg(rs),
                    format_reg(rt),
                    format_reg(rd)
                )

    class I:
        FORMAT_MAP = {
            'addiu': '001001%s%s%s',
            'andi': '001100%s%s%s',
            'beq': '000100%s%s%s',
            'bne': '000101%s%s%s',
            'lui': '00111100000%s%s',
            'lw': '100011%s%s%s',
            'ori': '001101%s%s%s',
            'sltiu': '001011%s%s%s',
            'sw': '101011%s%s%s'
        }

        def __init__(self, tokens):
            self.tokens = tokens

        def __str__(self):
            instr = self.tokens[0]
            format_str = self.FORMAT_MAP[instr]
            if instr == 'lui':
                rt, imm = self.tokens[1:]
                return format_str % (
                    format_reg(rt),
                    format_imm(imm, 16)
                )
            elif instr == 'beq' or instr == 'bne':
                rs, rt, imm = self.tokens[1:]
                return format_str % (
                    format_reg(rs),
                    format_reg(rt),
                    format_imm(imm / 4, 16)
                )
            elif instr == 'lw' or instr == 'sw':
                rt, rs_with_offset = self.tokens[1:]
                offset, rs, _ = re.split('[()]', rs_with_offset)
                return format_str % (
                    format_reg(rs),
                    format_reg(rt),
                    format_imm(offset, 16)
                )
            else:
                rt, rs, imm = self.tokens[1:]
                return format_str % (
                    format_reg(rs),
                    format_reg(rt),
                    format_imm(imm, 16)
                )

    class J:
        FORMAT_MAP = {
            'j': '000010%s',
            'jal': '000011%s'
        }

        def __init__(self, tokens):
            self.tokens = tokens

        def __str__(self):
            instr = self.tokens[0]
            format_str = self.FORMAT_MAP[instr]
            target = self.tokens[1] / 4
            return format_str % format_imm(target, 26)

    INSTRUCTION_MAP = {
        'addiu': I,
        'addu': R,
        'and': R,
        'andi': I,
        'beq': I,
        'bne': I,
        'j': J,
        'jal': J,
        'jr': R,
        'lui': I,
        'lw': I,
        'nor': R,
        'or': R,
        'ori': I,
        'sltiu': I,
        'sltu': R,
        'sll': R,
        'srl': R,
        'sw': I,
        'subu': R
    }

    def __init__(self, tokens):
        self.tokens = tokens

    def __str__(self):
        instr = self.tokens[0]
        return str(self.INSTRUCTION_MAP[instr](self.tokens))

class Assembler:
    TEXT_ADDRESS = 0x400000
    DATA_ADDRESS = 0x10000000

    def __init__(self):
        self.labels = {}
        self.instructions = []
        self.data = []

    def assemble(self, filename):
        try:
            f = open(filename, 'r')
        except IOError, e:
            print 'IO error(%d): %s' % (e.errno, e.strerror)
            sys.exit(2)
        else:
            self.parse(f.readlines())
            self.resolve_label()
            self.save('%s.o' % os.path.splitext(filename)[0])

    def parse(self, lines):
        mode = None
        address = None
        for line in lines:
            tokens = line.replace(',', '').split()
            for i in range(len(tokens)):
                token = tokens[i]
                if token.endswith(':'):
                    self.labels[token[:-1]] = address
                elif token == '.data':
                    mode = 'data'
                    address = self.DATA_ADDRESS
                elif token == '.text':
                    mode = 'text'
                    address = self.TEXT_ADDRESS
                elif mode == 'data':
                    value = tokens[i + 1]
                    if value.startswith('0x') or value.startswith('-0x'):
                        value = int(value, 16)
                    else:
                        value = int(value)
                    self.data.append(value)
                    address += 4
                    break
                elif mode == 'text':
                    if token == 'la':
                        label = tokens[i + 2]
                        label_address = self.labels[label]
                        if label_address & 0xFFFF == 0:
                            self.instructions.append([
                                address,
                                'lui', tokens[i + 1], label_address >> 16])
                        else:
                            self.instructions.append([
                                address,
                                'lui', tokens[i + 1], label_address >> 16])
                            address += 4
                            self.instructions.append([
                                address,
                                'ori', tokens[i + 1], tokens[i + 1],
                                label_address & 0xFFFF])
                    else:
                        self.instructions.append([address] + tokens[i:])
                    address += 4
                    break

    def resolve_label(self):
        instructions = self.instructions
        for i in range(len(instructions)):
            instruction = instructions[i]
            address = instruction[0]
            instr = instruction[1]
            if instr == 'beq' or instr == 'bne':
                label = instruction[4]
                offset = self.labels[label] - address - 4
                self.instructions[i][4] = offset
            elif instr == 'j' or instr == 'jal':
                label = instruction[2]
                address = self.labels[label]
                self.instructions[i][2] = address

    def save(self, filename):
        try:
            f = open(filename, 'w')
        except IOError, e:
            print 'IO error(%d): %s' % (e.errno, e.strerror)
            sys.exit(2)
        else:
            f.write(self.generate_output())
            f.close()

    def generate_output(self):
        text_section = ''.join(map(
            lambda tokens: str(Instruction(tokens[1:])),
            self.instructions))
        data_section = ''.join(map(
            lambda x: bin_str(x, 32),
            self.data))
        return (bin_str(len(text_section) / 8, 32) +
                bin_str(len(data_section) / 8, 32) +
                text_section + data_section)

def usage():
    print 'usage: %s <assembly file>' % sys.argv[0]

def main(argv):
    try:
        opts, args = getopt.getopt(argv, '')
    except getopt.GetoptError, e:
        usage()
        sys.exit(2)

    for opt, arg in opts:
        usage()
        sys.exit(2)

    if len(args) != 1:
        usage()
        sys.exit(2)

    Assembler().assemble(args[0])

if __name__ == '__main__':
    main(sys.argv[1:])
