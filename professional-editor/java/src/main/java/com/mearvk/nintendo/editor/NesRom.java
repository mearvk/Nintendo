package com.mearvk.nintendo.editor;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * An in-memory iNES ROM image with text/menu editing.
 *
 * <p>Part of the mearvk/Nintendo Professional Editor. This class edits an iNES
 * {@code .nes} image the user already owns; it ships no copyrighted ROM content.
 *
 * <p>Editing is non-destructive by contract: a text replacement is bounded by
 * the original slot length (padded with the table fill byte if shorter), so
 * in-ROM pointers and bank boundaries are never disturbed.
 */
public final class NesRom {
    private static final int HEADER = 16;
    private static final int PRG_BANK = 16384;
    private static final int CHR_BANK = 8192;
    private static final int TRAINER = 512;

    private final byte[] data;
    private final int prgBanks;
    private final int chrBanks;
    private final boolean hasTrainer;
    private final int mapper;
    private final int prgOffset;
    private final int prgSize;
    private final int chrOffset;
    private final int chrSize;

    private NesRom(byte[] data) throws NesException {
        this.data = data;
        if (data.length < HEADER
                || data[0] != 'N' || data[1] != 'E' || data[2] != 'S' || (data[3] & 0xFF) != 0x1A) {
            throw new NesException("not a valid iNES image (bad magic)");
        }
        this.prgBanks = data[4] & 0xFF;
        this.chrBanks = data[5] & 0xFF;
        this.hasTrainer = (data[6] & 0x04) != 0;
        this.mapper = ((data[6] & 0xFF) >> 4) | (data[7] & 0xF0);
        this.prgOffset = HEADER + (hasTrainer ? TRAINER : 0);
        this.prgSize = prgBanks * PRG_BANK;
        this.chrSize = chrBanks * CHR_BANK;
        this.chrOffset = chrSize != 0 ? (prgOffset + prgSize) : 0;
        if (prgOffset + prgSize > data.length) {
            throw new NesException("iNES PRG region exceeds file size");
        }
    }

    /** Load an iNES image from disk. */
    public static NesRom load(Path path) throws IOException, NesException {
        return new NesRom(Files.readAllBytes(path));
    }

    /** Write the (possibly edited) image back to disk. */
    public void save(Path path) throws IOException {
        Files.write(path, data);
    }

    public int prgBanks() { return prgBanks; }
    public int chrBanks() { return chrBanks; }
    public int mapper() { return mapper; }
    public boolean hasTrainer() { return hasTrainer; }
    public int size() { return data.length; }
    public int prgOffset() { return prgOffset; }
    public int prgSize() { return prgSize; }
    public int chrOffset() { return chrOffset; }
    public int chrSize() { return chrSize; }

    /** Decode the {@code len}-byte slot at absolute file {@code offset}. */
    public String decode(int offset, int len, CharTable tbl) throws NesException {
        checkSlot(offset, len);
        StringBuilder sb = new StringBuilder(len);
        for (int i = 0; i < len; i++) sb.append(tbl.decodeByte(data[offset + i]));
        return sb.toString();
    }

    /**
     * Replace the slot at {@code offset} with {@code text} (encoded via
     * {@code tbl}, padded to {@code len}). Throws if the encoding exceeds the
     * slot or a glyph is unmapped.
     */
    public void setText(int offset, int len, String text, CharTable tbl) throws NesException {
        checkSlot(offset, len);
        if (text.length() > len) {
            throw new NesException("replacement (" + text.length()
                    + ") longer than slot (" + len + ")");
        }
        int fill = tbl.fillByte();
        for (int i = 0; i < len; i++) {
            if (i < text.length()) {
                int b = tbl.encodeGlyph(text.charAt(i));
                if (b < 0) throw new NesException("glyph not in table: '" + text.charAt(i) + "'");
                data[offset + i] = (byte) b;
            } else {
                data[offset + i] = (byte) fill;
            }
        }
    }

    /**
     * Guard: true iff the slot at {@code offset} currently decodes to
     * {@code expected} (ignoring trailing fill). Does not modify the ROM.
     */
    public boolean expectText(int offset, int len, String expected, CharTable tbl) throws NesException {
        checkSlot(offset, len);
        if (expected.length() > len) return false;
        String got = decode(offset, len, tbl);
        if (!got.regionMatches(0, expected, 0, expected.length())) return false;
        for (int i = expected.length(); i < len; i++) {
            char c = got.charAt(i);
            if (c != '.' && c != ' ') return false;
        }
        return true;
    }

    private void checkSlot(int offset, int len) throws NesException {
        if (offset < 0 || len < 0 || (long) offset + len > data.length) {
            throw new NesException(String.format(
                    "slot out of range: offset=0x%X len=%d size=%d", offset, len, data.length));
        }
    }
}
