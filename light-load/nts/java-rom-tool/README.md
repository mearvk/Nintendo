# NTS Java ROM tool (`java-rom-tool`)

The JVM equivalent of the C reader (`c-ines-reader`), the C++ discriminator
(`cpp-discriminator`), and the C canvas generator (`c-canvas-gen`), combined
into one Java program. It **reads** iNES / NES 2.0 structure and **generates**
new, empty NES 2.0 canvases.

> Like the C/C++ tools, it reads only header-derived structure and emits only
> original (header + zero) bytes. It never reads, extracts, or reproduces game
> content. See [`../../LEGALITY.md`](../../LEGALITY.md).

## Why a Java version — the OOD study

The same domain is modeled three ways on purpose, as a comparative
object-oriented-design exercise:

- **C** — procedural, explicit layout, closest to the hardware.
- **C++** — value types + namespaces, a thin typed layer over the C core.
- **Java** — classes, enums, records, and a builder; the clearest OOD
  expression and a natural fit for JVM build pipelines.

Key OOD choices here:
- `INesHeader` — an **immutable value object** built by a validating static
  factory (`parse`), with a private `Builder`.
- `RomStructure` — composes a header with whole-file facts (trailer vs.
  truncation).
- `Discriminator` — turns a structure into `Condition` **records** and renders
  the `ruth` diagram.
- `CanvasGenerator` — `Spec` / `SizeField` **records** model the request and
  the encoded result.

Studying one well-specified format across three paradigms shows where OOD helps
(modeling, extensibility, testability) and where it costs (indirection,
allocation) — useful background for anyone building commercial tooling in this
space.

## Build & run

```sh
cd nts/java-rom-tool
javac -d out src/com/mearvk/nts/*.java

# structural analysis / verdict:
java -cp out com.mearvk.nts.Main analyze ../../NobunagaAmbition001.nes
java -cp out com.mearvk.nts.Main analyze ../../NobunagaAmbition002.nes --ruth
java -cp out com.mearvk.nts.Main analyze <file> --declare

# emit a new, empty NES 2.0 canvas:
java -cp out com.mearvk.nts.Main canvas out.ineshdr 200 32 5
```

## Cross-language interop (verified)

All three implementations read each other's output and agree. The Java canvas
generator emits a byte-identical header to the C generator — e.g. a 200 MB
request yields `4E 45 53 1A 67 64 52 08 00 FF ...` (PRG rounds up to
`2^25 * 7 = 224 MiB`, CHR = `2^25 = 32 MiB`, both in NES 2.0 exponent notation)
in every language.

## Layout

```
java-rom-tool/
└── src/com/mearvk/nts/
    ├── INesHeader.java       # immutable header model + NES 2.0 size decode
    ├── RomStructure.java     # file-level view: trailer vs. truncation
    ├── Discriminator.java    # 3 axes implied, 5 conditions, ruth diagram
    ├── CanvasGenerator.java  # new empty NES 2.0 canvas emitter
    └── Main.java             # CLI: analyze / canvas
```

`out/` (compiled `.class` files) is git-ignored.
