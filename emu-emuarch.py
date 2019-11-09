MAX64 = 2**64 - 1
MAX32 = 2**32 - 1

SYS_MALLOC  = 0
SYS_FREE    = 1
SYS_PUTCHAR = 2
SYS_PRINTF  = 3

OFFSET_MASK =   0xFFFFFFFF00000000
ADR_MASK =      0x00000000FFFFFFFF
RAM_OFFSET  =   0x100000000
CODE_OFFSET =   0x00

CPU_FEATURES = 0x300 # FPU, 64-bit mode

ram = [255] * (32 * 1024)
mem_ptrs = []
heap_top = 0

# Load RAX with 'H' in a creative way
# code = [0b10000001, 0b01000001, 0xFF, ord('H'), 0b00001100, 0b00000001, 0xFF]

# Test movd 1, rax, 0xf00f1337
# code = [0b10000001, 0b10100000, 0xf0, 0x0f, 0x13, 0x37]

def syscall(val, cpu):
    global heap_top, ram, ram_ptrs
    if val == SYS_MALLOC:
        qty = cpu.reg_set_0[0]
        if qty > len(ram) - heap_top:
            throw(cpu, -1)
        out = heap_top
        mem_ptrs.append([out, qty])
        heap_top += qty
        cpu.reg_set_0[0] = out | RAM_OFFSET
    elif val == SYS_FREE:
        ptr = cpu.reg_set_0[0]
        for n in range(len(mem_ptrs)):
            block = mem_ptrs[n]
            if ptr == block[0]:
                qty = block[1]
                if n == len(mem_ptrs) - 1:
                    if len(mem_ptrs) > 1:
                        heap_top = sum(mem_ptrs[n - 1])
                    else:
                        heap_top = 0
                mem_ptrs.pop(n)
                cpu.reg_set_0[0] = qty
                return
        throw(cpu, -2)
    elif val == SYS_PUTCHAR:
        print(chr(cpu.reg_set_0[0]&255), end = '')
    else:
        throw(cpu, -3)
def size_to_bytes(n):
    return 2**(3 - n)

def size_to_mask(n):
    return (256**size_to_bytes(n)) - 1

def reg_set_size(set):
    if set == 0 or set == 3:
        return 0 # set 0 and 3 have 64 bits per register
    else:
        return 1

def reg_set_bits(set):
    if set == 0 or set == 3:
        return 64 # set 0 and 3 have 64 bits per register
    else:
        return 32
def reg_set_unsigned_max(set):
    return 2**reg_set_bits(set) - 1
def sign(b, n):
    if n > 2**(b - 1):
        n = -(2**b - n)
    return n
