package com.mearvk.nts;

import java.nio.file.Path;

/**
 * NTS Java CLI — the JVM equivalent of {@code nts-analyze} + {@code nts-canvas}.
 *
 * <pre>
 *   analyze &lt;file.nes&gt; [--ruth|--declare]   structural analysis / verdict
 *   canvas  &lt;out&gt; &lt;prgMB&gt; [chrMB] [mapper] [--body]  emit a new empty NES 2.0 image
 * </pre>
 *
 * Reads structure and generates new, empty images only — never reproduces game
 * content.
 */
public final class Main {

    public static void main(String[] args) throws Exception {
        if (args.length < 1) { usage(); System.exit(2); return; }
        switch (args[0]) {
            case "analyze" -> analyze(args);
            case "canvas"  -> canvas(args);
            default -> { usage(); System.exit(2); }
        }
    }

    private static void analyze(String[] a) throws Exception {
        if (a.length < 2) { usage(); System.exit(2); return; }
        Path path = Path.of(a[1]);
        String mode = a.length >= 3 ? a[2] : "";
        RomStructure rom = RomStructure.read(path);
        Discriminator d = new Discriminator(rom);
        INesHeader h = rom.header();
        String name = path.getFileName().toString();

        if (mode.equals("--ruth")) {
            System.out.print(d.ruthDiagram(name));
        } else if (mode.equals("--declare")) {
            System.out.printf("declared capacity for %s:%n", name);
            System.out.printf("  format : %s%n",
                    h.format() == INesHeader.Format.NES20 ? "NES 2.0" : "iNES");
            System.out.printf("  mapper : %d%n", h.mapper());
            System.out.printf("  PRG    : %.2f MB%s%n",
                    h.prgBytes() / 1048576.0, h.exponentPrg() ? " (exponent)" : "");
            System.out.printf("  CHR    : %.2f MB%s%n",
                    h.chrBytes() / 1048576.0, h.exponentChr() ? " (exponent)" : "");
            System.out.printf("  body   : %s%n",
                    rom.truncated() ? "absent (header-only declaration)" : "present");
        } else {
            System.out.printf("# NTS (Java) structural analysis — %s%n%n", name);
            System.out.printf("format=%s mapper=%d prg=%d B chr=%d B trailer=%d B%n%n",
                    h.format(), h.mapper(), h.prgBytes(), h.chrBytes(),
                    rom.trailerBytes());
            for (Discriminator.Condition c : d.conditions()) {
                System.out.printf("  %-10s %-4s %s%n", c.name(),
                        c.satisfied() ? "yes" : "no", c.rationale());
            }
        }
    }

    private static void canvas(String[] a) throws Exception {
        if (a.length < 3) { usage(); System.exit(2); return; }
        Path out = Path.of(a[1]);
        double prgMb = Double.parseDouble(a[2]);
        double chrMb = 0;
        int mapper = 5;
        boolean body = false;
        boolean chrSet = false;
        for (int i = 3; i < a.length; i++) {
            if (a[i].equals("--body")) body = true;
            else if (!chrSet) { chrMb = Double.parseDouble(a[i]); chrSet = true; }
            else mapper = Integer.parseInt(a[i]);
        }
        CanvasGenerator.Spec spec = new CanvasGenerator.Spec(
                (long) (prgMb * 1048576), (long) (chrMb * 1048576),
                mapper, 0, true);
        CanvasGenerator.write(out, spec, body);
        long ap = CanvasGenerator.encode(spec.prgBytes()).actualBytes();
        long ac = CanvasGenerator.encode(spec.chrBytes()).actualBytes();
        System.out.printf("wrote %s%n", out);
        System.out.printf("  declared PRG : %.2f MB (requested %.2f MB)%n",
                ap / 1048576.0, prgMb);
        System.out.printf("  declared CHR : %.2f MB (requested %.2f MB)%n",
                ac / 1048576.0, chrMb);
        System.out.printf("  mapper       : %d (battery)%n", mapper);
        System.out.printf("  body written : %s%n",
                body ? "yes (full zeroed canvas)" : "no (header-only declaration)");
        System.out.println("  note         : format-capacity canvas; not a runnable ROM.");
    }

    private static void usage() {
        System.err.println("""
            usage:
              nts analyze <file.nes> [--ruth|--declare]
              nts canvas  <out> <prgMB> [chrMB] [mapper] [--body]""");
    }
}
