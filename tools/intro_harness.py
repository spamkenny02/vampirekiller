"""Bounded Z80 scene reconstruction with explicit BIOS/VDP drawing adapters.
This is not a complete MSX emulator. Drawing commands execute synchronously.
"""
from collections import Counter
from z80 import Z80Machine
from PIL import Image


class IntroHarness:
    def __init__(self, rom):
        self.rom = rom
        self.cpu = Z80Machine()
        self.vram = bytearray(131072)
        self.palette = [(0, 0, 0)] * 16
        self.regs = bytearray(64)
        self.address = 0
        self.latch = None
        self.draw_calls = Counter()
        self.bios_calls = Counter()
        for bank in range(4):
            self.cpu.set_memory_block(0x4000 + bank * 8192, rom[bank * 8192:(bank + 1) * 8192])
        self.cpu.set_write_callback(self.mapper_write)
        self.cpu.mark_addrs(0x6000, 0x6000, self.cpu.WRITE_MARK)
        self.cpu.set_output_callback(self.output)
        self.cpu.set_input_callback(lambda port: 0)
        self.cpu.memory[6] = self.cpu.memory[7] = 0x98
        self.cpu.memory[0x2B] = 1  # International BIOS variant.
        self.hooks = {0x0047: self.bios_vdp, 0x005F: self.bios_mode,
                      0x0062: lambda: None, 0x0093: lambda: None,
                      0x4814: self.set_palette, 0x484C: self.wait_vdp,
                      0x490A: self.fill, 0x4946: self.copy,
                      0x498A: self.cpu_copy, 0x49DB: self.logical_copy,
                      0x509F: lambda: None}
        for address in (*self.hooks, 0x0100):
            self.cpu.set_breakpoint(address)

    def mapper_write(self, address, value):
        bank = value & 15
        window = address & 0xE000
        self.cpu.set_memory_block(window, self.rom[bank * 8192:(bank + 1) * 8192])

    def register(self, index, value):
        self.regs[index] = value
        if index < 8:
            self.cpu.memory[0xF3DF + index] = value
        elif index < 24:
            self.cpu.memory[0xFFE7 + index - 8] = value

    def bios_vdp(self):
        self.bios_calls['WRTVDP'] += 1
        self.register(self.cpu.c, self.cpu.b)

    def bios_mode(self):
        if self.cpu.a != 5:
            raise ValueError('Only SCREEN 5 is implemented')
        self.bios_calls['CHGMOD'] += 1
        self.register(2, 0x1F)
        self.register(1, 0x60)

    def set_palette(self):
        c = self.cpu
        self.palette[c.a & 15] = ((c.d >> 4) & 7, c.e & 7, c.d & 7)
        c.memory[0xF680 + c.a * 2] = c.d
        c.memory[0xF681 + c.a * 2] = c.e

    def wait_vdp(self):
        self.cpu.a = self.cpu.f = 0

    def output(self, port, value):
        port &= 255
        if port == 0x98:
            self.vram[self.address & 0x1FFFF] = value
            self.address = (self.address + 1) & 0x1FFFF
        elif port == 0x99:
            if self.latch is None:
                self.latch = value
            else:
                if value & 0x80:
                    self.register(value & 63, self.latch)
                else:
                    self.address = ((self.regs[14] & 7) << 14) | ((value & 63) << 8) | self.latch
                self.latch = None
        elif port == 0x9B:
            register = self.regs[17] & 63
            self.register(register, value)
            if not self.regs[17] & 128:
                self.regs[17] = (register + 1) & 63
            if register == 46:
                self.command(value)
        else:
            raise ValueError(f'Unadapted VDP output port {port:02X}')

    def command(self, value):
        if value != 0x70:
            raise ValueError(f'Unadapted VDP command {value:02X}')
        self.draw_calls['LINE'] += 1
        r = self.regs
        x, y = r[36] | ((r[37] & 1) << 8), r[38] | ((r[39] & 3) << 8)
        major, minor = r[40] | ((r[41] & 1) << 8), r[42] | ((r[43] & 3) << 8)
        if minor:
            raise ValueError('Only axis-aligned LINE is adapted')
        dx, dy = (-1 if r[45] & 4 else 1), (-1 if r[45] & 8 else 1)
        for step in range(major + 1):
            self.put(x if r[45] & 1 else x + step * dx,
                     y + step * dy if r[45] & 1 else y, r[44] & 15)

    def pixel(self, x, y):
        byte = self.vram[((y & 1023) * 128 + ((x & 255) >> 1)) & 0x1FFFF]
        return (byte & 15) if x & 1 else byte >> 4

    def put(self, x, y, color):
        if not 0 <= x < 256 or not 0 <= y < 1024:
            return
        address = y * 128 + x // 2
        value = self.vram[address]
        self.vram[address] = (value & 0xF0) | color if x & 1 else (value & 15) | (color << 4)

    def fill(self):
        c = self.cpu
        self.draw_calls['HMMV'] += 1
        x, y = c.h & 254, c.l + c.d * 256
        width, height = (c.b or 256) & 254, c.c or 256
        for yy in range(y, min(y + height, 1024)):
            for xx in range(x, min(x + width, 256)):
                self.put(xx, yy, c.a & 15 if xx & 1 else c.a >> 4)

    def blit(self, sx, sy, dx, dy, width, height, transparent=False):
        pixels = [[self.pixel(sx + x, sy + y) for x in range(width)] for y in range(height)]
        for y, row in enumerate(pixels):
            for x, color in enumerate(row):
                if color or not transparent:
                    self.put(dx + x, dy + y, color)

    def copy(self):
        c = self.cpu
        self.draw_calls['HMMM'] += 1
        self.blit(c.h & 254, c.l + (c.a & 3) * 256,
                  c.d & 254, c.e + ((c.a >> 2) & 3) * 256,
                  (c.b or 256) & 254, c.c or 256)

    def logical_copy(self):
        c = self.cpu
        op = c.a & 15
        if op not in (0, 8):
            raise ValueError(f'Unadapted LMMM operation {op}')
        self.draw_calls['LMMM'] += 1
        self.blit(c.h, c.l + ((c.a >> 6) & 3) * 256,
                  c.d, c.e + ((c.a >> 4) & 3) * 256,
                  c.b or 256, c.c or 256, op == 8)

    def cpu_copy(self):
        c = self.cpu
        self.draw_calls['HMMC'] += 1
        x, y, width, height = c.d & 254, c.e + c.a * 256, c.b or 256, c.c or 256
        address = c.hl
        for yy in range(height):
            for xx in range(0, width & 254, 2):
                value = c.memory[address]
                address = (address + 1) & 65535
                self.put(x + xx, y + yy, value >> 4)
                self.put(x + xx + 1, y + yy, value & 15)
        c.hl = address

    def call(self, address, budget=5000000):
        c = self.cpu
        c.sp = 0xF100
        c.memory[0xF100:0xF102] = bytes((0, 1))
        c.pc = address
        c.ticks_to_stop = budget
        for _ in range(100000):
            c.run()
            if c.pc == 0x0100:
                return
            if c.pc in self.hooks:
                self.hooks[c.pc]()
                c.pc = c.memory[c.sp] | (c.memory[(c.sp + 1) & 65535] << 8)
                c.sp = (c.sp + 2) & 65535
            elif not c.ticks_to_stop:
                raise RuntimeError(f'Z80 budget exceeded at {c.pc:04X} in call {address:04X}')
        raise RuntimeError('Too many scene hook calls')

    def indices(self, page=0, height=212):
        return bytes(self.pixel(x, y + page * 256) for y in range(height) for x in range(256))

    def image(self, page=0, height=212):
        image = Image.new('RGB', (256, height))
        colors = [tuple(round(v * 255 / 7) for v in color) for color in self.palette]
        image.putdata([colors[p] for p in self.indices(page, height)])
        return image

    def sprite_groups(self):
        groups = []
        for sprite in range(32):
            y, x, pattern, _ = self.vram[0xF600 + sprite * 4:0xF604 + sprite * 4]
            if y == 216:
                break
            y = y + 1 if y < 224 else y - 255
            if y <= -16 or y >= 212:
                continue
            color_lines = self.vram[0xF400 + sprite * 16:0xF410 + sprite * 16]
            early = bool(color_lines[0] & 128)
            x -= 32 if early else 0
            if not groups or groups[-1]['x'] != x or groups[-1]['y'] != y:
                groups.append({'x': x, 'y': y, 'pixels': bytearray(256), 'sprites': []})
            group = groups[-1]
            group['sprites'].append(sprite)
            source = 0xF800 + (pattern & 252) * 8
            for yy in range(16):
                command = color_lines[yy]
                color = command & 15
                for xx in range(16):
                    mask = self.vram[(source + yy + (16 if xx >= 8 else 0)) & 0x1FFFF]
                    if color and mask & (128 >> (xx & 7)):
                        index = yy * 16 + xx
                        if command & 64:
                            group['pixels'][index] |= color
                        elif not group['pixels'][index]:
                            group['pixels'][index] = color
        return [group for group in groups if any(group['pixels'])]

    def composite(self):
        image = self.image()
        colors = [tuple(round(v * 255 / 7) for v in color) for color in self.palette]
        for group in reversed(self.sprite_groups()):
            for yy in range(16):
                for xx in range(16):
                    color = group['pixels'][yy * 16 + xx]
                    x, y = group['x'] + xx, group['y'] + yy
                    if color and 0 <= x < 256 and 0 <= y < 212:
                        image.putpixel((x, y), colors[color])
        return image
