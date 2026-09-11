#!/usr/bin/env python3
"""Find every address nTheme needs in a Firebird snapshot or raw RAM dump.

Usage: hookfinder.py SNAPSHOT_OR_RAM [-o dump.txt]

Writes a hooks.c row for the OS found, the guard words, the dialog literals
and the theme color table. Anything that cannot be found is listed as
UNRESOLVED instead of guessed. Pure standard library.
"""

import argparse
import gzip
import struct
import sys

RAM_BASE = 0x10000000
SDRAM_SIZE = 0x04000000
BOOT1_SIZE = 0x80000
CODE_END = RAM_BASE + 0x00C00000
ARM_VECTORS = b"\x18\xf0\x9f\xe5" * 4
FINGERPRINT_ADDRESS = 0x10000020

# Ndless utils.c: fingerprint word -> (osid, model, version). CX II rows only.
KNOWN_OS = {
    0x1040E4D0: (34, "CX II", "5.2.0.771"),
    0x1040EAE0: (35, "CX II-T", "5.2.0.771"),
    0x1040F3B0: (36, "CX II CAS", "5.2.0.771"),
    0x10416CC0: (39, "CX II", "5.3.0.564"),
    0x10417460: (40, "CX II-T", "5.3.0.564"),
    0x10417DA0: (41, "CX II CAS", "5.3.0.564"),
    0x10429E10: (44, "CX II", "6.2.0.333"),
    0x1042A580: (45, "CX II-T", "6.2.0.333"),
    0x1042AE10: (46, "CX II CAS", "6.2.0.333"),
    0x10429EC0: (47, "CX II", "6.4.0.74"),
    0x1042A600: (48, "CX II-T", "6.4.0.74"),
    0x1042AEE0: (49, "CX II CAS", "6.4.0.74"),
}

SYST = 0x73797374

# Instruction encodings used as anchors.
PUSH_R4_LR = 0xE92D4010
SUBS_IP_R0 = 0xE250C000         # subs ip,r0,#0
LDR_IP_IP = 0xE59CC000          # ldr ip,[ip]
LDR_IP_IP_18 = 0xE59CC018       # ldr ip,[ip,#0x18]
BLX_IP = 0xE12FFF3C
# push {r3,r4,r5,lr}; ldr ip,[r0,#0x30]
DRIVER_PROLOGUE = (0xE92D4038, 0xE590C030)
ADD_R1_R2_R4_LSL2 = 0xE0821104
LDR_R0_R3_64 = 0xE5930064


class Unresolved(Exception):
    pass


# ---------------------------------------------------------------- memory


class Image:
    """64 MiB of SDRAM as seen at RAM_BASE."""

    def __init__(self, data):
        self.data = data

    def offset(self, address):
        return address - RAM_BASE

    def u32(self, address):
        return struct.unpack_from("<I", self.data, self.offset(address))[0]

    def find_all(self, pattern, start=RAM_BASE):
        out, index = [], self.offset(start)
        while True:
            index = self.data.find(pattern, index)
            if index < 0:
                return out
            out.append(RAM_BASE + index)
            index += 1

    def find_one(self, pattern, what):
        hits = self.find_all(pattern)
        if len(hits) != 1:
            raise Unresolved("%s: %d occurrences of %r"
                             % (what, len(hits), pattern))
        return hits[0]

    def find_near(self, pattern, address, span):
        """Address of `pattern` within `span` bytes after `address`."""
        begin = self.offset(address)
        index = self.data.find(pattern, begin, begin + span)
        return RAM_BASE + index if index >= 0 else None

    def pointers_to(self, address):
        word = struct.pack("<I", address)
        return [a for a in self.find_all(word) if a % 4 == 0]

    def cstring(self, address, limit=128):
        begin = self.offset(address)
        end = self.data.find(b"\0", begin, begin + limit)
        if end < 0:
            return None
        return self.data[begin:end].decode("latin1")

    def wstring(self, address, limit=256):
        out = []
        for i in range(limit):
            unit = struct.unpack_from("<H", self.data,
                                      self.offset(address) + 2 * i)[0]
            if unit == 0:
                return "".join(out)
            if unit < 0x20 or unit > 0x7E:
                return None
            out.append(chr(unit))
        return None

    def string_start(self, address):
        begin = self.data.rfind(b"\0", 0, self.offset(address)) + 1
        return RAM_BASE + begin


