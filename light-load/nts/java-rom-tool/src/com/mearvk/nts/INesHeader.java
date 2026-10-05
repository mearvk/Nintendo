package com.mearvk.nts;

/**
 * Nintendo Technical Series (NTS) — iNES / NES 2.0 structural model.
 *
 * <p>This is the Java equivalent of the C {@code c-ines-reader} core. It parses
 * ONLY the 16-byte header and derives the physical region layout. It never
 * reads, extracts, or reproduces game content (text, maps, CHR tiles, code) —
 * it is the "describe, don't distribute" companion to light-load.
 *
 * <p>Modeled with Java OOD: an immutable value object built by a static factory
 * that validates and decodes the header. Compare with the procedural C version
 * and the C++ value-type version to study the same domain across paradigms.
 */
public final class INesHeader {

    public static final int HEADER_SIZE = 16;
    public static final int PRG_BANK = 16 * 1024;
    public static final int CHR_BANK = 8 * 1024;
    public static final int TRAINER_SIZE = 512;

    public enum Format { INES, NES20 }

    public enum Mirroring { HORIZONTAL, VERTICAL, FOUR_SCREEN }

    private final Format format;
    private final int mapper;
    private final int submapper;
    private final long prgBytes;
    private final long chrBytes;
    private final long prgRamBytes;
    private final long chrRamBytes;
    private final boolean exponentPrg;
    private final boolean exponentChr;
    private final Mirroring mirroring;
    private final boolean battery;
    private final boolean trainer;
    private final boolean usesChrRam;

    private INesHeader(Builder b) {
        this.format = b.format;
        this.mapper = b.mapper;
        this.submapper = b.submapper;
        this.prgBytes = b.prgBytes;
        this.chrBytes = b.chrBytes;
        this.prgRamBytes = b.prgRamBytes;
        this.chrRamBytes = b.chrRamBytes;
        this.exponentPrg = b.exponentPrg;
        this.exponentChr = b.exponentChr;
        this.mirroring = b.mirroring;
        this.battery = b.battery;
        this.trainer = b.trainer;
        this.usesChrRam = b.usesChrRam;
    }

    /** Thrown when the buffer is not a valid iNES header. */
    public static final class ParseException extends Exception {
        public ParseException(String msg) { super(msg); }
    }

    /**
     * Decode a NES 2.0 12-bit size field into a byte count. When the high
     * nibble is 0x0F the low byte is exponent notation: 2^E * (M*2+1).
     */
    public static long decodeSize(int low12, int unit, boolean[] isExponentOut) {
        if ((low12 & 0x0F00) == 0x0F00) {
            int mm = low12 & 0x03;
            int ex = (low12 >> 2) & 0x3F;
            if (isExponentOut != null) isExponentOut[0] = true;
            return (1L << ex) * (long) (mm * 2 + 1);
        }
        if (isExponentOut != null) isExponentOut[0] = false;
        return (long) low12 * unit;
    }

    /** Parse a 16-byte (or longer) header buffer into an immutable model. */
    public static INesHeader parse(byte[] buf) throws ParseException {
        if (buf == null || buf.length < HEADER_SIZE) {
            throw new ParseException("buffer smaller than iNES header");
        }
        if (!(buf[0] == 'N' && buf[1] == 'E' && buf[2] == 'S'
                && (buf[3] & 0xFF) == 0x1A)) {
            throw new ParseException("missing NES\\x1A magic");
        }

        Builder b = new Builder();
        int f6 = buf[6] & 0xFF;
        int f7 = buf[7] & 0xFF;

        b.format = ((f7 & 0x0C) == 0x08) ? Format.NES20 : Format.INES;

        int prgLo = buf[4] & 0xFF;
        int chrLo = buf[5] & 0xFF;

        if (b.format == Format.NES20) {
            int hi = buf[9] & 0xFF;
            int prg12 = prgLo | ((hi & 0x0F) << 8);
            int chr12 = chrLo | ((hi >> 4) << 8);
            b.mapper = (f6 >> 4) | (f7 & 0xF0) | ((buf[8] & 0x0F) << 8);
            b.submapper = (buf[8] & 0xFF) >> 4;

            boolean[] ep = new boolean[1];
            boolean[] ec = new boolean[1];
            b.prgBytes = decodeSize(prg12, PRG_BANK, ep);
            b.chrBytes = decodeSize(chr12, CHR_BANK, ec);
            b.exponentPrg = ep[0];
            b.exponentChr = ec[0];
            b.usesChrRam = (chr12 == 0);

            int pr = buf[10] & 0xFF, cr = buf[11] & 0xFF;
            b.prgRamBytes = ramShift(pr & 0x0F) + ramShift(pr >> 4);
            b.chrRamBytes = ramShift(cr & 0x0F) + ramShift(cr >> 4);
        } else {
            b.mapper = (f6 >> 4) | (f7 & 0xF0);
            b.submapper = 0;
            b.prgBytes = (long) prgLo * PRG_BANK;
            b.chrBytes = (long) chrLo * CHR_BANK;
            b.usesChrRam = (chrLo == 0);
        }

        b.battery = (f6 & 0x02) != 0;
        b.trainer = (f6 & 0x04) != 0;
        if ((f6 & 0x08) != 0) b.mirroring = Mirroring.FOUR_SCREEN;
        else if ((f6 & 0x01) != 0) b.mirroring = Mirroring.VERTICAL;
        else b.mirroring = Mirroring.HORIZONTAL;

        return new INesHeader(b);
    }

    private static long ramShift(int n) { return n == 0 ? 0L : (64L << n); }

    // --- accessors -----------------------------------------------------
    public Format format() { return format; }
    public int mapper() { return mapper; }
    public int submapper() { return submapper; }
    public long prgBytes() { return prgBytes; }
    public long chrBytes() { return chrBytes; }
    public long prgRamBytes() { return prgRamBytes; }
    public long chrRamBytes() { return chrRamBytes; }
    public boolean exponentPrg() { return exponentPrg; }
    public boolean exponentChr() { return exponentChr; }
    public Mirroring mirroring() { return mirroring; }
    public boolean battery() { return battery; }
    public boolean trainer() { return trainer; }
    public boolean usesChrRam() { return usesChrRam; }

    /** Header + trainer + PRG + CHR, in bytes. */
    public long computedSize() {
        long n = HEADER_SIZE + (trainer ? TRAINER_SIZE : 0);
        return n + prgBytes + chrBytes;
    }

    /** Builder used internally by {@link #parse(byte[])}. */
    private static final class Builder {
        Format format = Format.INES;
        int mapper, submapper;
        long prgBytes, chrBytes, prgRamBytes, chrRamBytes;
        boolean exponentPrg, exponentChr, battery, trainer, usesChrRam;
        Mirroring mirroring = Mirroring.HORIZONTAL;
    }
}
