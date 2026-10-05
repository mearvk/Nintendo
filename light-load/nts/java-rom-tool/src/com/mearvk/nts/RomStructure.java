package com.mearvk.nts;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * File-level structural view: the parsed {@link INesHeader} plus whole-file
 * facts (actual size, computed size, trailer, truncation, bank counts).
 *
 * <p>Separates the benign TRAILER (file larger than the declared layout) from
 * TRUNCATION (file smaller than declared — corruption), mirroring the C/C++
 * tools. Reads only header bytes and the file length; no content is inspected.
 */
public final class RomStructure {

    private final INesHeader header;
    private final long fileSize;
    private final long computedSize;
    private final long trailerBytes;
    private final boolean truncated;
    private final long prgBanks;
    private final long chrBanks;

    private RomStructure(INesHeader header, long fileSize) {
        this.header = header;
        this.fileSize = fileSize;
        this.computedSize = header.computedSize();
        this.truncated = fileSize != 0 && computedSize > fileSize;
        this.trailerBytes = (!truncated && fileSize != 0)
                ? fileSize - computedSize : 0;
        this.prgBanks = header.prgBytes() / INesHeader.PRG_BANK;
        this.chrBanks = header.chrBytes() / INesHeader.CHR_BANK;
    }

    /** Parse a header buffer with a known file size. */
    public static RomStructure of(byte[] header, long fileSize)
            throws INesHeader.ParseException {
        return new RomStructure(INesHeader.parse(header), fileSize);
    }

    /** Read a file from disk and build its structural view. */
    public static RomStructure read(Path path)
            throws IOException, INesHeader.ParseException {
        byte[] all = Files.readAllBytes(path);
        byte[] head = new byte[INesHeader.HEADER_SIZE];
        if (all.length < head.length) {
            throw new INesHeader.ParseException("file smaller than header");
        }
        System.arraycopy(all, 0, head, 0, head.length);
        return new RomStructure(INesHeader.parse(head), all.length);
    }

    public INesHeader header() { return header; }
    public long fileSize() { return fileSize; }
    public long computedSize() { return computedSize; }
    public long trailerBytes() { return trailerBytes; }
    public boolean truncated() { return truncated; }
    public long prgBanks() { return prgBanks; }
    public long chrBanks() { return chrBanks; }
}
