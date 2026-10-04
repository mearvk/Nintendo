package com.mearvk.nintendo.editor;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

/**
 * Command-line NES ROM text/menu editor (Java).
 *
 * <p>Part of the mearvk/Nintendo Professional Editor. Shares the edit-script
 * grammar with the C / C++ / SLeeLa implementations:
 * <pre>
 *   # comment
 *   table &lt;path.tbl&gt;
 *   find  &lt;offset-hex&gt; &lt;len&gt; &lt;text&gt;
 *   set   &lt;offset-hex&gt; &lt;len&gt; &lt;text&gt;
 *   menu  &lt;name&gt; &lt;offset-hex&gt; &lt;len&gt; &lt;text&gt;
 * </pre>
 *
 * <p>Usage:
 * <pre>
 *   NesEditCli info    &lt;rom.nes&gt;
 *   NesEditCli dump    &lt;rom.nes&gt; &lt;offset-hex&gt; &lt;len&gt; [table.tbl]
 *   NesEditCli apply   &lt;in.nes&gt; &lt;script.edit&gt; &lt;out.nes&gt;
 *   NesEditCli recurve &lt;rom.nes&gt;
 *   NesEditCli refresh &lt;in.nes&gt; &lt;out.nes&gt;
 * </pre>
 */
public final class NesEditCli {

    public static void main(String[] args) {
        if (args.length < 1) { usage(); System.exit(2); }
        try {
            switch (args[0]) {
                case "info"    -> { requireArgs(args, 2); System.exit(info(Path.of(args[1]))); }
                case "dump"    -> { requireArgs(args, 4); System.exit(dump(args)); }
                case "apply"   -> { requireArgs(args, 4); System.exit(apply(Path.of(args[1]), Path.of(args[2]), Path.of(args[3]))); }
                case "recurve" -> { requireArgs(args, 2); System.exit(recurve(Path.of(args[1]))); }
                case "refresh" -> { requireArgs(args, 3); System.exit(refresh(Path.of(args[1]), Path.of(args[2]))); }
                default        -> { usage(); System.exit(2); }
            }
        } catch (NesException | IOException e) {
            System.err.println("error: " + e.getMessage());
            System.exit(1);
        }
    }

    private static int info(Path path) throws IOException, NesException {
        NesRom rom = NesRom.load(path);
        System.out.printf("iNES image: %s%n", path);
        System.out.printf("  size      : %d bytes%n", rom.size());
        System.out.printf("  PRG banks : %d (%d bytes @ 0x%X)%n", rom.prgBanks(), rom.prgSize(), rom.prgOffset());
        System.out.printf("  CHR banks : %d (%d bytes @ 0x%X)%n", rom.chrBanks(), rom.chrSize(), rom.chrOffset());
        System.out.printf("  mapper    : %d%n", rom.mapper());
        System.out.printf("  trainer   : %s%n", rom.hasTrainer() ? "yes" : "no");
        return 0;
    }

    private static int dump(String[] args) throws IOException, NesException {
        NesRom rom = NesRom.load(Path.of(args[1]));
        CharTable tbl = args.length >= 5 ? CharTable.load(Path.of(args[4])) : new CharTable();
        int off = Integer.parseInt(args[2], 16);
        int len = Integer.parseInt(args[3]);
        System.out.printf("0x%X [%d] = \"%s\"%n", off, len, rom.decode(off, len, tbl));
        return 0;
    }