def load(path):
    with open(path, "rb") as f:
        data = f.read()
    if data[:2] == b"\x1f\x8b":
        data = gzip.decompress(data)
        size_word = struct.pack("<I", SDRAM_SIZE)
        index = -1
        while True:
            index = data.find(size_word, index + 1)
            if index < 0:
                raise Unresolved("no SDRAM block in snapshot")
            start = index + 4 + BOOT1_SIZE
            if data[start:start + 16] == ARM_VECTORS:
                return Image(data[start:start + SDRAM_SIZE])
    if data[:16] != ARM_VECTORS:
        raise Unresolved("neither a snapshot nor a RAM dump "
                         "starting with ARM vectors")
    return Image(data)


# ------------------------------------------------------------ ARM decoding


def is_bl(word):
    return word & 0xFF000000 == 0xEB000000


def is_b(word):
    return word & 0xFF000000 == 0xEA000000


def branch_target(address, word):
    offset = word & 0x00FFFFFF
    if offset & 0x00800000:
        offset -= 0x01000000
    return address + 8 + offset * 4


def is_push_lr(word):
    return word & 0xFFFF4000 == 0xE92D4000


def ldr_literal(address, word):
    """Address a `ldr rX,[pc,#imm]` loads from, else None."""
    if word & 0x0F7F0000 != 0x051F0000:
        return None
    imm = word & 0xFFF
    return address + 8 + (imm if word & 0x00800000 else -imm)


def mov_immediate(word):
    """(rd, value) for `mov rd,#imm`, else None."""
    if word & 0x0FEF0000 != 0x03A00000:
        return None
    imm, rotate = word & 0xFF, ((word >> 8) & 0xF) * 2
    value = ((imm >> rotate) | (imm << (32 - rotate))) & 0xFFFFFFFF
    return (word >> 12) & 0xF, value


def is_pc_relative(word):
    if word & 0x0E000000 == 0x0A000000:                    # b, bl
        return True
    uses_pc_base = (word >> 16) & 0xF == 15
    if word & 0x0C000000 == 0x04000000 and uses_pc_base:   # ldr/str [pc]
        return True
    if word & 0x0C000000 == 0x00000000 and uses_pc_base:   # add/sub rd,pc
        return True
    return False


def destination_register(word):
    """Register written by a data-processing or load instruction."""
    if word & 0x0C000000 == 0 or word & 0x0C100000 == 0x04100000:
        return (word >> 12) & 0xF
    return None


