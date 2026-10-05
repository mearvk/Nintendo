// ============================================================================
// Nintendo Technical Series (NTS) — merit / strategy / components discriminator
// ----------------------------------------------------------------------------
// Thin C++ layer over the C structural reader (ines.h). It classifies the
// PHYSICAL STRUCTURE of an iNES image into three analytic axes and evaluates a
// set of named discriminator CONDITIONS. It never reads, extracts, or
// reproduces game content — only header-derived structural metadata.
//
// The three axes:
//   merit       — intrinsic, measurable structural facts (sizes, banks, hash).
//   strategy    — layout/mapper-driven consequences for how the image is used
//                 (bank-switching surface, CHR source, persistence).
//   components  — the enumerated physical regions (header/trainer/prg/chr).
//
// The five conditions (predicates over structural metadata). These are defined
// here as *structural* interpretations of the developer's taxonomy so the
// logic is concrete and testable; adjust the thresholds in conditions.cpp to
// retune. None of them inspect game content.
//   play        — the image is structurally playable (valid header, regions
//                 fit within the file, PRG present).
//   guarantee   — integrity is guaranteed: computed size == file size exactly
//                 and no truncated region.
//   chapters    — the image is partitioned into multiple switchable PRG banks
//                 (>1), i.e. content is delivered in addressable "chapters".
//   win         — a terminal/complete configuration: a known mapper, a battery
//                 for persisted progress, and a consistent layout.
//   chemistry   — cross-region coherence: PRG and CHR (or CHR-RAM) balance in a
//                 way the mapper supports (no CHR banks iff CHR-RAM, etc.).
// ============================================================================
#ifndef NTS_DISCRIMINATOR_HPP
#define NTS_DISCRIMINATOR_HPP

extern "C" {
#include "../c-ines-reader/ines.h"
}

#include <string>
#include <vector>
#include <cstdint>

namespace nts {

struct Metric {
    std::string key;
    std::string value;
    std::string note;
};

struct Condition {
    std::string name;      // play / guarantee / chapters / win / chemistry
    bool        satisfied;
    std::string rationale; // why, in structural terms
};

struct Axis {
    std::string          name;    // merit / strategy / components
    std::vector<Metric>  metrics;
};

struct Analysis {
    std::string            source;   // file name only (no bytes)
    nts_rom                rom;
    std::vector<Axis>      axes;      // merit, strategy, components
    std::vector<Condition> conditions;
    bool                   valid;
    std::string            error;
};

// Build the full analysis from a parsed ROM structure. `source` is a label
// (filename), used for reporting only.
Analysis analyze(const nts_rom& rom, const std::string& source);

// Convenience: read a file's structure and analyze it.
Analysis analyze_file(const std::string& path);

// The three axis builders (exposed for unit testing / reuse).
Axis build_merit(const nts_rom& rom);
Axis build_strategy(const nts_rom& rom);
Axis build_components(const nts_rom& rom);

// The five condition evaluators.
std::vector<Condition> evaluate_conditions(const nts_rom& rom);

// Render helpers.
std::string to_markdown(const Analysis& a);   // human report
std::string to_tsv(const Analysis& a);         // machine-ingestible line(s)

// The "ruth" diagram: encloses the five conditions
// (play / guarantee / chapters / win / chemistry) in a single labelled box,
// each shown with its satisfied-state glyph. "ruth" is the chosen name for
// this five-cell enclosure of the discriminator's verdict.
std::string to_ruth_diagram(const Analysis& a);

} // namespace nts

#endif // NTS_DISCRIMINATOR_HPP
