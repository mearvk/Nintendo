package com.mearvk.nintendo.editor;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashMap;
import java.util.Map;

/**
 * A character map (.tbl) for NES text: byte &lt;-&gt; glyph in both directions.
 * An unloaded table is the ASCII identity (printable ASCII maps to itself).
 *
 * <p>Part of the mearvk/Nintendo Professional Editor. Ships no copyrighted ROM
 * content; this is a tool for editing a ROM the user already owns.
 *
 * <p>Table format (community-standard "Thingy" style): one {@code HH=glyph}
 * line per entry, {@code #} comment lines ignored.
 */
public final class CharTable {
    private boolean loaded;
    private final Map<Character, Integer> glyphToByte = new HashMap<>();
    private final Map<Integer, Character> byteToGlyph = new HashMap<>();
    private int fill = 0x00;

    /** An empty table (ASCII identity until {@link #load} is called). */
    public CharTable() {}

    /** Load a {@code .tbl} character map. */
    public static CharTable load(Path path) throws IOException {
        CharTable t = new CharTable();
        for (String line : Files.readAllLines(path, StandardCharsets.UTF_8)) {
            if (line.length() < 4 || line.charAt(0) == '#') continue;
            int hi = hex(line.charAt(0)), lo = hex(line.charAt(1));
            if (hi < 0 || lo < 0 || line.charAt(2) != '=') continue;
            int b = hi * 16 + lo;
            char g = line.charAt(3);
            if (g == '\r' || g == '\n') g = ' ';
            t.byteToGlyph.put(b, g);
            t.glyphToByte.putIfAbsent(g, b);
            if (g == ' ') t.fill = b;
        }
        t.loaded = true;
        return t;
    }

    public boolean isLoaded() { return loaded; }
    public int fillByte() { return loaded ? fill : 0x00; }

    /** Encode one glyph to its ROM byte, or -1 if the glyph is unmapped. */
    public int encodeGlyph(char glyph) {
        if (!loaded) return glyph & 0xFF;
        Integer b = glyphToByte.get(glyph);
        return b == null ? -1 : b;
    }

    /** Decode one ROM byte to a glyph ('.' if unmapped / non-printable). */
    public char decodeByte(int b) {
        b &= 0xFF;
        if (!loaded) return (b >= 0x20 && b < 0x7F) ? (char) b : '.';
        Character g = byteToGlyph.get(b);
        return g == null ? '.' : g;
    }

    private static int hex(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }
}
