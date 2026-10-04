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

    // ---- integrity audit (recurve) & provenance refresh (refresh) --------

    /** Severity of a {@link Finding}. */
    public enum Severity { OK, WARN, ERROR }

    /** One finding from a {@code recurve} audit. */
    public record Finding(Severity severity, String check, String message) {}

    /** The result of a {@code recurve} audit: findings plus the worst severity. */
    public static final class Report {
        public final java.util.List<Finding> findings = new java.util.ArrayList<>();
        public Severity worst = Severity.OK;

        void add(Severity sev, String check, String message) {
            if (sev.ordinal() > worst.ordinal()) worst = sev;
            findings.add(new Finding(sev, check, message));
        }
    }

    /**
     * A stable 64-bit FNV-1a fingerprint over the whole image. Matches the
     * C / C++ / SLeeLa tools so refresh checksums compare across languages.
     */
    public long fingerprint() {
        long h = 0xcbf29ce484222325L;             // FNV-1a offset basis
        for (byte b : data) {
            h ^= (b & 0xFFL);
            h *= 0x100000001b3L;                   // FNV prime
        }
        return h;
    }

    /** Classify a region: 0 = mixed, 1 = all 0x00, 2 = all 0xFF. */
    private int regionFill(int off, int n) {
        if (n == 0) return 0;
        int first = data[off] & 0xFF;
        if (first != 0x00 && first != 0xFF) return 0;
        for (int i = 1; i < n; i++) if ((data[off + i] & 0xFF) != first) return 0;
        return first == 0x00 ? 1 : 2;
    }

    private long fnv1a(int off, int n) {
        long h = 0xcbf29ce484222325L;
        for (int i = 0; i < n; i++) { h ^= (data[off + i] & 0xFFL); h *= 0x100000001b3L; }
        return h;
    }

    /**
     * recurve: audit structural + embedded-image (CHR tile bank) health
     * without modifying the ROM.
     */
    public Report recurve() {
        Report rep = new Report();

        // 1. Header / magic.
        if (data.length < 4 || data[0] != 'N' || data[1] != 'E'
                || data[2] != 'S' || (data[3] & 0xFF) != 0x1A) {
            rep.add(Severity.ERROR, "header", "iNES magic 'NES\\x1A' missing");
        } else {
            rep.add(Severity.OK, "header", "iNES magic present");
        }

        // 2. Bank counts / mapper.
        if (prgBanks == 0) {
            rep.add(Severity.ERROR, "prg", "PRG bank count is 0 (no program)");
        } else {
            rep.add(Severity.OK, "prg", String.format(
                    "%d PRG bank(s), %d bytes; mapper %d", prgBanks, prgSize, mapper));
        }

        // 3. Size consistency.
        long expect = (long) HEADER + (hasTrainer ? TRAINER : 0) + prgSize + chrSize;
        if (expect > data.length) {
            rep.add(Severity.ERROR, "size", String.format(
                    "truncated: header declares %d bytes but file is %d", expect, data.length));
        } else if (expect < data.length) {
            rep.add(Severity.WARN, "size", String.format(
                    "%d trailing byte(s) after declared regions", data.length - expect));
        } else {
            rep.add(Severity.OK, "size", "file length matches header");
        }

        // 4. Embedded images: audit each CHR (tile) bank.
        if (chrBanks == 0) {
            rep.add(Severity.WARN, "chr",
                    "no CHR-ROM banks (CHR-RAM title or program-only image)");
        } else {
            for (int b = 0; b < chrBanks; b++) {
                int off = chrOffset + b * CHR_BANK;
                String name = "chr#" + b;
                if ((long) off + CHR_BANK > data.length) {
                    rep.add(Severity.ERROR, name, String.format(
                            "CHR bank %d extends past end of file (damaged)", b));
                    continue;
                }
                int fill = regionFill(off, CHR_BANK);
                if (fill == 1) {
                    rep.add(Severity.WARN, name, String.format(
                            "CHR bank %d is entirely 0x00 (blank/no tiles)", b));
                } else if (fill == 2) {
                    rep.add(Severity.WARN, name, String.format(
                            "CHR bank %d is entirely 0xFF (erased/undamaged check)", b));
                } else {
                    rep.add(Severity.OK, name, String.format(
                            "CHR bank %d intact, tiles present (fp %016X)", b, fnv1a(off, CHR_BANK)));
                }
            }
        }

        // 5. PRG tail observation.
        if (prgSize >= 16 && regionFill(prgOffset + prgSize - 16, 16) == 2) {
            rep.add(Severity.OK, "prg-tail", "PRG tail is 0xFF fill (typical of unused space)");
        }

        return rep;
    }

    /**
     * refresh: write a clean, canonical re-emission of this owned ROM to
     * {@code outPath} (byte-identical payload) and a sidecar provenance record
     * at "&lt;outPath&gt;.provenance" (ISO-8601 UTC timestamp + fingerprint).
     * No ROM content is synthesized. Returns the fingerprint written.
     */
    public long refresh(Path outPath) throws IOException {
        // Re-emit a clean, canonical copy of the owned ROM (byte-identical).
        save(outPath);

        long fp = fingerprint();
        String ts = java.time.format.DateTimeFormatter.ofPattern("yyyy-MM-dd'T'HH:mm:ss'Z'")
                .withZone(java.time.ZoneOffset.UTC)
                .format(java.time.Instant.now());

        String record = ""
                + "# mearvk/Nintendo Professional Editor -- refresh provenance record\n"
                + "# This records WHEN a ROM you own was last re-emitted and its content\n"
                + "# fingerprint. It contains no copyrighted ROM data.\n"
                + "tool        = NesEditCli (Java)\n"
                + "action      = refresh\n"
                + "refreshed   = " + ts + "\n"
                + "image       = " + outPath + "\n"
                + "size        = " + data.length + "\n"
                + "prg_banks   = " + prgBanks + "\n"
                + "chr_banks   = " + chrBanks + "\n"
                + "mapper      = " + mapper + "\n"
                + String.format("fingerprint = fnv1a64:%016X%n", fp);
        Files.writeString(Path.of(outPath.toString() + ".provenance"), record);
        return fp;
    }

    /** Severity label ("ok" | "warn" | "error"). */
    public static String severityStr(Severity s) {
        return switch (s) {
            case OK -> "ok";
            case WARN -> "warn";
            case ERROR -> "error";
        };
    }
}
