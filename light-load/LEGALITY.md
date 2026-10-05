# Legality of modifying old NES ROMs — what else needs to be known

> **Not legal advice.** I'm not a lawyer, and this is a plain-language
> engineering summary, not counsel. Copyright is jurisdiction-specific; these
> are mostly US points, and *Nobunaga's Ambition* is a Japanese Koei work, so
> other regimes (Japan, EU) differ. For anything you act on, consult a
> qualified IP attorney. Sources are linked inline. Content was rephrased for
> compliance with licensing restrictions.

## The one-sentence version

Age does not free an NES ROM: a 1980s game is still under copyright for
decades, so *modifying and distributing* someone else's game image remains an
infringement risk — but *analyzing* structure, *privately* modifying a copy you
own, and *authoring entirely new* games on NES hardware are the defensible
lanes.

## 1. "40+ years old" does not mean public domain

There is no "abandonware" exception in law. A game staying under copyright even
when the publisher no longer sells it is the consistent position across
reference sources, including the
[Wikipedia abandonware entry](https://en.wikipedia.org/wiki/Abandonware),
which notes that distributing out-of-print games is still treated as piracy
because the copyright is not actually abandoned. US corporate-authored works
are generally protected for 95 years from publication, so late-1980s titles
have most of their term left.

## 2. What the NES case law actually supports (developer-friendly)

Two decisions are the practical footing for a *structure-only, non-distributing*
toolkit:

- **Reverse engineering for interoperability can be fair use** —
  [*Atari Games Corp. v. Nintendo of America Inc.*, 975 F.2d 832 (Fed. Cir.
  1992)](https://www.copyright.gov/fair-use/summaries/atari-nintendo-fedcir1992.pdf).
  Intermediate copying to understand the unprotected ideas in a program can
  qualify as fair use. (Atari lost on other facts — it had obtained Nintendo's
  code improperly — not because studying the format was itself illegal.)
- **User-side modification that creates no new fixed copy is not a derivative
  work** —
  [*Lewis Galoob Toys, Inc. v. Nintendo of America, Inc.*, 964 F.2d 965 (9th
  Cir. 1992)](https://en.wikipedia.org/wiki/Lewis_Galoob_Toys,_Inc._v._Nintendo_of_America,_Inc.).
  The Game Genie altered play on the fly without producing a new permanent copy
  of the game, so it did not infringe the derivative-work right.

## 3. Modifying a ROM: where the lines fall

- **Analyzing header/structure** (what this toolkit does): closest to the
  *Atari* interoperability zone; no game content is copied or emitted.
- **Modifying a copy you own, kept private**: lowest exposure; the risky act in
  copyright is *distribution*, not private tinkering. Owning a cartridge does
  **not**, by itself, grant the right to download a ROM of it — those are
  separate rights.
- **Distributing a modified ROM (a "ROM hack")**: still ships the original
  copyrighted code inside the patched image, so it carries the original
  copyright risk. Distributing only a **patch file** (IPS/BPS) that contains no
  original game bytes is a common harm-reduction practice in the community —
  but it is a *practice*, not a settled legal safe harbor.
- **Emulator-side enhancement** (HD packs, widescreen overlays): rendering
  modern assets *over* a game at runtime without creating a new fixed copy
  leans on the *Galoob* principle. The enhancement assets must be your own
  original work.

## 4. The clean lane: author your own

The strongest, lowest-risk path is to build **new, original** games on NES
hardware/formats — your own code, art, and music — targeting modern
large-memory mappers (e.g. MMC5, or UNROM-512 / Mapper 30 used by homebrew
toolchains). Nothing is copied; the result is wholly ownable IP that runs on
emulators and flash carts alike. This is exactly what the "author-your-own-ROM"
direction of this project is for.

## 5. What you *do* own

- Your **analysis and commentary** (the NTS reports, the merit/strategy/
  components framework, the ruth diagram) — original authorship.
- Your **tools** (the C reader, the C++ discriminator) — licensable however you
  choose.
- Any **original game** you create. Facts you extract (mapper number, bank
  counts) are not copyrightable, but your expression, selection, and
  arrangement of them is.

You never acquire rights in the underlying game by studying it, however deeply.
Ownership stays with its author; what you own is your own output *about* or
*alongside* it.

## 6. First-sale, briefly

Buying a used cartridge at a flea market, yard sale, or swap meet is lawful
under the **first-sale doctrine**: once a copy is sold with authorization, that
physical copy can be resold. First sale covers the *physical object* — it does
not authorize making or downloading digital ROM copies of it.

---

### Sources

- Atari Games Corp. v. Nintendo of America Inc., 975 F.2d 832 (Fed. Cir. 1992) —
  US Copyright Office fair-use summary:
  <https://www.copyright.gov/fair-use/summaries/atari-nintendo-fedcir1992.pdf>
- Lewis Galoob Toys, Inc. v. Nintendo of America, Inc. (9th Cir. 1992):
  <https://en.wikipedia.org/wiki/Lewis_Galoob_Toys,_Inc._v._Nintendo_of_America,_Inc.>
- Abandonware (overview of its non-status in law):
  <https://en.wikipedia.org/wiki/Abandonware>