class Code:
    """Instruction-level helpers over the OS image."""

    def __init__(self, image):
        self.image = image
        self.bl_index = {}
        for address in range(RAM_BASE, CODE_END, 4):
            word = image.u32(address)
            if is_bl(word):
                target = branch_target(address, word)
                self.bl_index.setdefault(target, []).append(address)

    def word(self, address):
        return self.image.u32(address)

    def function_start(self, address):
        for a in range(address, address - 0x4000, -4):
            if is_push_lr(self.word(a)):
                return a
        raise Unresolved("no function start before 0x%08x" % address)

    def function_end(self, start):
        for a in range(start + 4, start + 0x4000, 4):
            if is_push_lr(self.word(a)):
                return a
        return start + 0x4000

    def instructions(self, start):
        end = self.function_end(start)
        return [(a, self.word(a)) for a in range(start, end, 4)]

    def calls(self, start):
        return [(a, branch_target(a, w))
                for a, w in self.instructions(start) if is_bl(w)]

    def call_targets(self, start):
        return [t for _, t in self.calls(start)]

    def tail_branches(self, start):
        end = self.function_end(start)
        out = []
        for a, w in self.instructions(start):
            if is_b(w) and not start <= branch_target(a, w) < end:
                out.append(branch_target(a, w))
        return out

    def literals(self, start):
        out = []
        for a, w in self.instructions(start):
            at = ldr_literal(a, w)
            if at is not None:
                out.append((a, self.word(at)))
        return out

    def callers(self, start):
        """bl sites reaching `start`; the entry may precede the push."""
        out = []
        for entry in (start, start - 4, start - 8):
            out += self.bl_index.get(entry, [])
        return out

    def function_referencing(self, address, what):
        refs = self.image.pointers_to(address)
        starts = sorted({self.function_start(r)
                         for r in refs if RAM_BASE <= r < CODE_END})
        if len(starts) != 1:
            raise Unresolved("%s: referenced from %d functions"
                             % (what, len(starts)))
        return starts[0]

    def functions_referencing_string(self, text):
        starts = set()
        for hit in self.image.find_all(text):
            start = self.image.string_start(hit)
            for ref in self.image.pointers_to(start):
                if RAM_BASE <= ref < CODE_END:
                    starts.add(self.function_start(ref))
        return sorted(starts)

    def gc_wrapper_slot(self, target):
        """Vtable slot a gc.c wrapper dispatches through, else None.

        Shape: null check, push {r4,lr}, ldr ip,[ip]; ldr ip,[ip,#slot].
        """
        head = {self.word(target), self.word(target + 4)}
        if head != {PUSH_R4_LR, SUBS_IP_R0}:
            return None
        for a in range(target + 8, target + 40, 4):
            if (self.word(a) == LDR_IP_IP
                    and self.word(a + 4) & 0xFFFFF000 == LDR_IP_IP):
                return self.word(a + 4) & 0xFFF
        return None

    def is_gc_wrapper(self, target):
        return self.gc_wrapper_slot(target) is not None

    def last_write_to(self, register, before, limit=16):
        """Constant last written to `register` before `before`, within
        the same basic block."""
        for a in range(before - 4, before - 4 * limit, -4):
            w = self.word(a)
            if is_bl(w):
                break
            if destination_register(w) != register:
                continue
            mov = mov_immediate(w)
            if mov:
                return mov[1]
            literal = ldr_literal(a, w)
            if literal is not None:
                return self.word(literal)
            raise Unresolved("r%d written by a non-constant instruction "
                             "at 0x%08x" % (register, a))
        raise Unresolved("no write to r%d before 0x%08x"
                         % (register, before))

    def hook_window_before(self, address, protected_registers):
        """Last 8 PC-free bytes before `address`; nothing from there to
        `address` may write a protected register."""
        for site in range(address - 8, address - 64, -4):
            words = [self.word(site), self.word(site + 4)]
            if any(is_pc_relative(w) for w in words):
                continue
            following = [self.word(a) for a in range(site, address, 4)]
            if any(destination_register(w) in protected_registers
                   for w in following):
                raise Unresolved("register clobbered between 0x%08x and "
                                 "0x%08x" % (site, address))
            return site, words
        raise Unresolved("no hook window before 0x%08x" % address)


# ------------------------------------------------------------------ finders


class Result:
    def __init__(self, image):
        self.image = image
        self.values = {}
        self.notes = []
        self.unresolved = []

    def set(self, name, value, note=""):
        self.values[name] = value
        if note:
            self.notes.append("%s: %s" % (name, note))

    def get(self, name):
        if name not in self.values:
            raise Unresolved("needs " + name)
        return self.values[name]

    def run(self, finder, code):
        try:
            finder(code, self)
        except Exception as e:   # one failed finder must not hide the rest
            self.unresolved.append("%s: %s: %s"
                                   % (finder.__name__, type(e).__name__, e))


