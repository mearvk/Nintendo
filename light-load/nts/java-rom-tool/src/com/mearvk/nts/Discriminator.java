package com.mearvk.nts;

import java.util.ArrayList;
import java.util.List;

/**
 * NTS discriminator — classifies a parsed {@link RomStructure} along three axes
 * (merit / strategy / components) and evaluates the five conditions
 * (play / guarantee / chapters / win / chemistry), rendered as the "ruth"
 * diagram. Operates purely on structural metadata; no game content is read.
 *
 * <p>OOD note: the five conditions are modeled as {@link Condition} value
 * objects produced by small, independently testable methods — the Java
 * counterpart to the C++ free functions in {@code cpp-discriminator}.
 */
public final class Discriminator {

    /** A single evaluated condition. */
    public record Condition(String name, boolean satisfied, String rationale) {}

    private final RomStructure rom;

    public Discriminator(RomStructure rom) { this.rom = rom; }

    public List<Condition> conditions() {
        List<Condition> out = new ArrayList<>();
        INesHeader h = rom.header();

        boolean fits = !rom.truncated();
        boolean play = (rom.prgBanks() > 0) && fits;
        out.add(new Condition("play", play, play
                ? "valid header, PRG present, regions fit within the image"
                : "missing PRG or declared regions overflow the file (truncated)"));

        boolean guarantee = rom.fileSize() != 0 && !rom.truncated();
        out.add(new Condition("guarantee", guarantee, guarantee
                ? (rom.trailerBytes() > 0
                    ? "layout intact; " + rom.trailerBytes()
                        + "-byte trailer present but not truncated"
                    : "computed layout equals file size exactly")
                : "declared regions exceed the file (truncated / corrupt)"));

        boolean chapters = rom.prgBanks() > 1;
        out.add(new Condition("chapters", chapters, chapters
                ? rom.prgBanks() + " PRG banks: content partitioned into chapters"
                : "single PRG bank: no chapter partitioning"));

        boolean win = (h.mapper() != 0 || rom.prgBanks() >= 1)
                && h.battery() && guarantee;
        out.add(new Condition("win", win, win
                ? "mapped, battery-backed, and size-consistent: completable"
                : "not terminal (needs mapper + battery + exact/trailered size)"));

        boolean chrCoherent = (h.usesChrRam() && rom.chrBanks() == 0)
                || (!h.usesChrRam() && rom.chrBanks() > 0);
        boolean chemistry = chrCoherent && fits;
        out.add(new Condition("chemistry", chemistry, chemistry
                ? "CHR source and bank count are mutually consistent"
                : "CHR source/bank mismatch — regions do not cohere"));

        return out;
    }

    /** The five-cell "ruth" diagram as text. */
    public String ruthDiagram(String source) {
        List<Condition> cs = conditions();
        final int W = 48;
        StringBuilder sb = new StringBuilder();
        sb.append('+').append("-".repeat(W)).append("+\n");
        sb.append('|').append(center("r u t h", W)).append("|\n");
        sb.append('|').append(center(source, W)).append("|\n");
        sb.append('+').append("-".repeat(W)).append("+\n");
        sb.append("| ").append(cell(cs, "play")).append("    ")
          .append(cell(cs, "guarantee")).append(" |\n");
        sb.append('|').append(" ".repeat(W)).append("|\n");
        sb.append("| ").append(cell(cs, "chapters")).append("    ")
          .append(cell(cs, "win")).append(" |\n");
        sb.append('|').append(" ".repeat(W)).append("|\n");
        sb.append('|').append(center(cell(cs, "chemistry"), W)).append("|\n");
        sb.append('+').append("-".repeat(W)).append("+\n");
        sb.append("  legend: [x] satisfied   [ ] not satisfied\n");
        return sb.toString();
    }

    private static String cell(List<Condition> cs, String name) {
        String g = "[?]";
        for (Condition c : cs) {
            if (c.name().equals(name)) { g = c.satisfied() ? "[x]" : "[ ]"; break; }
        }
        String s = g + " " + name;
        return s.length() >= 20 ? s : s + " ".repeat(20 - s.length());
    }

    private static String center(String s, int width) {
        if (s.length() >= width) return s.substring(0, width);
        int pad = width - s.length();
        return " ".repeat(pad / 2) + s + " ".repeat(pad - pad / 2);
    }
}
