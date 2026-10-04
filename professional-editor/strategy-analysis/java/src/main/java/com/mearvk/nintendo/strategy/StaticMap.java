package com.mearvk.nintendo.strategy;

import java.io.IOException;
import java.io.Writer;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

/**
 * Part A — static 6502 structure analysis of an iNES ROM.
 *
 * <p>Part of the mearvk/Nintendo Professional Editor. Analyzes a ROM the user
 * owns to produce structural evidence; ships no copyrighted content and copies
 * no game code. Classifies PRG-ROM bytes as code or data by following control
 * flow from the reset/NMI/IRQ vectors, and detects jump tables (seeding the
 * code pass from their entries so indirectly-dispatched routines are found).
 *
 * <p>This is a static <em>approximation</em>; indirect/computed jumps and bank
 * switches cannot be fully resolved statically. See {@code docs/METHOD.md}.
 */
public final class StaticMap {
    private static final int HEADER = 16;
    private static final int PRG_BANK = 16384;

    public enum SpanKind { CODE, DATA, JUMPTABLE }

    public record Span(SpanKind kind, int cpuAddr, int prgOff, int len, int detail) {}

    private final List<Span> spans = new ArrayList<>();
    private int resetVec, nmiVec, irqVec, mapBase;
    private int prgSize;

    // 6502 opcode -> length (1..3); 0 == undefined (decode stop).
    private static final int[] OPLEN = {
        1,2,0,0,0,2,2,0,1,2,1,0,0,3,3,0, 2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0,
        3,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0, 2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0,
        1,2,0,0,0,2,2,0,1,2,1,0,3,3,3,0, 2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0,
        1,2,0,0,0,2,2,0,1,2,1,0,3,3,3,0, 2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0,
        0,2,0,0,2,2,2,0,1,0,1,0,3,3,3,0, 2,2,0,0,2,2,2,0,1,3,1,0,0,3,0,0,
        2,2,2,0,2,2,2,0,1,2,1,0,3,3,3,0, 2,2,0,0,2,2,2,0,1,3,1,0,3,3,3,0,
        2,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0, 2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0,
        2,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0, 2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0
    };

    // --- working state during analysis ---
    private byte[] rom;
    private int prgOff;
    private byte[] code; // per-PRG-byte visited mark

    private StaticMap() {}

    public List<Span> spans() { return spans; }
    public int resetVec() { return resetVec; }
    public int nmiVec() { return nmiVec; }
    public int irqVec() { return irqVec; }
    public int prgSize() { return prgSize; }
    public int mapBase() { return mapBase; }

    public static StaticMap analyzeFile(Path path) throws IOException, NesException {
        byte[] data = Files.readAllBytes(path);
        if (data.length < HEADER || data[0] != 'N' || data[1] != 'E' || data[2] != 'S' || (data[3] & 0xFF) != 0x1A)
            throw new NesException("not a valid iNES image (bad magic): " + path);
        StaticMap m = new StaticMap();
        m.rom = data;
        int prgBanks = data[4] & 0xFF;
        boolean trainer = (data[6] & 0x04) != 0;
        m.prgOff = HEADER + (trainer ? 512 : 0);
        m.prgSize = prgBanks * PRG_BANK;
        if (m.prgOff + m.prgSize > data.length)
            throw new NesException("iNES PRG region exceeds file size: " + path);
        m.mapBase = (m.prgSize == PRG_BANK) ? 0xC000 : 0x8000;
        m.code = new byte[m.prgSize];

        int vec = m.prgSize - 6;
        m.nmiVec = m.rd16(vec);
        m.resetVec = m.rd16(vec + 2);
        m.irqVec = m.rd16(vec + 4);

        m.traceFrom(m.resetVec);
        m.traceFrom(m.nmiVec);
        m.traceFrom(m.irqVec);

        // Seed from jump-table entries to a fixpoint (resolve indirect targets).
        for (;;) {
            long before = marks(m.code);
            m.detectJumpTables(null, true);
            if (marks(m.code) == before) break;
        }
        m.detectJumpTables(m.spans, false);
        m.emitSpans();
        return m;
    }

    private static long marks(byte[] code) {
        long n = 0; for (byte b : code) n += b; return n;
    }

    private int cpuToPrg(int addr) {
        addr &= 0xFFFF;
        if (addr < mapBase) return -1;
        int off = addr - mapBase;
        if (prgSize == PRG_BANK) off %= PRG_BANK;
        if (off >= prgSize) return -1;
        return off;
    }

    private int rd16(int prgRel) {
        int fo = prgOff + prgRel;
        return (rom[fo] & 0xFF) | ((rom[fo + 1] & 0xFF) << 8);
    }

