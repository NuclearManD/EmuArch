# based on Plasma Cortex assembler, which was based on old NGX series assembler.
print("EmuArch assembler, Copyright 2019 Dylan Brophy")
import sys
args=sys.argv
if(len(args)==1):
    args.append(input("input filename:"))
    sys.argv.append('-po')
ofn=args[1]
try:
    ofn=ofn[:ofn.index('.')]
except:
    pass

regs=['rax','rbx','rcx','rdx','rsi','rdi','rsp','rpc',
      'r0', 'r1', 'r2', 'r3', 'r4', 'r5', 'cnt', 'cr0',
      'f0', 'f1', 'f2', 'f3', 'f4', 'f5', 'f6', 'f7']

file=open(args[1])
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
    print("Error on line "+str(ln)+": Opcode "+cmd+" missing argument(s)")
names = {}
adr = 0
out=[]
lswrt=0
location = 0
def emit(s):
    global lswrt
    out.append(s)
    lswrt=location
def wr64(x):
    emit(((x>>56)+256)&255)
    emit((x>>48)&255)
    emit((x>>40)&255)
    emit((x>>32)&255)
    emit((x>>24)&255)
    emit((x>>16)&255)
    emit((x>>8)&255)
    emit(x&255)
def wr32(x):
    emit(((x>>24)+256)&255)
    emit((x>>16)&255)
    emit((x>>8)&255)
    emit(x&255)
def wr16(x):
    emit(((x>>8)+256)&255)
    emit(x&255)
def parseint(x, allow_ref=True):
    if is_int(x):
        return(to_int(x))
    elif x=='$':
        return(location)
    elif("-po" in args and allow_ref):
        return (x)
    elif(x in names.keys()):
        return (names[x])
    else:
        errormsg("'"+tokens[i]+"' is not a valid number, identifier, or symbol.")
def evaluate(x):
    if x in regs:
        errormsg("Internal attempt to interpret register as label")
    if is_int(x):
        wr32(to_int(x))
    elif x=='$':
        wr32(location)
    else:
        emit(x)
for tokens, linenum in token_gen(filedat):
    cmd = tokens[0]
    if cmd == '.org':
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
    elif cmd == 'movb':
        if len(tokens) < 3:
            error_missing_arg(linenum, cmd)
        else:
            if len(tokens) == 4 and tokens.pop(1) in ['s', 'rot', 'rev', 'rotate', 'swap', 'flip']:
                s = 0b10000000
            else:
                s = 0
            reg = regs[:16].index(tokens[1])
            emit(0b10000001)
            emit(0b01100000 | s | reg)
            emit(parseint(tokens[2]))
    elif cmd == 'syscall':
        if len(tokens) < 2:
            error_missing_arg(linenum, cmd)
        else:
            emit(0b10000000)
            wr16(parseint(tokens[1]))
    elif cmd == 'halt':
        emit(0xFF)
