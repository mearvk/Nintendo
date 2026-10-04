// NesStrategy.hpp -- NES ROM strategy analysis (C++17).
// Part of the mearvk/Nintendo Professional Editor. Analyzes a ROM you own to
// produce your own structural + behavioral evidence; ships no copyrighted
// content and copies no game code. See ../docs/METHOD.md for the honest limits.
#ifndef MEARVK_NINTENDO_NESSTRATEGY_HPP
#define MEARVK_NINTENDO_NESSTRATEGY_HPP

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace mearvk::nintendo::strategy {

class NesError : public std::runtime_error {
public:
    explicit NesError(const std::string& w) : std::runtime_error(w) {}
};

// ---- Part A: static structure ------------------------------------------

enum class SpanKind { Code, Data, JumpTable };

struct Span {
    SpanKind kind;
    std::uint16_t cpuAddr;
    std::size_t prgOff;
    std::size_t len;
    unsigned detail; // jump-table entry count, else 0
};

// Static analysis of a loaded iNES PRG-ROM: 6502 code-vs-data reachability from
// the hardware vectors + jump-table detection (with indirect-target seeding).
class StaticMap {
public:
    static StaticMap analyzeFile(const std::string& path);
    void writeTsv(const std::string& path) const;

    const std::vector<Span>& spans() const { return spans_; }
    std::uint16_t resetVec() const { return reset_; }
    std::uint16_t nmiVec() const { return nmi_; }
    std::uint16_t irqVec() const { return irq_; }
    std::size_t prgSize() const { return prgSize_; }
    std::uint16_t mapBase() const { return mapBase_; }

    // True if a CPU address falls inside a detected jump table.
    bool inJumpTable(std::uint16_t addr) const;

private:
    std::vector<Span> spans_;
    std::uint16_t reset_ = 0, nmi_ = 0, irq_ = 0, mapBase_ = 0x8000;
    std::size_t prgSize_ = 0;
};

// ---- Part B: dynamic evidence ------------------------------------------

// Ingest an execution trace (TSV) and write a Strategy Evidence report
// (Markdown), optionally cross-referencing a StaticMap (may be nullptr).
void evidenceFromTrace(const std::string& tracePath,
                       const StaticMap* staticMapOrNull,
                       const std::string& outMdPath);

} // namespace mearvk::nintendo::strategy

#endif // MEARVK_NINTENDO_NESSTRATEGY_HPP