    private static boolean isJmpAbs(int op) { return op == 0x4C; }
    private static boolean isJsr(int op) { return op == 0x20; }
    private static boolean isRtsRti(int op) { return op == 0x60 || op == 0x40; }
    private static boolean isJmpInd(int op) { return op == 0x6C; }
    private static boolean isBranch(int op) {
        return switch (op) { case 0x10, 0x30, 0x50, 0x70, 0x90, 0xB0, 0xD0, 0xF0 -> true; default -> false; };
    }

    private void traceFrom(int entry) {
        ArrayList<Integer> stack = new ArrayList<>();
        stack.add(entry & 0xFFFF);
        while (!stack.isEmpty()) {
            int pc = stack.remove(stack.size() - 1);
            for (;;) {
                int off = cpuToPrg(pc);
                if (off < 0) break;
                if (code[off] != 0) break;
                int op = rom[prgOff + off] & 0xFF;
                int len = OPLEN[op];
                if (len == 0) { code[off] = 1; break; }
                for (int i = 0; i < len; i++) {
                    int o = cpuToPrg(pc + i);
                    if (o >= 0) code[o] = 1;
                }
                if (isJsr(op) || isJmpAbs(op)) {
                    stack.add(rd16(off + 1));
                    if (isJmpAbs(op)) break;
                } else if (isBranch(op)) {
                    int rel = (byte) rom[prgOff + off + 1];
                    stack.add((pc + 2 + rel) & 0xFFFF);
                } else if (isRtsRti(op) || isJmpInd(op)) {
                    break;
                }
                pc = (pc + len) & 0xFFFF;
            }
        }
    }

    private void detectJumpTables(List<Span> out, boolean seed) {
        final int MIN_ENTRIES = 4;
        int i = 0;
        while (i + 2 <= prgSize) {
            int start = i, entries = 0;
            while (i + 2 <= prgSize) {
                int ptr = rd16(i);
                if (cpuToPrg(ptr) < 0) break;
                entries++; i += 2;
            }
            if (entries >= MIN_ENTRIES) {
                if (out != null) {
                    int addr = mapBase + (start % (prgSize == PRG_BANK ? PRG_BANK : prgSize));
                    out.add(new Span(SpanKind.JUMPTABLE, addr & 0xFFFF, prgOff + start, entries * 2, entries));
                }
                if (seed) for (int e = 0; e < entries; e++) traceFrom(rd16(start + e * 2));
            } else {
                i = start + 1;
            }
        }
    }

    private void emitSpans() {
        int i = 0;
        while (i < prgSize) {
            int v = code[i];
            int run = 1;
            while (i + run < prgSize && code[i + run] == v) run++;
            int addr = mapBase + (i % (prgSize == PRG_BANK ? PRG_BANK : prgSize));
            spans.add(new Span(v != 0 ? SpanKind.CODE : SpanKind.DATA, addr & 0xFFFF, prgOff + i, run, 0));
            i += run;
        }
    }

    /** True if a CPU address falls inside a detected jump table. */
    public boolean inJumpTable(int addr) {
        for (Span s : spans) {
            if (s.kind() != SpanKind.JUMPTABLE) continue;
            int lo = s.cpuAddr(), hi = lo + s.len();
            if (addr >= lo && addr < hi) return true;
        }
        return false;
    }

    public void writeTsv(Path path) throws IOException {
        try (Writer w = Files.newBufferedWriter(path, StandardCharsets.UTF_8)) {
            w.write("# NES static map (code-vs-data + jump tables). Approximate; see METHOD.md.\n");
            w.write(String.format("# vectors: reset=%04X nmi=%04X irq=%04X  prg=%d map_base=%04X%n",
                    resetVec, nmiVec, irqVec, prgSize, mapBase));
            w.write("kind\tbank\taddr\tend\tdetail\n");
            w.write(String.format("vector\t-\tFFFC\tFFFD\treset=%04X%n", resetVec));
            for (Span s : spans) {
                String k = s.kind() == SpanKind.CODE ? "code" :
                           s.kind() == SpanKind.JUMPTABLE ? "jumptable" : "data";
                int end = s.cpuAddr() + (s.len() > 0 ? s.len() - 1 : 0);
                String detail = s.kind() == SpanKind.JUMPTABLE ? "entries=" + s.detail()
                        : (s.kind() == SpanKind.CODE ? "block" : "unreached");
                w.write(String.format("%s\t0\t%04X\t%04X\t%s%n", k, s.cpuAddr(), end, detail));
            }
        }
    }
}