def find_identity(code, r):
    fingerprint = code.word(FINGERPRINT_ADDRESS)
    r.set("fingerprint", fingerprint)
    if fingerprint not in KNOWN_OS:
        raise Unresolved("fingerprint 0x%08x not in the Ndless OS table"
                         % fingerprint)
    osid, model, version = KNOWN_OS[fingerprint]
    r.set("osid", osid)
    r.set("model", model)
    r.set("os_version", version)


def read_names(image, table, count):
    """Names come from the runtime pointer array the OS builds; the static
    blob after the table is not in index order."""
    first = table + count * 4
    if not image.wstring(first):
        return None
    for array in image.pointers_to(first):
        names = []
        for i in range(count):
            pointer = image.u32(array + 4 * i)
            if not RAM_BASE <= pointer < RAM_BASE + SDRAM_SIZE - 512:
                break
            name = image.wstring(pointer)
            if not name:
                break
            names.append(name)
        if len(names) == count:
            return names
    return None


def find_color_table(code, r):
    image = code.image
    message = image.find_one(b"UI Theme is Out of Range (GetColor).",
                             "GetColor assert")
    get_color = code.function_referencing(message, "GetColor")
    r.set("get_color", get_color,
          "function asserting 'UI Theme is Out of Range (GetColor).'")
    literals = [v for _, v in code.literals(get_color)]
    bounds = [v + 1 for v in literals if v < 4096]
    tables = [v for v in literals
              if RAM_BASE < v < CODE_END and len(image.pointers_to(v)) == 2]
    if len(tables) != 1 or not bounds:
        raise Unresolved("%d table literals, %d bounds in GetColor"
                         % (len(tables), len(bounds)))
    table = tables[0]
    r.set("color_table", table,
          "literal in GetColor with exactly two references")
    for count in bounds:
        names = read_names(image, table, count)
        if names:
            r.set("color_count", count,
                  "GetColor bound literal + 1, confirmed by the name array")
            r.set("color_names", names,
                  "heap array of %d pointers, the first one to the name "
                  "blob after the table" % count)
            return
    r.set("color_count", bounds[0],
          "first small literal in GetColor + 1; names not found")
    raise Unresolved("no runtime name array for any bound in %s "
                     "(OS not booted in this dump?)"
                     % ", ".join(str(b) for b in bounds))


def find_menu(code, r):
    image = code.image
    context = image.find_one(b"CTXT_ChangeLanguage\0",
                             "first Home > 5 context name")
    base_refs = image.pointers_to(context)
    if len(base_refs) != 1:
        raise Unresolved("%d pointers to CTXT_ChangeLanguage"
                         % len(base_refs))
    base = base_refs[0]
    count = 0
    while (image.cstring(image.u32(base + 4 * count)) or "") \
            .startswith("CTXT_"):
        count += 1
    builders = code.functions_referencing_string(b"/popupMenu_sys")
    literal_refs = [a for a in image.pointers_to(base)
                    if any(f <= a < code.function_end(f) for f in builders)]
    if len(literal_refs) != 1:
        raise Unresolved("%d menu table pointers inside functions "
                         "mentioning popupMenu_sys*.c" % len(literal_refs))
    r.set("menu_base", base, "pointer to 'CTXT_ChangeLanguage'")
    r.set("menu_context_count", count)
    r.set("menu_literal", literal_refs[0],
          "literal-pool word of the popupMenu_sys*.c builder "
          "holding menu_base")
    records = base + 4 * count
    callback = image.u32(records + 8)
    events = [v for _, v in code.literals(callback)][:2]
    if len(events) != 2:
        raise Unresolved("menu callback has no event literals")
    r.set("event_enter", events[0],
          "first literal of the shared menu callback 0x%08x" % callback)
    r.set("event_click", events[1],
          "second literal of the shared menu callback")
    rows = []
    for i in range(16):
        record = records + 20 * i
        if image.u32(record) == 0:
            break
        rows.append("{type %d, syst 0x%x, callback 0x%08x, arg %d}" % (
            image.u32(record), image.u32(record + 4),
            image.u32(record + 8), image.u32(record + 12)))
    r.set("menu_records", rows)


