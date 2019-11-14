# based on Plasma Cortex assembler, which was based on old NGX series assembler.
print("EmuArch assembler, Copyright 2019 Dylan Brophy")
import sys
args=sys.argv
if args[0].startswith('python'):
    args.pop(0)
args.pop(0)
if len(args) == 0:
    args.append(input("input filename:"))
ofn=args[0]
if '-o' in args:
    i = args.index('-o') + 1
    if i == len(args):
        print("-o must precede a filename.")
        quit()
    ofn = args[i]
try:
    ofn=ofn[:ofn.index('.')]
except:
    pass


regs=['rax','rbx','rcx','rdx','si','di','sp','pc',
      'r0', 'r1', 'r2', 'r3', 'r4', 'r5', 'cnt', 'cr0',
      'f0', 'f1', 'f2', 'f3', 'f4', 'f5', 'f6', 'f7']

file=open(args[0])
filedat=file.read()
file.close()

defined_labels = []

def token_gen(filedat):
    filedat = filedat.replace('\n\r', '\n').replace('\r\n', '\n').replace('\r', '\n')
    lines = filedat.split('\n')
    linenum = 0
    for ln in lines:
        linenum += 1
        tokens = []
        tmp = ''
        inq = False
        inbracket = False
        for i in ln:
            if i == '"':
                inq = not inq
                if tmp != '':
                    tokens.append(tmp)
                    tmp = ''
                if inq:
                    tokens.append('"')
            elif (not inq) and (i == '[' or i == ']'):
                inbracket = not inbracket
                if tmp != '':
                    tokens.append(tmp)
                    tmp = ''
                if inbracket:
                    tmp = '['
            elif (not inq) and (not inbracket):
                if i.isspace() or i == ',':
                    if tmp != '':
                        tokens.append(tmp)
                        tmp = ''
                elif i == ';':
                    break
                else:
                    tmp += i
            else:
                tmp += i
        if tmp != '':
            tokens.append(tmp)
            tmp = ''
        if len(tokens) > 0:
            yield (tokens, linenum)
                    
def to_int(s):
    if (len(s) == 3) and (s[0] == "'") and (s[2] == "'"):
        return ord(s[1])
    try:
        return eval(s)
    except:
        if(s[0]=="'" and s[2]=="'"):
            return ord(s[1])
def is_int(s):
    try: 
        if(s[0]=="'" and s[2]=="'"):
            return True
        return type(eval(s))==int
    except:
        return False
def can_eval(x):
    if("-po" in args):
        return True
    if(x in names.keys()):
        return True
    elif is_int(x):
        return True
    elif x=='$':
        return True
    else:
        return False

error_flag = False
def errormsg(ln, string):
    global error_flag
    error_flag = True
    print("Error on line "+str(ln)+": "+string)
def error_missing_arg(ln, cmd):
    errormsg(ln, "Opcode "+cmd+" missing argument(s)")
def error_too_many_args(ln, cmd):
    errormsg(ln, "Opcode "+cmd+" has too many arguments")
def error_bad_arg(ln, cmd, arg):
    errormsg(ln, "Opcode "+cmd+" not accepting argument '"+arg+"'")
def error_non_reg_arg(ln, cmd, tk):
    errormsg(ln, "Invalid register or instruction not possible: {} {}, [any]".format(cmd, tk))
names = {}
adr = 0
out=[]
lswrt = 0
location = 0
lsop = 0
def emit(s, sz = 8):
    global lswrt, location
    lswrt = location
    if type(s) == int:
        out.append(s & 255)
        location += 1
    else:
        out.append([sz, s])
        location += sz
def wr64(x):
    if type(x)==str:
        emit(x)
        return
    emit(x&255)
    emit((x>>8)&255)
    emit((x>>16)&255)
    emit((x>>24)&255)
    emit((x>>32)&255)
    emit((x>>40)&255)
    emit((x>>48)&255)
    emit(((x>>56)+256)&255)
def wr32(x):
    if type(x)==str:
        emit(x, 4)
        return
    emit(x&255)
    emit((x>>8)&255)
    emit((x>>16)&255)
    emit(((x>>24)+256)&255)
def wr16(x):
    if type(x)==str:
        emit(x, 2)
        return
    emit(x&255)
    emit(((x>>8)+256)&255)