class emuarch_cpu:
    def __init__(self, code, stacksize = 8192):
        self.code = code
        self.reg_set_0 = [0] * 8
        self.reg_set_1 = [0] * 8
        self.reg_set_2 = [0.0] * 8

        self.reg_set_0[6] = (len(ram) - 1) | RAM_OFFSET

        self.exited = False
    def getreg(self, regid):
        num = regid & 7
        set = regid >> 3
        if set == 0:
            return self.reg_set_0[num]
        elif set == 1:
            if num == 7:
                # CR0 bits 16-31 cannot be changed
                return (self.reg_set_1[7] & 0xFFFF) | CPU_FEATURES
            return self.reg_set_1[num]
        elif set == 2:
            return self.reg_set_2[num]
        else:
            raise ValueError("Invalid register set {}".format(set))
    def loadreg(self, set, num, val = None):
        if val == None:
            val = num
            num = set & 7
            set = set >> 3
        if num >= 8:
            raise ValueError("Invalid register id {}:{}".format(set, val))
        if set == 0:
            self.reg_set_0[num] = int(val) & MAX64
        elif set == 1:
            self.reg_set_1[num] = int(val) & MAX32
        elif set == 2:
            self.reg_set_2[num] = float(val)
        else:
            raise ValueError("Invalid register set {}".format(set))
    def readbyte(self, addr):
        offset = addr & OFFSET_MASK
        addr = addr & ADR_MASK
        if offset == RAM_OFFSET:
            if addr >= len(ram):
                return 0xFF
            return ram[addr]
        elif offset == CODE_OFFSET:
            if addr >= len(self.code):
                return 0xFF
            return self.code[addr] & 255
        else:
            return 0x00
    def readword(self, addr):
        return self.readbyte(addr)  * 0x000000100 + self.readbyte(addr + 1)
    def readdword(self, addr):
        return self.readword(addr)  * 0x000010000 + self.readword(addr + 2)
    def readqword(self, addr):
        return self.readdword(addr) * 0x100000000 + self.readdword(addr + 4)
    def writebyte(self, addr, val):
        o = addr
        offset = addr & OFFSET_MASK
        addr = addr & ADR_MASK
        val &= 255
        if offset == RAM_OFFSET:
            if addr < len(ram):
                ram[addr] = val
        elif offset == CODE_OFFSET:
            print("WARN: Attempt to write code at", hex(o))
            if addr < len(self.code):
                self.code[addr] = val
    def writeword(self, addr, val):
        self.writebyte(addr, val >> 8)
        self.writebyte(addr + 1, val)
    def writedword(self, addr, val):
        self.writeword(addr, val >> 16)
        self.writeword(addr + 2, val)
    def writeqword(self, addr, val):
        self.writedword(addr, val >> 32)
        self.writedword(addr + 4, val)
    def writesize(self, size, addr, val):
        if size == 0:
            self.writeqword(addr, val)
        elif size == 1:
            self.writedword(addr, val)
        elif size == 2:
            self.writeword(addr, val)
        elif size == 3:
            self.writebyte(addr, val)
        else:
            raise ValueError("Invalid size - internal emulator error")
    def readsize(self, size, addr):
        if size == 0:
            return self.readqword(addr)
        elif size == 1:
            return self.readdword(addr)
        elif size == 2:
            return self.readword(addr)
        elif size == 3:
            return self.readbyte(addr)
        else:
            raise ValueError("Invalid size - internal emulator error")
    def writereg(self, size, regid, val):
        mask = size_to_mask(size)
        mask2 = mask ^ reg_set_unsigned_max(regid)
        self.loadreg(regid, (self.getreg(regid) & mask2) | (val & mask))
    def pushbyte(self, val):
        adr = self.getreg(6) - 1
        self.writebyte(adr + 1, val)
        self.loadreg(6, adr)
    def pushword(self, val):
        adr = self.getreg(6) - 2
        self.writeword(adr + 1, val)
        self.loadreg(6, adr)
    def pushdword(self, val):
        adr = self.getreg(6) - 4
        self.writedword(adr + 1, val)
        self.loadreg(6, adr)
    def pushqword(self, val):
        adr = self.getreg(6) - 8
        self.writeqword(adr + 1, val)
        self.loadreg(6, adr)
    def popbyte(self):
        adr = self.getreg(6)
        out = self.readbyte(adr + 1)
        self.loadreg(6, adr + 1)
        return out
    def popword(self):
        adr = self.getreg(6)
        out = self.readword(adr + 1)
        self.loadreg(6, adr + 2)
        return out
    def popdword(self):
        adr = self.getreg(6)
        out = self.readdword(adr + 1)
        self.loadreg(6, adr + 4)
        return out
    def popqword(self):
        adr = self.getreg(6)
        out = self.readqword(adr + 1)
        self.loadreg(6, adr + 8)
        return out
    def popsize(self, size):
        if size == 0:
            return self.popqword()
        elif size == 1:
            return self.popdword()
        elif size == 2:
            return self.popword()
        elif size == 3:
            return self.popbyte()
        else:
            raise ValueError("Invalid size - internal emulator error")
    def pushsize(self, size, val):
        if size == 0:
            return self.pushqword(val)
        elif size == 1:
            return self.pushdword(val)
        elif size == 2:
            return self.pushword(val)
        elif size == 3:
            return self.pushbyte(val)
        else:
            raise ValueError("Invalid size - internal emulator error")
    def compare(self, a, b):
        r = a - b
        flags = 0
        if r < 0:
            flags |= 1 # less than
        elif r > 0:
            flags |= 2 # more than
        else:
            flags |= 4 # equal
        self.reg_set_1[7] = (self.reg_set_1[7] & 0xFFFFFFF0) | flags
    def alu(self, op, nbits, a, b):
        #print(a, op, b, " | ", nbits)
        if op == 0:
            res = a + 1
        elif op == 1:
            res = a - 1
        elif op == 2:
            res = -a
        elif op == 3:
            res = ((2**nbits) - 1) ^ a
        elif op == 4:
            if type(a)==int:
                res = int(65536*math.sqrt(a/65536.0))
            else:
                res = math.sqrt(a)
        elif op == 5:
            if type(a)==int:
                res = int(65536*math.tanh(a/65536.0))
            else:
                res = math.tanh(a)
        elif op == 16 or op == 24:
            res = a + b
        elif op == 17 or op == 25:
            res = a - b
        elif op == 18 or op == 26:
            res = a * b
        elif op == 19 or op == 27:
            res = a / b
        elif op == 20:
            res = a & b
        elif op == 21:
            res = a | b
        elif op == 22:
            res = a ^ b
        elif op == 23 or op == 31:
            compare(a, b)
            return a
        elif op == 28:
            res = a << b
        elif op == 29:
            if a < 0:
                a = ((2**nbits) - 1) + a
            res = a >> b
        elif op == 30:
            res = (a >> b)
            if a >> (nbits - 1) == 1:
                res |= ((2**nbits - 1) << (nbits - b))
            
        self.compare(res, 0)
        return res
    def step(self):
        if self.exited:
            return -1
        pc = self.getreg(7)

        opcode = self.readbyte(pc)
        pc += 1
        
        spec1 = opcode & 0x80
        spec2 = opcode & 0x40
        spec3 = opcode & 0x20
        spec4 = opcode & 0x10

        # size applies for some operations. Ex: movw rax, rbx
        size = (opcode>>2) & 3

        if not spec1:
            # basic operations
            if not spec2:
                # load/store
                if not spec3:
                    # 0b000xxxxx
                    if not spec4:
                        # register-register load/store (mov[s] r1, r2)
                        reg_raw = self.readbyte(pc)
                        pc += 1
                        reg1 = ((opcode & 2) << 3) | (reg_raw >> 4)
                        reg2 = ((opcode & 1) << 4) | (reg_raw & 15)
                        data = self.getreg(reg2) & size_to_mask(size)
                        data |= self.getreg(reg1) & (reg_set_unsigned_max(reg1) ^ size_to_mask(size))
                        self.loadreg(reg1, data)
                    elif size == 0:
                        # register-register exchange (exx r1, r2)
                        reg_raw = self.readbyte(pc)
                        pc += 1
                        reg1 = ((opcode & 2) << 3) | (reg_raw >> 4)
                        reg2 = ((opcode & 1) << 4) | (reg_raw & 15)
                        data = self.getreg(reg2)
                        tmp = self.getreg(reg1)
                        self.loadreg(reg2, tmp)
                        self.loadreg(reg1, data)
                    elif size == 1:
                        size = opcode & 3
                        self.compare(self.getreg(0) & size_to_mask(size), self.readsize(size, pc))
                        pc += size_to_bytes(size)
                    elif size == 2:
                        # l32 r, #
                        data = self.readdword(pc)
                        pc += 4
                        self.writereg(1, opcode & 3, data)
                    else:
                        # reserved
                        pass
                else:
                    # 0b001xxxxx
                    if not spec4:
                        # 0b0010xxxx
                        if size == 0:
                            # mov[s] r1i, [r2i + **]
                            size = opcode & 3

                            rawreg = self.readbyte(pc)
                            reg1 = rawreg >> 4
                            reg2 = rawreg & 15
                            pc += 1

                            address = sign(16, self.readword(pc)) + self.getreg(reg2)
                            pc += 2
                            self.writereg(size, reg1, self.readsize(size, address))
                            if reg1 == 7:
                                pc = self.getreg(7)
                        elif size == 1:
                            # mov[s] [r2i + **], r1i
                            size = opcode & 3

                            rawreg = self.readbyte(pc)
                            reg1 = rawreg >> 4
                            reg2 = rawreg & 15
                            pc += 1

                            address = sign(16, self.readword(pc)) + self.getreg(reg2)
                            self.writesize(size, address, self.getreg(reg1))
                            pc += 2
                    else:
                        # reserved
                        pass
            else:
                # 0b01xxxxxx
                # math operations
                klass = opcode >> 4
                if klass == 4:
                    # 0b0100xxxx
                    # Single argument register mathematics
                    rawreg = self.readbyte(pc)
                    pc += 1
                    reg = rawreg & 0x1F
                    self.loadreg(reg, self.alu(opcode & 15, reg_set_bits(reg>>3), self.getreg(reg), 0))
                elif klass == 5:
                    # 0b0101xxxx
                    # Register - register dual argument mathematics
                    reg_raw = self.readbyte(pc)
                    pc += 1

                    operation = opcode & 31

                    if (operation < 0x1C and operation > 0x17) or operation == 0x1F:
                        reg_class = 0x10
                    else:
                        reg_class = 0x00
                    reg1 = reg_class | (reg_raw >> 4)
                    reg2 = reg_class | (reg_raw & 15)

                    a = self.getreg(reg1)
                    b = self.getreg(reg2)

                    self.loadreg(reg1, self.alu(operation, reg_set_bits(reg1), a, b))
                elif klass == 6:
                    # 0b0110xxxx
                    # Register - constant dual argument mathematics
                    reg_raw = self.readbyte(pc)
                    pc += 1

                    operation = (opcode & 15) | 16

                    if (operation < 0x1C and operation > 0x17) or operation == 0x1F:
                        throw()
                    
                    reg1 = (reg_raw & 15)

                    a = self.getreg(reg1)
                    b = self.readsize(reg_set_size(reg1 >> 3), pc)
                    pc += reg_set_bits(reg1 >> 3) >> 3

                    self.loadreg(reg1, self.alu(operation, reg_set_bits(reg1), a, b))
                else:
                    # 0b0111xxxx
                    # [reserved mathematical operations with memory]
                    pass
        else:
            # 0b1xxxxxxx
            # CISC operations
            pattern = opcode & 0x1F
            if not spec2:
                # 0b10xxxxxx
                if not spec3:
                    # 0b100xxxxx
                    if pattern == 0:
                        # (syscall n)
                        pc += 2
                        syscall(self.readword(pc - 2), self)
                    elif pattern == 1:
                        # (mov[s] <msb reverse bit> r1, *+)
                        pc += 1
                        reg_raw = self.readbyte(pc - 1)
                        regid = reg_raw & 0x1F
                        size = (reg_raw >> 5) & 3
                        mask = size_to_mask(size)
                        size = size_to_bytes(size)
                        # if this is a one, then the data will be moved to the most significant half of the register.
                        flip_msb = reg_raw >> 7

                        if size == 8:
                            # load qword
                            data = self.readqword(pc)
                        elif size == 4:
                            data = self.readdword(pc)
                        elif size == 2:
                            data = self.readword(pc)
                        else:
                            data = self.readbyte(pc)
                        pc += size

                        if flip_msb:
                            data = data << (reg_set_bits(regid>>3)//2)
                            mask = mask << (reg_set_bits(regid>>3)//2)

                        mask = mask ^ ((2**reg_set_bits(regid>>3)) - 1)
                        self.loadreg(regid, (self.getreg(regid) & mask) | data)
                    elif pattern == 0x10:
                        ctrl = self.readbyte(pc)
                        ptr = self.readqword(pc + 1)
                        op = ctrl >> 4
                        regdat = self.getreg(ctrl & 0x0F)
                        pc += 9
                        if op == 0:
                            # jz r1, @
                            if regdat == 0:
                                pc = ptr
                        else:
                            # jnz r1, @
                            if regdat != 0:
                                pc = ptr
                else:
                    # 0b101xxxxx
                    # CISC stack ops
                    if pattern == 0x00:
                        # pop
                        ctrl = self.readbyte(pc)
                        reg = ctrl & 0x1F
                        if reg == 7:
                            pc = self.popqword()
                        else:
                            self.loadreg(reg, self.popsize(reg_set_size(reg >> 3)))
                            pc += 1
                    elif pattern == 0x01:
                        # push
                        ctrl = self.readbyte(pc)
                        pc += 1
                        reg = ctrl & 0x1F
                        self.pushsize(reg_set_size(reg >> 3), self.getreg(reg))
                    elif pattern == 0x02:
                        # 0xA2 : call @
                        self.pushqword(pc + 8)
                        pc = self.readqword(pc)
            else:
                # 0b11xxxxxx
                if not spec3:
                    # 0b110xxxxx
                    if pattern & 0x13 == 0x00:
                        # lod[s]
                        ptr = self.getreg(4)
                        self.writereg(size, 0, self.readsize(size, ptr))
                        self.loadreg(4, ptr + size_to_bytes(size))
                else:
                    # 0b111xxxxx
                    if pattern == 0x00:
                        pc = self.readqword(pc)
                    elif pattern == 0x1F:
                        # stop
                        self.exited = True
        self.loadreg(0, 7, pc)
        return opcode
    def run(self, cycles = 0):
        while True:
            if self.step() == -1:
                return
            cycles -= 1
            if cycles == 0:
                return
    def printout(self):
        regnames = ['rax','rbx','rcx','rdx','si','di','sp','pc',
              'r0', 'r1', 'r2', 'r3', 'r4', 'r5', 'cnt', 'cr0',
              'f0', 'f1', 'f2', 'f3', 'f4', 'f5', 'f6', 'f7']
        for i in range(16):
            print(regnames[i], '\t: ', hex(self.getreg(i)))
    def printstep(self):
        try:
            print("Opcode", hex(self.step()))
            self.printout()
        except:
            print("Internal Exception!!!")
            self.printout()
            throw()
def loadbin(fn):
    f = open(fn, 'rb')
    data = f.read()
    f.close()
    return data
