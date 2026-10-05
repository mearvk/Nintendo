// ============================================================================
// NTS discriminator — axis builders, condition evaluators, and renderers.
// Operates purely on structural metadata from nts_rom. No content inspection.
// ============================================================================
#include "discriminator.hpp"

#include <sstream>
#include <iomanip>

namespace nts {

namespace {

std::string u(uint32_t v) { return std::to_string(v); }

std::string kib(uint64_t bytes) {
    std::ostringstream os;
    if (bytes >= (1024ull * 1024ull))
        os << (bytes / (1024ull * 1024ull)) << " MiB";
    else
        os << (bytes / 1024ull) << " KiB";
    return os.str();
}

std::string u64(uint64_t v) { return std::to_string(v); }

std::string hex32(uint32_t v) {
    std::ostringstream os;
    os << "0x" << std::hex << std::uppercase << std::setw(8)
       << std::setfill('0') << v;
    return os.str();
}

Condition mk(const std::string& n, bool ok, const std::string& why) {
    return Condition{ n, ok, why };
}

} // namespace

// ---- merit: intrinsic measurable facts ------------------------------------
Axis build_merit(const nts_rom& rom) {
    Axis ax; ax.name = "merit";
    ax.metrics.push_back({ "format",
        rom.format == NTS_FMT_NES20 ? "NES 2.0" : "iNES", "container variant" });
    ax.metrics.push_back({ "prg_banks", u(rom.prg_banks),
        kib(rom.prg_bytes) + " of program ROM" });
    ax.metrics.push_back({ "chr_banks", u(rom.chr_banks),
        rom.uses_chr_ram ? "CHR-RAM (no CHR-ROM banks)"
                         : kib(rom.chr_bytes) + " of character ROM" });
    if (rom.format == NTS_FMT_NES20) {
        if (rom.exponent_prg)
            ax.metrics.push_back({ "prg_size_notation", "exponent",
                "PRG size encoded in NES 2.0 exponent form (large-memory)" });
        if (rom.exponent_chr)
            ax.metrics.push_back({ "chr_size_notation", "exponent",
                "CHR size encoded in NES 2.0 exponent form (large-memory)" });
        ax.metrics.push_back({ "prg_ram", kib(rom.prg_ram_bytes),
            "NES 2.0 PRG-RAM (work + save)" });
        ax.metrics.push_back({ "chr_ram", kib(rom.chr_ram_bytes),
            "NES 2.0 CHR-RAM" });
    }
    ax.metrics.push_back({ "file_size", kib(rom.file_size),
        "actual bytes on disk" });
    ax.metrics.push_back({ "computed_size", kib(rom.computed_size),
        "header + trainer + prg + chr" });
    ax.metrics.push_back({ "trailer", u64((uint64_t)rom.trailer_bytes) + " B",
        rom.trailer_bytes ? "bytes past the last declared region" : "none" });
    ax.metrics.push_back({ "header_fingerprint",
        hex32(nts_region_sum(rom.raw_header, 0, NTS_INES_HEADER_SIZE)),
        "FNV-style accumulator over the 16-byte header" });
    return ax;
}

// ---- strategy: layout/mapper consequences for use -------------------------
Axis build_strategy(const nts_rom& rom) {
    Axis ax; ax.name = "strategy";
    ax.metrics.push_back({ "mapper", u(rom.mapper),
        "memory-mapper board number (determines bank-switching surface)" });
    if (rom.format == NTS_FMT_NES20)
        ax.metrics.push_back({ "submapper", u(rom.submapper),
            "NES 2.0 board sub-variant" });
    ax.metrics.push_back({ "mirroring", nts_mirroring_str(rom.mirroring),
        "nametable arrangement; drives scroll strategy" });
    ax.metrics.push_back({ "chr_source",
        rom.uses_chr_ram ? "CHR-RAM" : "CHR-ROM",
        rom.uses_chr_ram ? "tiles streamed into RAM at runtime"
                         : "tiles fixed in ROM" });
    ax.metrics.push_back({ "persistence",
        rom.has_battery ? "battery-backed WRAM" : "volatile",
        rom.has_battery ? "progress can be saved" : "no save hardware" });
    ax.metrics.push_back({ "bank_switch_surface",
        rom.prg_banks > 1 ? "multi-bank" : "flat",
        rom.prg_banks > 1 ? "code/data paged through the mapper"
                          : "single fixed PRG window" });
    return ax;
}

// ---- components: enumerated physical regions ------------------------------
Axis build_components(const nts_rom& rom) {
    Axis ax; ax.name = "components";
    for (uint32_t i = 0; i < rom.region_count; ++i) {
        const nts_region& r = rom.regions[i];
        std::ostringstream v;
        v << hex32(r.offset) << " +" << kib(r.length);
        ax.metrics.push_back({ r.name, v.str(),
            "offset " + hex32(r.offset) + ", length " + u(r.length) + " B" });
    }
    return ax;
}

// ---- the five conditions --------------------------------------------------
std::vector<Condition> evaluate_conditions(const nts_rom& rom) {
    std::vector<Condition> out;

    // play: valid header + PRG present + not truncated.
    bool fits = !rom.truncated;
    bool play = (rom.prg_banks > 0) && fits;
    out.push_back(mk("play", play,
        play ? "valid header, PRG present, regions fit within the image"
             : "missing PRG or declared regions overflow the file (truncated)"));

    // guarantee: integrity holds. A benign trailer (file larger than the
    // declared layout) no longer counts as a violation; only truncation or a
    // completely sizeless file does. The trailer size is reported separately.
    bool guarantee = (rom.file_size != 0) && !rom.truncated;
    out.push_back(mk("guarantee", guarantee,
        guarantee
            ? (rom.trailer_bytes
                ? ("layout intact; " + std::to_string(rom.trailer_bytes) +
                   "-byte trailer present but not truncated")
                : std::string("computed layout equals file size exactly"))
            : "declared regions exceed the file (truncated / corrupt)"));

    // chapters: content delivered across multiple switchable PRG banks.
    bool chapters = rom.prg_banks > 1;
    out.push_back(mk("chapters", chapters,
        chapters ? (u(rom.prg_banks) +
                    " PRG banks: content is partitioned into addressable chapters")
                 : "single PRG bank: no chapter partitioning"));

    // win: terminal/complete config — known mapper + persistence + consistency.
    bool win = (rom.mapper != 0 || rom.prg_banks >= 1) &&
               rom.has_battery && guarantee;
    out.push_back(mk("win", win,
        win ? "mapped, battery-backed, and size-consistent: a completable configuration"
            : "not a terminal configuration (needs mapper + battery + exact size)"));

    // chemistry: cross-region coherence between CHR source and bank count.
    bool chr_coherent = (rom.uses_chr_ram && rom.chr_banks == 0) ||
                        (!rom.uses_chr_ram && rom.chr_banks > 0);
    bool chemistry = chr_coherent && fits;
    out.push_back(mk("chemistry", chemistry,
        chemistry ? "CHR source and bank count are mutually consistent"
                  : "CHR source/bank mismatch — regions do not cohere"));

    return out;
}

// ---- assembly -------------------------------------------------------------
Analysis analyze(const nts_rom& rom, const std::string& source) {
    Analysis a;
    a.source = source;
    a.rom = rom;
    a.valid = true;
    a.axes.push_back(build_merit(rom));
    a.axes.push_back(build_strategy(rom));
    a.axes.push_back(build_components(rom));
    a.conditions = evaluate_conditions(rom);
    return a;
}

Analysis analyze_file(const std::string& path) {
    Analysis a;
    a.source = path;
    nts_status st = nts_read_file(path.c_str(), &a.rom);
    if (st != NTS_OK) {
        a.valid = false;
        a.error = nts_status_str(st);
        return a;
    }
    // slice to basename for reporting (label only; no bytes)
    std::string base = path;
    size_t slash = base.find_last_of("/\\");
    if (slash != std::string::npos) base = base.substr(slash + 1);
    return analyze(a.rom, base);
}

// ---- renderers ------------------------------------------------------------
std::string to_markdown(const Analysis& a) {
    std::ostringstream os;
    os << "# NTS structural analysis — " << a.source << "\n\n";
    if (!a.valid) {
        os << "**Invalid:** " << a.error << "\n";
        return os.str();
    }
    for (const Axis& ax : a.axes) {
        os << "## " << ax.name << "\n\n";
        os << "| metric | value | note |\n|---|---|---|\n";
        for (const Metric& m : ax.metrics)
            os << "| " << m.key << " | " << m.value << " | " << m.note << " |\n";
        os << "\n";
    }
    os << "## conditions\n\n";
    os << "| condition | satisfied | rationale |\n|---|---|---|\n";
    for (const Condition& c : a.conditions)
        os << "| " << c.name << " | " << (c.satisfied ? "yes" : "no")
           << " | " << c.rationale << " |\n";
    os << "\n";
    return os.str();
}

std::string to_tsv(const Analysis& a) {
    std::ostringstream os;
    // one line per condition: source, condition, satisfied, mapper, prg, chr
    for (const Condition& c : a.conditions) {
        os << a.source << '\t' << c.name << '\t'
           << (c.satisfied ? 1 : 0) << '\t'
           << a.rom.mapper << '\t'
           << a.rom.prg_banks << '\t'
           << a.rom.chr_banks << '\n';
    }
    return os.str();
}

// ---- the "ruth" diagram ---------------------------------------------------
// A single enclosure holding exactly the five discriminator cells. Each cell
// shows the condition name and a verdict glyph ([x] satisfied, [ ] not).
std::string to_ruth_diagram(const Analysis& a) {
    // Fixed-width cell: "[x] name" padded to exactly 20 columns.
    auto cell = [&](const std::string& name) -> std::string {
        std::string g = "[?]";
        for (const Condition& c : a.conditions)
            if (c.name == name) { g = c.satisfied ? "[x]" : "[ ]"; break; }
        std::string s = g + " " + name;
        while (s.size() < 20) s += ' ';
        return s;
    };
    auto center = [](std::string s, size_t width) -> std::string {
        if (s.size() >= width) return s.substr(0, width);
        size_t pad = width - s.size();
        return std::string(pad / 2, ' ') + s + std::string(pad - pad / 2, ' ');
    };

    const size_t W = 48; // inner width
    std::ostringstream os;
    os << "+" << std::string(W, '-') << "+\n";
    os << "|" << center("r u t h", W) << "|\n";
    os << "|" << center(a.source, W) << "|\n";
    os << "+" << std::string(W, '-') << "+\n";
    // Two 20-wide cells + 2 leading + 2 trailing = 44; pad 4 between -> 48.
    os << "| " << cell("play")     << "    " << cell("guarantee") << " |\n";
    os << "|" << std::string(W, ' ') << "|\n";
    os << "| " << cell("chapters") << "    " << cell("win")       << " |\n";
    os << "|" << std::string(W, ' ') << "|\n";
    os << "|" << center(cell("chemistry"), W) << "|\n";
    os << "+" << std::string(W, '-') << "+\n";
    os << "  legend: [x] satisfied   [ ] not satisfied\n";
    return os.str();
}

} // namespace nts