def wr_size(s, x):
    if s == 0:
        wr64(x)
    elif s == 1:
        wr32(x)
    elif s == 2:
        wr16(x)
    elif s == 3:
        emit(x, 1)
    else:
        raise ValueError("[Critical error : flaw in assembler] wr_size requires first argument to be in the range 0-3 (inclusive)")
def parseint(x, allow_ref=True):
    if is_int(x):
        return(to_int(x))
    elif x=='$':
        return(lsop)
    elif allow_ref:
        if x[0] == '.':
            x = lslbl + x
        return x
    else:
        return None
def old_evaluate(x):
    if x in regs:
        errormsg(linenum, "Internal attempt to interpret register as label")
    if is_int(x):
        wr32(to_int(x))
    elif x=='$':
        wr32(location)
    else:
        emit(x)

MOV_OPS = ['movq', 'movd', 'movw', 'movb']
LOAD_OPS = ['lodq', 'lodd', 'lodw', 'lodb']
RAX_CMP_OPS = ['cmpq', 'cmpd', 'cmpw', 'cmpb']
JCON_OPS = ['jz', 'jnz']
FAST_JCON_OPS = ['jnz', 'jz', 'jl', 'jg']
ONE_ARG_MATH_OPS = ['inc', 'dec', 'neg', 'not', 'sqrt', 'tanh']
TWO_ARG_MATH_OPS = ['add', 'sub', 'mul', 'div', 'and', 'or', 'xor', 'cmp',
                    None, None, None, None, 'lsh', 'rsh', 'ras', None]
FLOW_OPS = ['inc', 'dec', 'neg', 'not', 'add', 'sub', 'mul', 'div',
            'and', 'or',  'xor', 'shl', 'shr', 'sar']
FAST_JCON_OPS = ['jnz', 'jz', 'jl', 'jg']

lslbl = ''