def theme_color_call(code, instructions, index, set_color_literals):
    """(theme id, callee) if instructions[index] is `mov r1,#id` followed
    by a bl into the SetColor block, else None."""
    (a, w), (_, next_word) = instructions[index], instructions[index + 1]
    mov = mov_immediate(w)
    if not (mov and mov[0] == 1 and is_bl(next_word)):
        return None
    target = branch_target(a + 4, next_word)
    if not any(target <= lit < target + 0x100 for lit in set_color_literals):
        return None
    return mov[1], target


def find_title_font(code, r, instructions, draw_string, set_theme_color):
    """The font is the constant r1 of gui_gc_getStringWidth, the last call
    before the title drawString whose r1 is a constant (the theme-color
    call in between is skipped)."""
    calls = [x for x, y in instructions
             if is_bl(y) and x < draw_string
             and branch_target(x, y) != set_theme_color]
    for call in sorted(calls, reverse=True):
        try:
            font = code.last_write_to(1, call)
        except Unresolved:
            continue
        r.set("home_title_font", font,
              "constant r1 of the call at 0x%08x (gui_gc_getStringWidth)"
              % call)
        return
    raise Unresolved("no call with a constant r1 before the title "
                     "drawString")


def find_home_painter(code, r):
    image = code.image
    set_color_literals = set(image.pointers_to(r.get("color_table")))
    painters = code.functions_referencing_string(b"homescreenframe.c")
    for painter in painters:
        instructions = code.instructions(painter)
        for i in range(len(instructions) - 1):
            found = theme_color_call(code, instructions, i,
                                     set_color_literals)
            if not found:
                continue
            background_id, set_theme_color = found
            a = instructions[i][0]
            r.set("home_painter", painter,
                  "function referencing homescreenframe.c with "
                  "mov r1,#id; bl gcSetThemeColor")
            r.set("gc_set_theme_color", set_theme_color)
            r.set("home_background_id", background_id,
                  "theme index the painter fills the background with")
            wrapper_calls = [x for x, y in instructions if is_bl(y)
                             and code.is_gc_wrapper(branch_target(x, y))]
            fill = min(x for x in wrapper_calls if x > a)
            words = [code.word(fill + 4), code.word(fill + 8)]
            if any(is_pc_relative(w) for w in words):
                raise Unresolved("code after the background fill is "
                                 "PC-relative")
            r.set("home_background_site", (fill + 4, words),
                  "8 bytes after bl gui_gc_fillRect at 0x%08x" % fill)
            draw_string = max(x for x in wrapper_calls if x < a)
            site, words = code.hook_window_before(draw_string, {1, 2})
            r.set("home_title_site", (site, words),
                  "before bl gui_gc_drawString at 0x%08x; "
                  "r1 = text, r2 = x" % draw_string)
            find_title_font(code, r, instructions, draw_string,
                            set_theme_color)

            def slot(call):
                return code.gc_wrapper_slot(
                    branch_target(call, code.word(call)))
            r.notes.append("gc vtable slots: title drawString 0x%x, "
                           "background fillRect 0x%x (compare with a known "
                           "OS before trusting a new row)"
                           % (slot(draw_string), slot(fill)))
            return
    raise Unresolved("no painter with mov r1,#id; bl gcSetThemeColor")


