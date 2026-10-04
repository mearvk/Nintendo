package com.mearvk.nintendo.strategy;

import java.io.IOException;
import java.io.Writer;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.TreeSet;

/**
 * Part B — dynamic strategy evidence.
 *
 * <p>Ingests an execution trace (TSV emitted by an emulator/harness over a ROM
 * the user owns) and writes a Strategy Evidence report (Markdown): per decision
 * context, which code addresses and jump tables were exercised, with coverage
 * counts and explicit confidence caveats. See {@code docs/METHOD.md}.
 */
public final class Evidence {

    private static final class DecisionRec {
        long execs, branches, taken, reads, writes, jtbls;
        final TreeSet<Integer> codeAddrs = new TreeSet<>();
        final TreeSet<Integer> jtblAddrs = new TreeSet<>();
    }

    private Evidence() {}

    /** Produce a report from {@code tracePath}, optionally cross-referencing a static map. */
    public static void fromTrace(Path tracePath, StaticMap mapOrNull, Path outMd)
            throws IOException {
        // LinkedHashMap preserves first-seen decision order for a stable report.
        Map<String, DecisionRec> recs = new LinkedHashMap<>();
        long total = 0;

        for (String raw : Files.readAllLines(tracePath, StandardCharsets.UTF_8)) {
            String s = raw.strip();
            if (s.isEmpty() || s.charAt(0) == '#') continue;
            String[] cols = s.split("\t", -1);
            if (cols.length < 2) continue;
            total++;
            String event = cols[1];
            String pcs = cols.length > 2 ? cols[2] : "-";
            String arg = cols.length > 3 ? cols[3] : "-";
            String dec = cols.length > 4 ? cols[4] : "unknown";
            DecisionRec r = recs.computeIfAbsent(dec, k -> new DecisionRec());
            int pc = 0;
            try { pc = Integer.parseInt(pcs.trim(), 16); } catch (NumberFormatException ignored) {}
            switch (event) {
                case "exec"   -> { r.execs++; r.codeAddrs.add(pc); }
                case "branch" -> { r.branches++; if (arg.contains("taken=1")) r.taken++; }
                case "read"   -> r.reads++;
                case "write"  -> r.writes++;
                case "jtbl"   -> { r.jtbls++; r.jtblAddrs.add(pc); }
                default -> { /* ignore unknown events, forward-compatible */ }
            }
        }

        try (Writer w = Files.newBufferedWriter(outMd, StandardCharsets.UTF_8)) {
            w.write("# Strategy Evidence Report\n\n");
            w.write("Source trace: `" + tracePath + "`  \nTotal events: " + total
                    + "  \nDecision contexts: " + recs.size() + "\n\n");
            w.write("""
                    > This report reflects only the code paths the supplied trace actually
                    > exercised (and, where cross-referenced, a static approximation of code).
                    > It is evidence for a human analysis, not a complete or guaranteed
                    > extraction of the game's strategy. Absence of evidence is not evidence
                    > of absence.

                    """);
            for (Map.Entry<String, DecisionRec> e : recs.entrySet()) {
                DecisionRec r = e.getValue();
                w.write("## Decision: `" + e.getKey() + "`\n\n");
                w.write("- exec events: " + r.execs + " (distinct code addresses: " + r.codeAddrs.size() + ")\n");
                w.write("- branches: " + r.branches + " (taken: " + r.taken + ")\n");
                w.write("- reads: " + r.reads + ", writes: " + r.writes + "\n");
                w.write("- jump-table dispatches: " + r.jtbls + " (distinct tables: " + r.jtblAddrs.size() + ")\n");
                if (!r.jtblAddrs.isEmpty()) {
                    StringBuilder sb = new StringBuilder("- dispatch addresses:");
                    for (int a : r.jtblAddrs) {
                        sb.append(String.format(" %04X", a));
                        if (mapOrNull != null && mapOrNull.inJumpTable(a)) sb.append("(static-jt)");
                    }
                    w.write(sb.append("\n").toString());
                }
                if (!r.codeAddrs.isEmpty())
                    w.write(String.format("- first exercised address under this decision: `%04X`%n", r.codeAddrs.first()));
                w.write("\n");
            }
            w.write("---\nMethod: static structure (Part A) + dynamic trace (Part B). See `docs/METHOD.md`.\n");
        }
    }
}
