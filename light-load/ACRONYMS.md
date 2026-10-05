# Acronyms & terms used in this toolkit

Every acronym that appears across the `light-load` NES Technical Series tooling
and docs, with its technology era, year of origin (first appearance /
standardisation), and a plain definition. Grouped by domain.

> "Year" is the earliest widely-cited date for the term or standard. Where a
> format evolved, the year marks its introduction. Dates for community formats
> (NES 2.0, UNROM-512, MSU-1) are approximate first-publication dates.

## Platform & console

| Acronym | Era | Year | Definition |
|---|---|---|---|
| **NES** | 8-bit console | 1983 (JP Famicom) / 1985 (NA) | Nintendo Entertainment System — the 8-bit console (Famicom in Japan) this toolkit targets. |
| **CPU** | Microprocessor | 1970s (term); NES uses a 1975-era core | Central Processing Unit — the NES's processor (a Ricoh 2A03, a 6502 derivative). |
| **VS** | Arcade hardware | 1984 | Nintendo "VS. System" — arcade boards running NES-derived hardware; one of the header's console-type flags. |
| **PlayChoice** | Arcade hardware | 1986 | PlayChoice-10 — coin-op cabinets running NES cartridges; another header console-type flag. |

## File container formats

| Acronym | Era | Year | Definition |
|---|---|---|---|
| **iNES** | Emulation era | 1996 | The de-facto NES ROM container (named after Marat Fayzullin's *iNES* emulator): a 16-byte header + PRG/CHR data. |
| **NES 2.0** | Emulation era | 2006 | Backward-compatible extension of the iNES header; adds submappers, large (exponent) sizes, and RAM-shift fields. The large-memory format this toolkit decodes/generates. |
| **IPS** | Patch format | 1990s | International Patching System — a simple binary-diff/patch format for ROM hacks (ships changes, not the game). |
| **BPS** | Patch format | 2010s | Binary Patch System — a modern, checksum-verified successor to IPS. |

## Memory regions & chips

| Acronym | Era | Year | Definition |
|---|---|---|---|
| **ROM** | Memory | 1960s (term) | Read-Only Memory — fixed, non-writable storage; the game data on a cartridge. |
| **RAM** | Memory | 1960s (term) | Random-Access Memory — writable working memory. |
| **PRG** | NES cartridge | 1983 | "Program" ROM/RAM — the region holding the game's executable 6502 code and data. |
| **CHR** | NES cartridge | 1983 | "Character" ROM/RAM — the region holding graphics tile (pattern) data. CHR-ROM is fixed; CHR-RAM is written at runtime. |
| **WRAM** | NES cartridge | 1986 | Work RAM — on-cartridge RAM, often battery-backed for saved games (also called PRG-RAM / SRAM). |
| **MMC** | NES cartridge | 1986 | Memory Management Controller — Nintendo's family of on-cartridge mapper chips that bank-switch PRG/CHR to exceed the CPU's visible window. |
| **MMC5** | NES cartridge | 1988 | The most capable Nintendo MMC mapper (mapper 5); large PRG/CHR banking + extra features. Used by `NobunagaAmbition002.nes` and the canvas default. |
| **UNROM-512** | Homebrew | 2012 | A modern community mapper (mapper 30) based on the classic UNROM board, expanded to 512 KB with flash/self-write support; popular for homebrew that runs on emulators and real carts. |
| **MSU-1** | Enhancement | 2012 | An emulator-side expansion originally for SNES (ported in spirit to other systems) providing CD-quality streamed audio and data; cited as an example of ROM-space-driven enhancement. |

## Units

| Acronym | Era | Year | Definition |
|---|---|---|---|
| **KB / MB** | Computing | 1960s–70s | Kilobyte / Megabyte — decimal-ish size units as commonly written (10^3 / 10^6, or loosely binary). |
| **KiB / MiB** | Computing | 1998 (IEC) | Kibibyte / Mebibyte — the unambiguous binary units (2^10 = 1024 B, 2^20 = 1,048,576 B) used for exact region sizes here. |

## Software / tooling (this project & general)

| Acronym | Era | Year | Definition |
|---|---|---|---|
| **NTS** | This project | 2026 | Nintendo Technical Series — the name of this toolkit's structural reader + discriminator. |
| **CLI** | Software | 1960s (term) | Command-Line Interface — the `nts-analyze` / `nts-canvas` programs. |
| **TSV** | Data format | 1970s (convention) | Tab-Separated Values — the machine-ingestible output of `nts-analyze --tsv`. |
| **HD** | Display/enhancement | 2000s | High-Definition — context: emulator "HD packs" that overlay high-resolution art on 8-bit games. |
| **FNV** | Hashing | 1991 | Fowler–Noll–Vo hash — the style of multiplicative accumulator used for the toolkit's header fingerprint (used as a stable fingerprint, not for security). |

---

### Project-specific terms (not acronyms, for reference)

- **merit / strategy / components** — the three analytic axes the discriminator
  classifies a ROM structure along.
- **play / guarantee / chapters / win / chemistry** — the five discriminator
  conditions.
- **ruth** — the five-cell diagram that encloses those five conditions
  (`nts-analyze --ruth`).
- **trailer** — bytes present *past* the last declared iNES region (benign).
- **truncated** — the declared layout is *larger* than the file (corruption).
- **exponent notation** — the NES 2.0 size encoding `2^E · (M·2+1)` that lets a
  header declare very large PRG/CHR footprints.