    private static int apply(Path in, Path script, Path out) throws IOException, NesException {
        NesRom rom = NesRom.load(in);
        CharTable tbl = new CharTable();
        int lineno = 0, edits = 0;
        for (String raw : Files.readAllLines(script, StandardCharsets.UTF_8)) {
            lineno++;
            String line = raw.strip();
            if (line.isEmpty() || line.charAt(0) == '#') continue;
            String kw = firstWord(line);
            try {
                switch (kw) {
                    case "table" -> tbl = CharTable.load(Path.of(tokens(line, 2, 1).get(1)));
                    case "find" -> {
                        List<String> t = tokens(line, 4, 3);
                        if (t.size() < 4) throw new NesException("find needs offset len text");
                        if (!rom.expectText(Integer.parseInt(t.get(1), 16), Integer.parseInt(t.get(2)), t.get(3), tbl))
                            throw new NesException("find guard did not match");
                    }
                    case "set" -> {
                        List<String> t = tokens(line, 4, 3);
                        if (t.size() < 4) throw new NesException("set needs offset len text");
                        rom.setText(Integer.parseInt(t.get(1), 16), Integer.parseInt(t.get(2)), t.get(3), tbl);
                        edits++;
                    }
                    case "menu" -> {
                        List<String> t = tokens(line, 5, 4);
                        if (t.size() < 5) throw new NesException("menu needs name offset len text");
                        rom.setText(Integer.parseInt(t.get(2), 16), Integer.parseInt(t.get(3)), t.get(4), tbl);
                        System.out.printf("menu '%s' set @ %s%n", t.get(1), t.get(2));
                        edits++;
                    }
                    default -> throw new NesException("unknown directive '" + kw + "'");
                }
            } catch (NesException | IOException | NumberFormatException e) {
                System.err.printf("apply:%d: %s%n", lineno, e.getMessage());
                System.err.printf("apply: aborted; %s not written%n", out);
                return 1;
            }
        }
        rom.save(out);
        System.out.printf("applied %d edit(s) -> %s%n", edits, out);
        return 0;
    }

    private static int recurve(Path path) throws IOException, NesException {
        NesRom rom = NesRom.load(path);
        NesRom.Report rep = rom.recurve();
        System.out.printf("recurve: integrity audit of %s%n", path);
        for (NesRom.Finding f : rep.findings) {
            System.out.printf("  [%-5s] %-8s %s%n",
                    NesRom.severityStr(f.severity()), f.check(), f.message());
        }
        System.out.printf("  fingerprint fnv1a64:%016X%n", rom.fingerprint());
        System.out.printf("verdict: %s%n", switch (rep.worst) {
            case OK -> "HEALTHY";
            case WARN -> "HEALTHY (with warnings)";
            case ERROR -> "DAMAGED";
        });
        return rep.worst == NesRom.Severity.ERROR ? 1 : 0;
    }

    private static int refresh(Path in, Path out) throws IOException, NesException {
        NesRom rom = NesRom.load(in);
        long fp = rom.refresh(out);
        System.out.printf("refreshed %s -> %s (%d bytes)%n", in, out, rom.size());
        System.out.printf("  fingerprint fnv1a64:%016X%n", fp);
        System.out.printf("  provenance  %s.provenance%n", out);
        return 0;
    }

    /** First whitespace-delimited word. */
    private static String firstWord(String line) {
        int sp = indexOfWs(line, 0);
        return sp < 0 ? line : line.substring(0, sp);
    }

    /**
     * Split into up to {@code max} tokens; the token at index {@code textFrom}
     * captures the rest of the line, with optional surrounding quotes removed so
     * replacement text may contain spaces.
     */
    private static List<String> tokens(String line, int max, int textFrom) {
        List<String> out = new ArrayList<>();
        int i = 0, n = line.length();
        while (i < n && out.size() < max) {
            while (i < n && (line.charAt(i) == ' ' || line.charAt(i) == '\t')) i++;
            if (i >= n) break;
            if (out.size() == textFrom) {
                String rest = line.substring(i);
                if (!rest.isEmpty() && rest.charAt(0) == '"') {
                    rest = rest.substring(1);
                    int q = rest.indexOf('"');
                    if (q >= 0) rest = rest.substring(0, q);
                }
                out.add(rest);
                break;
            }
            int start = i;
            while (i < n && line.charAt(i) != ' ' && line.charAt(i) != '\t') i++;
            out.add(line.substring(start, i));
        }
        return out;
    }

    private static int indexOfWs(String s, int from) {
        for (int i = from; i < s.length(); i++) {
            char c = s.charAt(i);
            if (c == ' ' || c == '\t') return i;
        }
        return -1;
    }

    private static void requireArgs(String[] args, int n) {
        if (args.length < n) { usage(); System.exit(2); }
    }

    private static void usage() {
        System.err.println("""
            NesEditCli -- NES ROM text/menu editor (Java)
            usage:
              NesEditCli info    <rom.nes>
              NesEditCli dump    <rom.nes> <offset-hex> <len> [table.tbl]
              NesEditCli apply   <in.nes> <script.edit> <out.nes>
              NesEditCli recurve <rom.nes>
              NesEditCli refresh <in.nes> <out.nes>""");
    }

    private NesEditCli() {}
}
