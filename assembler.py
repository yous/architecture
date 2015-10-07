import sys
import getopt
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

def parse(filename):
    try:
        f = open(filename, 'r')
        lines = f.readlines()
    except IOError, e:
        print 'IO error(%d): %s' % (e.errno, e.strerror)
    else:
        f.close()

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

    parse(args[0])

if __name__ == '__main__':
    main(sys.argv[1:])