def find_set_color(code, r):
    image = code.image
    prologue = struct.pack("<2I", SUBS_IP_R0, PUSH_R4_LR)
    vtable_load = struct.pack("<2I", LDR_IP_IP, LDR_IP_IP_18)
    wrappers = [a for a in image.find_all(prologue) if a % 4 == 0
                and image.find_near(vtable_load, a, 32)]
    if len(wrappers) != 1:
        raise Unresolved("%d gui_gc_setColorRGB wrapper candidates"
                         % len(wrappers))
    wrapper = wrappers[0]
    call = image.find_near(struct.pack("<I", BLX_IP), wrapper, 64)
    if call is None:
        raise Unresolved("no blx ip in the wrapper at 0x%08x" % wrapper)
    r.set("set_color_wrapper_return", call + 4,
          "after blx ip in the wrapper at 0x%08x" % wrapper)
    prologue = struct.pack("<2I", *DRIVER_PROLOGUE)
    drivers = [a for a in image.find_all(prologue)
               if a % 4 == 0 and len(image.pointers_to(a)) == 1]
    if len(drivers) != 1:
        raise Unresolved("%d driver setColorRGB candidates" % len(drivers))
    r.set("set_color_driver", (drivers[0], list(DRIVER_PROLOGUE)),
          "push {r3,r4,r5,lr}; ldr ip,[r0,#0x30] with one pointer to it "
          "(gc vtable slot 0x18)")


def find_editor_configs(code, r):
    image = code.image
    found = []
    for hit in image.find_all(struct.pack("<I", 0xFFFFFF)):
        base = hit - 0x38
        if base % 4 or base < RAM_BASE:
            continue
        if (image.u32(base + 0x34) == 1 and image.u32(base + 0x44) == 1
                and image.u32(base + 0x48) == 0xFF0000):
            found.append(base)
    if len(found) != 3:
        raise Unresolved("%d editor config structs match the field "
                         "signature" % len(found))
    r.set("editor_config", found,
          "structs with +0x34 == 1, +0x38 == white, +0x44 == 1, "
          "+0x48 == red")


def find_plot_palette(code, r):
    image = code.image
    index_step = struct.pack("<I", ADD_R1_R2_R4_LSL2)
    bank_load = struct.pack("<I", LDR_R0_R3_64)
    getters = [a for a in image.find_all(index_step) if a % 4 == 0
               and image.find_near(bank_load, a, 32)
               and ldr_literal(a - 4, code.word(a - 4)) is not None]
    if len(getters) != 1:
        raise Unresolved("%d plot palette getter candidates" % len(getters))
    literal = ldr_literal(getters[0] - 4, code.word(getters[0] - 4))
    palette = code.word(literal) + 0x64
    entries = [image.u32(palette + 4 * i) for i in range(16)]
    if 0xC0C0C0 not in entries:
        raise Unresolved("no #C0C0C0 grid entry in the palette")
    r.set("plot_palette", palette, "getter struct literal + 0x64")
    r.set("plot_grid_index", entries.index(0xC0C0C0),
          "palette entry holding #C0C0C0")


