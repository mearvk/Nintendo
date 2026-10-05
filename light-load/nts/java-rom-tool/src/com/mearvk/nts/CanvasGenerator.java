package com.mearvk.nts;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * NTS canvas generator — emits a BRAND-NEW, EMPTY NES 2.0 image of a chosen
 * footprint. Copies nothing from any existing game; every byte is original
 * (header + zeros). This is the legitimate "expand the canvas" path.
 *
 * <p>Large sizes use NES 2.0 exponent notation ({@code 2^E * (M*2+1)}). A
 * multi-hundred-MB canvas is a FORMAT-CAPACITY demonstration, not a runnable
 * ROM — real mappers/emulators cap out far lower.
 */
public final class CanvasGenerator {

    /** Immutable request for a canvas. */
    public record Spec(long prgBytes, long chrBytes, int mapper,
                       int submapper, boolean battery) {}

    /** Result of encoding a size field. */
    public record SizeField(int lowByte, int highNibble, long actualBytes,
                            boolean exponent) {}

    /**
     * Encode a desired byte count as the smallest NES 2.0-representable size
     * {@code >= want} using exponent notation.
     */
    public static SizeField encode(long want) {
        if (want == 0) return new SizeField(0, 0, 0, false);
        long best = -1; int bestE = -1, bestM = -1;
        for (int m = 0; m < 4; m++) {
            long mult = m * 2L + 1;
            for (int e = 0; e < 63; e++) {
                if (e >= 62 && mult > 1) break;
                long size = (1L << e) * mult;
                if (size < want) continue;
                if (best < 0 || size < best) { best = size; bestE = e; bestM = m; }
            }
        }
        if (best < 0) throw new IllegalArgumentException("size not representable");
        int lo = ((bestE & 0x3F) << 2) | (bestM & 0x03);
        return new SizeField(lo, 0x0F, best, true);
    }

    /** Build the 16-byte NES 2.0 header for a spec. */
    public static byte[] header(Spec spec) {
        SizeField prg = encode(spec.prgBytes());
        SizeField chr = encode(spec.chrBytes());
        byte[] h = new byte[INesHeader.HEADER_SIZE];
        h[0] = 'N'; h[1] = 'E'; h[2] = 'S'; h[3] = 0x1A;
        h[4] = (byte) prg.lowByte();
        h[5] = (byte) chr.lowByte();
        h[6] = (byte) (((spec.mapper() & 0x0F) << 4) | (spec.battery() ? 0x02 : 0));
        h[7] = (byte) (0x08 | (spec.mapper() & 0xF0));
        h[8] = (byte) (((spec.mapper() >> 8) & 0x0F) | ((spec.submapper() & 0x0F) << 4));
        h[9] = (byte) ((prg.highNibble() & 0x0F) | ((chr.highNibble() & 0x0F) << 4));
        return h;
    }

    /**
     * Write a canvas file. When {@code body} is true, the full zeroed PRG+CHR
     * body is streamed after the header (large!); otherwise only the 16-byte
     * header is written (a capacity declaration).
     */
    public static void write(Path path, Spec spec, boolean body)
            throws IOException {
        byte[] h = header(spec);
        try (OutputStream os = Files.newOutputStream(path)) {
            os.write(h);
            if (body) {
                long remaining = encode(spec.prgBytes()).actualBytes()
                        + encode(spec.chrBytes()).actualBytes();
                byte[] zeros = new byte[65536];
                while (remaining > 0) {
                    int chunk = (int) Math.min(remaining, zeros.length);
                    os.write(zeros, 0, chunk);
                    remaining -= chunk;
                }
            }
        }
    }
}
