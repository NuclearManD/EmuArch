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
print("Assembling "+args[1]+"...")
file=open(args[1])
filedat=file.read()
file.close()
split=False
com=False
tmp=""
tokens=[]
tklines=[]
linenum=1
for i in filedat:
    if i==';':
        com=True
    elif(com):
        if i=='\n' or i=='\r':
            com=False
    elif i=='"':
        split=not split
        if(tmp!=""):
            tokens.append(tmp)
            tklines.append(linenum)
        tmp=""
        if split:
            tokens.append('"')
            tklines.append(linenum)
    elif (i.isspace() or i==',')and tmp!="'" and not split:
        if(tmp!=""):
            tokens.append(tmp)
            tklines.append(linenum)
        tmp=""
    else:
        tmp+=i
    if i=='\n' or i=='\r':
        linenum+=1
if len(tmp)>0:
    tokens.append(tmp)
    tklines.append(linenum)

regs=['ax','bx','cx','dx','si','di','sp','pc']
math_ops={'add':0x80, 'sub':0x81, 'mul':0x82, 'div':0x83, 'xor':0x85, 'and':0x86, 'or':0x87}
math_ops_onearg={'shl':0xC0,'shr':0xC1,'sar':0xC2,'abs':0xC3,'not':0xC4,'inc':0xC6,'dec':0xC7}
names={}
glbls=[]
sizes={}

location=0
i=0
def to_int(s):
    #exec("y="+s)
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
def errormsg(string):
    print("Error on line "+str(tklines[i])+": "+string)
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
    elif("-po" in args):
        emit(x)
    elif(x in names.keys()):
        wr32(names[x])
    else:
        errormsg("'"+tokens[i]+"' is not a valid number, identifier, or symbol.")