def find_dialog(code, r):
    image = code.image
    name = image.find_one(b"DLG AFW - Handheld Setup\0",
                          "Handheld Setup dialog name")
    create = code.function_referencing(name, "dialog create function")
    anchor = next((t for t in code.call_targets(create)
                   if is_b(code.word(t))), None)
    if anchor is None:
        raise Unresolved("no thunk call in the create function")

    def is_thunk(a):
        """Inside the dialog manager's entry-point table."""
        return abs(a - anchor) < 0x800

    def thunks(start):
        seen, out = set(), []
        for t in code.call_targets(start):
            if is_thunk(t) and t not in seen:
                seen.add(t)
                out.append(t)
        return out

    def plain_calls(start):
        return [t for t in code.call_targets(start) if not is_thunk(t)]

    def tail_thunk(start):
        tails = [t for t in code.tail_branches(start) if is_thunk(t)]
        if not tails:
            raise Unresolved("no thunk tail branch in 0x%08x" % start)
        return tails[-1]

    d = {}
    (d["dialog_new"], d["dialog_content"], d["panel_new"],
     d["set_layout"], d["dialog_delete"]) = thunks(create)[:5]
    builders = code.callers(create)
    if len(builders) != 1:
        raise Unresolved("%d callers of the create function"
                         % len(builders))
    builder = code.function_start(builders[0])
    _, rows, buttons = code.call_targets(builder)[:3]
    row = code.call_targets(rows)[0]
    d["add_label"], d["add_combo"], d["pair_components"] = thunks(row)[:3]
    checkbox_row = [t for t in code.tail_branches(rows)
                    if not is_thunk(t)][-1]
    d["add_checkbox"] = thunks(checkbox_row)[0]
    (d["panel_new"], d["set_layout"], d["pair_components"],
     d["set_default_button"]) = thunks(buttons)[:4]
    d["set_escape_button"] = tail_thunk(buttons)
    d["add_button"] = code.tail_branches(plain_calls(buttons)[0])[-1]
    callbacks = [v for _, v in code.literals(buttons)
                 if RAM_BASE < v < CODE_END and is_push_lr(code.word(v))]
    ok_callback, cancel_callback = callbacks[:2]
    d["run_or_close"] = thunks(ok_callback)[0]
    d["component_dialog"] = thunks(cancel_callback)[0]
    d["combo_get"], d["checkbox_get"] = thunks(plain_calls(ok_callback)[0])
    main = code.function_start(code.callers(builder)[0])
    load_values = code.call_targets(main)[1]
    set_values = code.tail_branches(load_values)[-1]
    d["combo_set"] = thunks(set_values)[0]
    d["checkbox_set"] = tail_thunk(set_values)
    for key, value in d.items():
        if not is_thunk(value) and key != "add_button":
            raise Unresolved("%s = 0x%08x is not a dialog-manager thunk"
                             % (key, value))
    r.set("dialog", d,
          "ordinal walk of the Handheld Setup builder 0x%08x" % builder)
    literals = {}
    for label, start in (("create", create), ("row", row),
                         ("checkbox_row", checkbox_row),
                         ("buttons", buttons), ("set_values", set_values)):
        literals[label] = sorted({v for _, v in code.literals(start)
                                  if 0x10000 < v < 0x20000})
    r.set("dialog_literals", literals)


def find_style_string(code, r):
    image = code.image
    functions = code.functions_referencing_string(b"TI_RM_GetString()")
    if len(functions) != 1:
        raise Unresolved("%d functions mention TI_RM_GetString()"
                         % len(functions))
    manager_pointer = next(v for _, v in code.literals(functions[0])
                           if v >= CODE_END)
    manager = image.u32(manager_pointer)
    for i in range(64):
        if image.u32(manager + 8 * i) == SYST:
            block = image.u32(manager + 8 * i + 4)
            break
    else:
        raise Unresolved("no 'syst' module in the string manager")
    count = image.u32(block)
    strings = block + 4 + count * 4
    for i in range(count):
        offset = image.u32(block + 4 + 4 * i)
        if image.wstring(strings + 2 * offset, 8) == "Style":
            r.set("menu_label_string", i, "syst string id of 'Style'")
            return
    raise Unresolved("'Style' not among the %d syst strings" % count)


FINDERS = [find_identity, find_color_table, find_menu, find_home_painter,
           find_set_color, find_editor_configs, find_plot_palette,
           find_dialog, find_style_string]


# ------------------------------------------------------------------- output


DIALOG_KEYS = ("dialog_new", "dialog_content", "panel_new", "set_layout",
               "add_label", "add_combo", "add_checkbox", "pair_components",
               "add_button", "set_default_button", "set_escape_button",
               "run_or_close", "component_dialog", "dialog_delete",
               "combo_get", "combo_set", "checkbox_get", "checkbox_set")


def site(values, key):
    if key not in values:
        return "UNRESOLVED"
    address, words = values[key]
    return "{ 0x%08x, 0x%08X, 0x%08X }" % (address, words[0], words[1])


