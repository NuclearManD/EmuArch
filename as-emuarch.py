# based on Plasma Cortex assembler, which was based on old NGX series assembler.
print("EmuArch assembler, Copyright 2019 Dylan Brophy")
import sys
args=sys.argv
if(len(args)==1):
    args.append(input("input filename:"))
    args.pop(0)
ofn=args[0]
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
        for i in ln:
            if i == '"':
                inq = not inq
                if tmp != '':
                    tokens.append(tmp)
                    tmp = ''
                if inq:
                    tokens.append('"')
            elif not inq:
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
def errormsg(ln, string):
    print("Error on line "+str(ln)+": "+string)
def error_missing_arg(ln, cmd):
    errormsg(ln, "Opcode "+cmd+" missing argument(s)")
def error_too_many_args(ln, cmd):
    errormsg(ln, "Opcode "+cmd+" has too many arguments")
def error_bad_arg(ln, cmd, arg):
    errormsg(ln, "Opcode "+cmd+" not accepting argument '"+arg+"'")
names = {}
adr = 0
out=[]
lswrt = 0
location = 0
lsop = 0
def emit(s):
    global lswrt, location
    lswrt = location
    if type(s) == int:
        out.append(s & 255)
        location += 1
    else:
        out.append(s)
        location += 8
def wr64(x):
    if type(x)==str:
        emit(x)
        return
    emit(((x>>56)+256)&255)
    emit((x>>48)&255)
    emit((x>>40)&255)
    emit((x>>32)&255)
    emit((x>>24)&255)
    emit((x>>16)&255)
    emit((x>>8)&255)
    emit(x&255)
def wr32(x : int):
    emit(((x>>24)+256)&255)
    emit((x>>16)&255)
    emit((x>>8)&255)
    emit(x&255)
def wr16(x : int):
    emit(((x>>8)+256)&255)
    emit(x&255)
def wr_size(s, x):
    if s == 0:
        wr64(x)
    elif s == 1:
        wr32(x)
    elif s == 2:
        wr16(x)
    elif s == 3:
        emit(x)
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

lslbl = ''

for tokens, linenum in token_gen(filedat):
    cmd = tokens[0]
    if cmd[-1] == ':':
        if cmd[0] == '.':
            cmd = lslbl + cmd
        else:
            lslbl = cmd
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
                for c in tokens[i + 1]:
                    emit(ord(c))
                i += 1
            else:
                emit(parseint(tokens[i]))
            i += 1
    elif cmd in MOV_OPS:
        if len(tokens) < 3:
            error_missing_arg(linenum, cmd)
        elif len(tokens) > 4:
            error_too_many_args(linenum, cmd)
        else:
            size = MOV_OPS.index(cmd)
            if tokens[-1] in regs:
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
            errormsg(linenum, "Invalid register or instruction not possible: {} {}, [any]".format(cmd, tokens[1]))
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
    elif cmd in LOAD_OPS:
        size = LOAD_OPS.index(cmd)
        if len(tokens) > 1:
            error_too_many_args(linenum, cmd)
        else:
            emit(0b11000000 | (size << 2))
    else:
        errormsg(linenum, "Unknown opcode '"+cmd+"'")
    lsop = location
if '-eo' in args:
    # export object file
    with open(ofn + '.eo', 'wb') as f:
        f.write(len(names.keys()).to_bytes(4, 'big'))
        for k, v in names.items():
            f.write(v.to_bytes(8, 'big'))
            f.write(k.encode())
            f.write(b'\x00')
        f.flush()
        chunks = []
        c_chunk = []
        while len(out) > 0:
            if type(out[0]) == int:
                c_chunk.append(out[0])
            else:
                chunks.append([bytes(c_chunk), out[0].encode()])
                c_chunk = []
            out = out[1:]
        if len(c_chunk) != 0:
            chunks.append([bytes(c_chunk), b''])
        for i in chunks:
            f.write(len(i[0]).to_bytes(4, 'big'))
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
            out += names[i].to_bytes(8, 'big')
    with open(ofn + '.bin', 'wb') as f:
        f.write(out)
