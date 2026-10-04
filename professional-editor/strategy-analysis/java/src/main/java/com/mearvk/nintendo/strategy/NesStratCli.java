package com.mearvk.nintendo.strategy;

import java.io.IOException;
import java.nio.file.Path;

/**
 * NES strategy analysis CLI (Java).
 *
 * <p>Part of the mearvk/Nintendo Professional Editor. Shares the method and
 * file formats with the C / C++ / SLeeLa implementations ({@code docs/METHOD.md}).
 *
 * <pre>
 *   NesStratCli static   &lt;rom.nes&gt; &lt;out.map.tsv&gt;
 *   NesStratCli evidence &lt;trace.tsv&gt; &lt;out.evidence.md&gt; [rom.nes]
 *   NesStratCli full      &lt;rom.nes&gt; &lt;trace.tsv&gt; &lt;out-prefix&gt;
 * </pre>
 */
public final class NesStratCli {

    public static void main(String[] args) {
        if (args.length < 1) { usage(); System.exit(2); }
        try {
            switch (args[0]) {
                case "static" -> {
                    require(args, 3);
                    StaticMap m = StaticMap.analyzeFile(Path.of(args[1]));
                    m.writeTsv(Path.of(args[2]));
                    long code = m.spans().stream().filter(s -> s.kind() == StaticMap.SpanKind.CODE).count();
                    long jt = m.spans().stream().filter(s -> s.kind() == StaticMap.SpanKind.JUMPTABLE).count();
                    System.out.printf("static: reset=%04X nmi=%04X irq=%04X; %d code blocks, %d jump tables -> %s%n",
                            m.resetVec(), m.nmiVec(), m.irqVec(), code, jt, args[2]);
                }
                case "evidence" -> {
                    require(args, 3);
                    StaticMap m = args.length >= 4 ? StaticMap.analyzeFile(Path.of(args[3])) : null;
                    Evidence.fromTrace(Path.of(args[1]), m, Path.of(args[2]));
                    System.out.println("evidence: " + args[2]);
                }
                case "full" -> {
                    require(args, 4);
                    StaticMap m = StaticMap.analyzeFile(Path.of(args[1]));
                    Path mapp = Path.of(args[3] + ".map.tsv");
                    Path evp = Path.of(args[3] + ".evidence.md");
                    m.writeTsv(mapp);
                    Evidence.fromTrace(Path.of(args[2]), m, evp);
                    System.out.println("full: " + mapp + " + " + evp);
                }
                default -> { usage(); System.exit(2); }
            }
        } catch (NesException | IOException e) {
            System.err.println("error: " + e.getMessage());
            System.exit(1);
        }
    }

    private static void require(String[] args, int n) {
        if (args.length < n) { usage(); System.exit(2); }
    }

    private static void usage() {
        System.err.println("""
            NesStratCli -- NES ROM strategy analysis (Java)
            usage:
              NesStratCli static   <rom.nes> <out.map.tsv>
              NesStratCli evidence <trace.tsv> <out.evidence.md> [rom.nes]
              NesStratCli full      <rom.nes> <trace.tsv> <out-prefix>""");
    }

    private NesStratCli() {}
}