def emit_row(r, out):
    v = r.values

    def g(key, fmt="0x%08x"):
        return fmt % v[key] if key in v else "UNRESOLVED"

    d = v.get("dialog", {})
    editors = ", ".join("0x%08x" % a for a in v.get("editor_config", []))
    out.append("\t{")
    out.append("\t\t.osid = %s, .hardware_subtype = 2," % g("osid", "%d"))
    out.append("\t\t.fingerprint_address = 0x%08x, .fingerprint = %s,"
               % (FINGERPRINT_ADDRESS, g("fingerprint")))
    out.append('\t\t.model = "%s", .os_version = "%s",'
               % (v.get("model", "?"), v.get("os_version", "?")))
    out.append("\t\t.color_table = %s, .color_count = %s,"
               % (g("color_table"), g("color_count", "%d")))
    out.append("\t\t.editor_config = { %s }," % (editors or "UNRESOLVED"))
    out.append("\t\t.plot_palette = %s," % g("plot_palette"))
    out.append("\t\t.menu_literal = %s, .menu_base = %s, "
               ".menu_context_count = %s,"
               % (g("menu_literal"), g("menu_base"),
                  g("menu_context_count", "%d")))
    out.append("\t\t.menu_label_string = %s, /* \"Style\" */"
               % g("menu_label_string", "0x%x"))
    out.append("\t\t.event_enter = %s, .event_click = %s,"
               % (g("event_enter", "0x%x"), g("event_click", "0x%x")))
    out.append("\t\t.set_color_driver = %s," % site(v, "set_color_driver"))
    out.append("\t\t.set_color_wrapper_return = %s,"
               % g("set_color_wrapper_return"))
    out.append("\t\t.home_background = %s,"
               % site(v, "home_background_site"))
    out.append("\t\t.home_title = %s," % site(v, "home_title_site"))
    out.append("\t\t.home_title_font = %s," % g("home_title_font", "0x%x"))
    out.append("\t\t.os_code_begin = 0x%08x, .os_code_end = 0x%08x,"
               % (RAM_BASE, CODE_END))
    out.append("\t\t.ids = { .home_background = %s, .plot_grid_index = %s,"
               " /* the rest: read the color table below */ },"
               % (g("home_background_id", "%d"),
                  g("plot_grid_index", "%d")))
    out.append("\t\t.dialog = {")
    for key in DIALOG_KEYS:
        value = "0x%08x" % d[key] if key in d else "UNRESOLVED"
        out.append("\t\t\t.%s = %s," % (key, value))
    out.append("\t\t},")
    out.append("\t},")


def write_dump(r, out):
    v = r.values
    out.append("# nTheme hookfinder: %s OS %s (osid %s)"
               % (v.get("model", "?"), v.get("os_version", "?"),
                  v.get("osid", "?")))
    out.append("")
    for line in r.unresolved:
        out.append("UNRESOLVED " + line)
    if r.unresolved:
        out.append("")
    out.append("## hooks.c row")
    emit_row(r, out)
    out.append("")
    out.append("## anchors")
    out.extend(r.notes)
    for row in v.get("menu_records", []):
        out.append("menu record: " + row)
    for label, values in v.get("dialog_literals", {}).items():
        out.append("dialog literals in %s: %s"
                   % (label, ", ".join("0x%x" % x for x in values)))
    out.append("")
    out.append("## color table")
    if "color_table" in v:
        for i, name in enumerate(v.get("color_names", [])):
            color = r.image.u32(v["color_table"] + 4 * i)
            out.append("%3d  %-56s #%06X" % (i, name, color))


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("input",
                        help="Firebird snapshot (gzip) or raw RAM dump")
    parser.add_argument("-o", "--output", default="dump.txt")
    args = parser.parse_args()
    try:
        image = load(args.input)
    except Unresolved as e:
        sys.exit("error: %s" % e)
    code = Code(image)
    result = Result(image)
    for finder in FINDERS:
        result.run(finder, code)
    lines = []
    write_dump(result, lines)
    with open(args.output, "w") as f:
        f.write("\n".join(lines) + "\n")
    for line in result.unresolved:
        print("UNRESOLVED", line)
    print("%s: %d unresolved, %d color entries"
          % (args.output, len(result.unresolved),
             len(result.values.get("color_names", []))))


if __name__ == "__main__":
    main()