for tokens, linenum in token_gen(filedat):
    cmd = tokens[0]
    if cmd[-1] == ':':
        if cmd[0] == '.':
            cmd = lslbl + cmd
        else:
            lslbl = cmd[:-1]
        names[cmd[:-1]] = location
    elif cmd == '.org':
        if len(tokens) < 2 or not is_int(tokens[1]):
            errormsg(linenum, ".org : invalid syntax")
        else:
            location = to_int(tokens[1])
    elif cmd == '.equ':
        if len(tokens) < 3:
            errormsg(linenum, ".equ : invalid syntax")
        else:
            qzx=tokens[1]
            names[qzx] = evaluate(tokens[2])
    elif cmd == 'db':
        i = 1
        while i < len(tokens):
            if tokens[i] == '"':
                for c in eval('"' + tokens[i + 1] + '"'):
                    emit(ord(c))
                i += 1
            else:
                emit(parseint(tokens[i]))
            i += 1
    elif cmd == '.end_flow':
        wr16(0xFF80)
    elif cmd == '.flow':
        if len(tokens) < 3:
            error_missing_arg(linenum, cmd)
        else:
            op = FLOW_OPS.index(tokens[1])
            if op == -1:
                error_bad_arg(linenum, cmd, tokens[1])
            else:
                reg1 = regs[:16].index(tokens[2])
                if len(tokens) > 3:
                    reg2 = regs[:16].index(tokens[3])
                else:
                    reg2 = 0
                if reg1 == -1:
                    error_non_reg_arg(linenum, cmd, tokens[2])
                elif reg2 == -1:
                    error_non_reg_arg(linenum, cmd, tokens[3])
                else:
                    wr16((op << 8) | (reg2 << 4) | reg1)
    elif cmd == "flow":
        if len(tokens) < 2:
            error_missing_arg(linenum, cmd)
        elif len(tokens) > 2:
            error_too_many_args(linenum, cmd)
        else:
            if tokens[1] == 'si':
                emit(0xEB)
                emit(0x01)
            else:
                emit(0xEB)
                emit(0x00)
                wr64(parseint(tokens[1]))
    elif cmd in MOV_OPS:
        if len(tokens) < 3:
            error_missing_arg(linenum, cmd)
        elif len(tokens) > 4:
            error_too_many_args(linenum, cmd)
        else:
            size = MOV_OPS.index(cmd)
            if tokens[-1].startswith('['):
                reg1 = regs[:16].index(tokens[-2])
                if reg1 == -1:
                    error_non_reg_arg(linenum, cmd, tokens[-2])
                else:
                    microtokens = tokens[-1][1:].replace('-', '+-').split('+')
                    if len(microtokens) > 2 or len(microtokens) == 0:
                        error_bad_arg(linenum, cmd, tokens[-1])
                    else:
                        if len(microtokens) == 1:
                            microtokens.append('0') # default offset is 0
                        microtokens[0] = microtokens[0].strip()
                        microtokens[1] = microtokens[1].strip()
                        reg2 = regs[:16].index(microtokens[0])
                        if reg2 == -1:
                            error_non_reg_arg(linenum, cmd, microtokens[0])
                        else:
                            emit(0b00100000 | size)
                            emit((reg1 << 4) | reg2)
                            wr16(parseint(microtokens[1]))
            elif tokens[-2].startswith('['):
                reg1 = regs[:16].index(tokens[-1])
                if reg1 == -1:
                    error_non_reg_arg(linenum, cmd, tokens[-1])
                else:
                    microtokens = tokens[-2][1:].replace('-', '+-').split('+')
                    if len(microtokens) > 2 or len(microtokens) == 0:
                        error_bad_arg(linenum, cmd, tokens[-2])
                    else:
                        if len(microtokens) == 1:
                            microtokens.append('0') # default offset is 0
                        microtokens[0] = microtokens[0].strip()
                        microtokens[1] = microtokens[1].strip()
                        reg2 = regs[:16].index(microtokens[0])
                        if reg2 == -1:
                            error_non_reg_arg(linenum, cmd, microtokens[0])
                        else:
                            emit(0b00100100 | size)
                            emit((reg1 << 4) | reg2)
                            wr16(parseint(microtokens[1]))
            elif tokens[-1] in regs:
                r1 = regs.index(tokens[1])
                r2 = regs.index(tokens[2])
                regcode = ((r1 << 4) & 0xF0) | (r2 & 15)
                opcode = 0b00000000 | (size << 2) | ((r1 >> 3) & 2) | (r1 >> 4)
                emit(opcode)
                emit(regcode)
            else:
                if len(tokens) == 4 and tokens[1] in ['s', 'rot', 'rev', 'rotate', 'swap', 'flip']:
                    s = 0b10000000
                    tokens.pop(1)
                else:
                    s = 0
                reg = regs[:16].index(tokens[1])
                emit(0b10000001)
                emit((size << 5) | s | reg)
                wr_size(size, parseint(tokens[2]))
    elif cmd == 'exx':
        if len(tokens) < 3:
            error_missing_arg(linenum, cmd)
        elif len(tokens) > 3:
            error_too_many_args(linenum, cmd)
        else:
            r1 = regs.index(tokens[1])
            r2 = regs.index(tokens[2])
            regcode = ((r1 << 4) & 0xF0) | (r2 & 15)
            opcode = 0b00010000 | ((r1 >> 3) & 2) | (r1 >> 4)
            emit(opcode)
            emit(regcode)
    elif cmd == 'syscall':
        if len(tokens) < 2:
            error_missing_arg(linenum, cmd)
        else:
            emit(0b10000000)
            wr16(parseint(tokens[1]))
    elif cmd == 'l32':
        if len(tokens) < 3:
            error_missing_arg(linenum, cmd)
        elif len(tokens) > 3:
            error_too_many_args(linenum, cmd)
        else:
            reg = ['rax', 'rbx', 'rcx', 'rdx'].index(tokens[1])
            if reg == -1:
                errormsg(linenum, "Invalid register or instruction not possible: l32 {}, [any]".format(tokens[1]))
            else:
                val = parseint(tokens[2], False)
                if val == None:
                    error_bad_arg(linenum, cmd, tokens[2])
                else:
                    emit(0x18 + reg)
                    wr32(val)
    elif cmd in RAX_CMP_OPS:
        if len(tokens) < 3:
            error_missing_arg(linenum, cmd)
        elif len(tokens) > 3:
            error_too_many_args(linenum, cmd)
        elif tokens[1] != 'rax':
            error_non_reg_arg(linenum, cmd, tokens[1])
        else:
            size = RAX_CMP_OPS.index(cmd)
            val = parseint(tokens[2], size == 0)
            if val == None:
                error_bad_arg(linenum, cmd, tokens[2])
            else:
                emit(0x14 + size)
                wr_size(size, val)
    elif cmd == 'halt':
        emit(0xFF)
    elif cmd == 'jmp':
        if len(tokens) < 2:
            error_missing_arg(linenum, cmd)
        else:
            emit(0b11100000)
            wr64(parseint(tokens[1]))
    elif cmd in JCON_OPS and len(tokens) == 3:
        if not tokens[1] in regs[:16]:
            error_non_reg_arg(linenum, cmd, tokens[1])
        else:
            ctrl = JCON_OPS.index(cmd) << 4
            ctrl |= regs.index(tokens[1])
            emit(0x90)
            emit(ctrl)
            wr64(parseint(tokens[2]))
    elif cmd in FAST_JCON_OPS:
        if len(tokens) < 2:
            error_missing_arg(linenum, cmd)
        elif len(tokens) > 2:
            error_too_many_args(linenum, cmd)
        else:
            emit(0x30 | FAST_JCON_OPS.index(cmd))
            wr64(parseint(tokens[1]))
    elif cmd in LOAD_OPS:
        size = LOAD_OPS.index(cmd)
        if len(tokens) > 1:
            error_too_many_args(linenum, cmd)
        else:
            emit(0b11000000 | (size << 2))
    elif cmd == 'call':
        if len(tokens) < 2:
            error_missing_arg(linenum, cmd)
        elif len(tokens) > 2:
            error_too_many_args(linenum, cmd)
        else:
            val = parseint(tokens[1])
            emit(0xA2)
            wr64(val)
    elif cmd == 'ret':
        if len(tokens) > 1:
            error_too_many_args(linenum, cmd)
        else:
            emit(0xA0)
            emit(0x07)
    elif cmd == 'pop':
        if len(tokens) < 2:
            error_missing_arg(linenum, cmd)
        elif len(tokens) > 2:
            error_too_many_args(linenum, cmd)
        elif not tokens[1] in regs:
            error_non_reg_arg(linenum, cmd, tokens[1])
        else:
            emit(0xA0)
            emit(regs.index(tokens[1]))
    elif cmd == 'push':
        if len(tokens) < 2:
            error_missing_arg(linenum, cmd)
        elif len(tokens) > 2:
            error_too_many_args(linenum, cmd)
        elif not tokens[1] in regs:
            error_non_reg_arg(linenum, cmd, tokens[1])
        else:
            emit(0xA1)
            emit(regs.index(tokens[1]))
    elif cmd in ONE_ARG_MATH_OPS:
        if len(tokens) < 2:
            error_missing_arg(linenum, cmd)
        elif len(tokens) > 2:
            error_too_many_args(linenum, cmd)
        elif not tokens[1] in regs:
            error_non_reg_arg(linenum, cmd, tokens[1])
        else:
            emit(0x40 | ONE_ARG_MATH_OPS.index(cmd))
            emit(regs.index(tokens[1]))
    elif cmd in TWO_ARG_MATH_OPS:
        if len(tokens) < 3:
            error_missing_arg(linenum, cmd)
        elif len(tokens) > 3:
            error_too_many_args(linenum, cmd)
        elif not tokens[1] in regs:
            error_non_reg_arg(linenum, cmd, tokens[1])
        else:
            opid = TWO_ARG_MATH_OPS.index(cmd)
            reg1 = regs.index(tokens[1])
            if tokens[2] in regs:
                # [math] r1, r2
                reg2 = regs.index(tokens[2])
                if (reg2 & 16) != (reg1 & 16):
                    errormsg(linenum, cmd+": Register types do not match")
                else:
                    regclass = reg2 >> 4
                    if regclass == 1 and (opid > 7 or TWO_ARG_MATH_OPS[opid + 8] != None):
                        errormsg(linenum, cmd+": Instruction cannot use floats")
                    else:
                        reg2 &= 15
                        reg1 &= 15
                        emit(0x50 | opid | (regclass << 3))
                        emit(reg2 | (reg1 << 4))
            else:
                # [math], r1, const
                if reg1 > 15:
                    errormsg(linenum, cmd+": Constants cannot operate on floats")
                else:
                    emit(0x60 | opid)
                    emit(reg1)
                    wr_size(reg1>>3, parseint(tokens[2]))
    elif cmd == 'strcmp':
        if len(tokens) < 3:
            error_missing_arg(linenum, cmd)
        elif len(tokens) > 3:
            error_too_many_args(linenum, cmd)
        elif tokens[1] != 'di':
            error_bad_arg(linenum, cmd, tokens[1])
        elif tokens[2] != 'si':
            error_bad_arg(linenum, cmd, tokens[2])
        else:
            emit(0xEC)
            emit(0x20)
    elif cmd == 'strcpy':
        if len(tokens) < 3:
            error_missing_arg(linenum, cmd)
        elif len(tokens) > 3:
            error_too_many_args(linenum, cmd)
        elif tokens[1] != 'di':
            error_bad_arg(linenum, cmd, tokens[1])
        elif tokens[2] != 'si':
            error_bad_arg(linenum, cmd, tokens[2])
        else:
            emit(0xEC)
            emit(0x10)
    elif cmd == 'strcat':
        if len(tokens) < 3:
            error_missing_arg(linenum, cmd)
        elif len(tokens) > 3:
            error_too_many_args(linenum, cmd)
        elif tokens[1] != 'di':
            error_bad_arg(linenum, cmd, tokens[1])
        elif tokens[2] != 'si':
            error_bad_arg(linenum, cmd, tokens[2])
        else:
            emit(0xEC)
            emit(0x08)
    elif cmd == 'strlen':
        if len(tokens) < 2:
            error_missing_arg(linenum, cmd)
        elif len(tokens) > 2:
            error_too_many_args(linenum, cmd)
        elif tokens[1] != 'si':
            error_bad_arg(linenum, cmd, tokens[1])
        else:
            emit(0xEC)
            emit(0x00)
    elif cmd == 'strf':
        if len(tokens) < 3:
            error_missing_arg(linenum, cmd)
        elif len(tokens) > 3:
            error_too_many_args(linenum, cmd)
        elif tokens[1] != 'si':
            error_bad_arg(linenum, cmd, tokens[1])
        elif tokens[2] != 'rax':
            error_bad_arg(linenum, cmd, tokens[2])
        else:
            emit(0xEC)
            emit(0x18)
    elif cmd == 'strskip':
        if len(tokens) < 3:
            error_missing_arg(linenum, cmd)
        elif len(tokens) > 3:
            error_too_many_args(linenum, cmd)
        elif tokens[1] != 'si':
            error_bad_arg(linenum, cmd, tokens[1])
        elif tokens[2] != 'rax':
            error_bad_arg(linenum, cmd, tokens[2])
        else:
            emit(0xEC)
            emit(0x30)
    elif cmd == 'strfi':
        if len(tokens) < 4:
            error_missing_arg(linenum, cmd)
        elif len(tokens) > 4:
            error_too_many_args(linenum, cmd)
        elif tokens[1] != 'si':
            error_bad_arg(linenum, cmd, tokens[1])
        elif tokens[2] != 'rax':
            error_bad_arg(linenum, cmd, tokens[2])
        elif tokens[3] in regs:
            error_bad_arg(linenum, cmd, tokens[3])
        else:
            emit(0xEC)
            emit(0x28)
            wr64(parseint(tokens[3]))
    elif cmd in FAST_JCON_OPS:
        if len(tokens) < 2:
            error_missing_arg(linenum, cmd)
        elif len(tokens) > 2:
            error_too_many_args(linenum, cmd)
        elif tokens[1] in regs:
            error_bad_arg(linenum, cmd, tokens[1])
        else:
            emit(0x30 | FAST_JCON_OPS.index(cmd))
            wr64(parseint(tokens[3]))
    else:
        errormsg(linenum, "Unknown opcode '"+cmd+"'")
    lsop = location
if not error_flag:
    if '-eo' in args:
        # export object file
        with open(ofn + '.eo', 'wb') as f:
            f.write(len(names.keys()).to_bytes(4, 'little'))
            for k, v in names.items():
                f.write(v.to_bytes(8, 'little'))
                f.write(k.encode())
                f.write(b'\x00')
            f.flush()
            chunks = []
            c_chunk = []
            while len(out) > 0:
                if type(out[0]) == int:
                    c_chunk.append(out[0])
                else:
                    chunks.append([bytes(c_chunk), bytes([out[0][0]]) + out[0][1].encode()])
                    c_chunk = []
                out = out[1:]
            if len(c_chunk) != 0:
                chunks.append([bytes(c_chunk), b''])
            for i in chunks:
                f.write(len(i[0]).to_bytes(4, 'little'))
                f.write(i[0])
                f.write(i[1])
                f.write(b'\x00')
    else:
        conv = out
        out = b''
        for i in conv:
            if type(i)==int:
                out += bytes([i])
            else:
                try:
                    out += names[i[1]].to_bytes(i[0], 'little')
                except KeyError:
                    errormsg('?', "unknown symbol '"+i[1]+"'")
        if not error_flag:
            with open(ofn + '.bin', 'wb') as f:
                f.write(out)
