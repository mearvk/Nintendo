// NesStrategy.cpp -- static structure + dynamic evidence (C++17).
// Part of the mearvk/Nintendo Professional Editor. No copyrighted content.
#include "NesStrategy.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <vector>

namespace mearvk::nintendo::strategy {

namespace {
constexpr std::size_t kHeader = 16;
constexpr std::size_t kPrgBank = 16384;

// 6502 opcode -> length (1..3); 0 == undefined (decode stop).
constexpr std::array<std::uint8_t, 256> OPLEN = {
    1,2,0,0,0,2,2,0,1,2,1,0,0,3,3,0, 2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0,
    3,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0, 2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0,
    1,2,0,0,0,2,2,0,1,2,1,0,3,3,3,0, 2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0,
    1,2,0,0,0,2,2,0,1,2,1,0,3,3,3,0, 2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0,
    0,2,0,0,2,2,2,0,1,0,1,0,3,3,3,0, 2,2,0,0,2,2,2,0,1,3,1,0,0,3,0,0,
    2,2,2,0,2,2,2,0,1,2,1,0,3,3,3,0, 2,2,0,0,2,2,2,0,1,3,1,0,3,3,3,0,
    2,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0, 2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0,
    2,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0, 2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0
};

bool isJmpAbs(std::uint8_t op) { return op == 0x4C; }
bool isJsr(std::uint8_t op) { return op == 0x20; }
bool isRtsRti(std::uint8_t op) { return op == 0x60 || op == 0x40; }
bool isJmpInd(std::uint8_t op) { return op == 0x6C; }
bool isBranch(std::uint8_t op) {
    switch (op) { case 0x10: case 0x30: case 0x50: case 0x70:
                  case 0x90: case 0xB0: case 0xD0: case 0xF0: return true; }
    return false;
}

// Working context for the static pass.
struct Ctx {
    std::vector<std::uint8_t> rom;
    std::size_t prgOff = 0, prgSize = 0;
    std::uint16_t mapBase = 0x8000;
    std::vector<std::uint8_t> code; // per-PRG-byte visited mark

    long cpuToPrg(std::uint16_t addr) const {
        if (addr < mapBase) return -1;
        std::size_t off = static_cast<std::size_t>(addr - mapBase);
        if (prgSize == kPrgBank) off %= kPrgBank;
        if (off >= prgSize) return -1;
        return static_cast<long>(off);
    }
    std::uint16_t rd16(std::size_t prgRel) const {
        std::size_t fo = prgOff + prgRel;
        return static_cast<std::uint16_t>(rom[fo] | (rom[fo + 1] << 8));
    }
};

// Recursive-descent code marking from one entry (iterative worklist).
void traceFrom(Ctx& c, std::uint16_t entry) {
    std::vector<std::uint16_t> stack;
    stack.push_back(entry);
    while (!stack.empty()) {
        std::uint16_t pc = stack.back(); stack.pop_back();
        for (;;) {
            long off = c.cpuToPrg(pc);
            if (off < 0) break;
            if (c.code[off]) break;
            std::uint8_t op = c.rom[c.prgOff + off];
            std::uint8_t len = OPLEN[op];
            if (len == 0) { c.code[off] = 1; break; }
            for (std::uint8_t i = 0; i < len; i++) {
                long o = c.cpuToPrg(static_cast<std::uint16_t>(pc + i));
                if (o >= 0) c.code[o] = 1;
            }
            if (isJsr(op) || isJmpAbs(op)) {
                stack.push_back(c.rd16(static_cast<std::size_t>(off) + 1));
                if (isJmpAbs(op)) break;
            } else if (isBranch(op)) {
                auto rel = static_cast<std::int8_t>(c.rom[c.prgOff + off + 1]);
                stack.push_back(static_cast<std::uint16_t>(pc + 2 + rel));
            } else if (isRtsRti(op) || isJmpInd(op)) {
                break;
            }
            pc = static_cast<std::uint16_t>(pc + len);
        }
    }
}

// Jump-table detection. Records tables into `out` (if non-null) and seeds the
// code pass from each entry (if `seed`).
void detectJumpTables(Ctx& c, std::vector<Span>* out, bool seed) {
    constexpr unsigned kMinEntries = 4;
    std::size_t i = 0;
    while (i + 2 <= c.prgSize) {
        std::size_t start = i; unsigned entries = 0;
        while (i + 2 <= c.prgSize) {
            std::uint16_t ptr = c.rd16(i);
            if (c.cpuToPrg(ptr) < 0) break;
            entries++; i += 2;
        }
        if (entries >= kMinEntries) {
            if (out) {
                auto addr = static_cast<std::uint16_t>(
                    c.mapBase + (start % (c.prgSize == kPrgBank ? kPrgBank : c.prgSize)));
                out->push_back({SpanKind::JumpTable, addr, c.prgOff + start, entries * 2u, entries});
            }
            if (seed) for (unsigned e = 0; e < entries; e++) traceFrom(c, c.rd16(start + e * 2));
        } else {
            i = start + 1;
        }
    }
}

void emitSpans(const Ctx& c, std::vector<Span>& out) {
    std::size_t i = 0;
    while (i < c.prgSize) {
        std::uint8_t v = c.code[i];
        std::size_t run = 1;
        while (i + run < c.prgSize && c.code[i + run] == v) run++;
        auto addr = static_cast<std::uint16_t>(
            c.mapBase + (i % (c.prgSize == kPrgBank ? kPrgBank : c.prgSize)));
        out.push_back({v ? SpanKind::Code : SpanKind::Data, addr, c.prgOff + i, run, 0});
        i += run;
    }
}

std::string trim(const std::string& s) {
    std::size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    std::size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}
} // namespace

StaticMap StaticMap::analyzeFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) throw NesError("cannot open ROM: " + path);
    auto sz = static_cast<std::size_t>(in.tellg());
    if (sz < kHeader) throw NesError("file too small to be iNES: " + path);
    in.seekg(0);
    Ctx c;
    c.rom.resize(sz);
    in.read(reinterpret_cast<char*>(c.rom.data()), static_cast<std::streamsize>(sz));
    if (std::memcmp(c.rom.data(), "NES\x1A", 4) != 0)
        throw NesError("not a valid iNES image (bad magic): " + path);

    unsigned prgBanks = c.rom[4];
    bool trainer = (c.rom[6] & 0x04) != 0;
    c.prgOff = kHeader + (trainer ? 512u : 0u);
    c.prgSize = static_cast<std::size_t>(prgBanks) * kPrgBank;
    if (c.prgOff + c.prgSize > sz) throw NesError("iNES PRG region exceeds file size: " + path);
    c.mapBase = (c.prgSize == kPrgBank) ? 0xC000 : 0x8000;
    c.code.assign(c.prgSize, 0);

    StaticMap m;
    std::size_t vec = c.prgSize - 6;
    m.nmi_ = c.rd16(vec + 0);
    m.reset_ = c.rd16(vec + 2);
    m.irq_ = c.rd16(vec + 4);
    m.prgSize_ = c.prgSize;
    m.mapBase_ = c.mapBase;

    traceFrom(c, m.reset_);
    traceFrom(c, m.nmi_);
    traceFrom(c, m.irq_);

    // Seed from jump-table entries to a fixpoint (resolve indirect targets).
    for (;;) {
        std::size_t before = 0; for (auto b : c.code) before += b;
        detectJumpTables(c, nullptr, /*seed=*/true);
        std::size_t after = 0; for (auto b : c.code) after += b;
        if (after == before) break;
    }
    detectJumpTables(c, &m.spans_, /*seed=*/false);
    emitSpans(c, m.spans_);
    return m;
}

bool StaticMap::inJumpTable(std::uint16_t addr) const {
    for (const auto& s : spans_) {
        if (s.kind != SpanKind::JumpTable) continue;
        auto lo = s.cpuAddr;
        auto hi = static_cast<std::uint16_t>(lo + s.len);
        if (addr >= lo && addr < hi) return true;
    }
    return false;
}

void StaticMap::writeTsv(const std::string& path) const {
    std::ofstream o(path, std::ios::binary);
    if (!o) throw NesError("cannot write map: " + path);
    o << "# NES static map (code-vs-data + jump tables). Approximate; see METHOD.md.\n";
    o << std::hex << std::uppercase;
    o << "# vectors: reset=" << reset_ << " nmi=" << nmi_ << " irq=" << irq_
      << std::dec << "  prg=" << prgSize_ << std::hex << " map_base=" << mapBase_ << "\n";
    o << "kind\tbank\taddr\tend\tdetail\n";
    o << "vector\t-\tFFFC\tFFFD\treset=" << reset_ << "\n";
    for (const auto& s : spans_) {
        const char* k = s.kind == SpanKind::Code ? "code" :
                        s.kind == SpanKind::JumpTable ? "jumptable" : "data";
        auto end = static_cast<unsigned>(s.cpuAddr + (s.len ? s.len - 1 : 0));
        o << k << "\t0\t" << s.cpuAddr << "\t" << end << "\t";
        if (s.kind == SpanKind::JumpTable) o << std::dec << "entries=" << s.detail << std::hex;
        else o << (s.kind == SpanKind::Code ? "block" : "unreached");
        o << "\n";
    }
}

// ---- Part B -------------------------------------------------------------

namespace {
struct DecisionRec {
    unsigned execs = 0, branches = 0, taken = 0, reads = 0, writes = 0, jtbls = 0;
    std::set<std::uint16_t> codeAddrs;
    std::set<std::uint16_t> jtblAddrs;
};
} // namespace

void evidenceFromTrace(const std::string& tracePath, const StaticMap* smap,
                       const std::string& outMd) {
    std::ifstream f(tracePath);
    if (!f) throw NesError("cannot open trace: " + tracePath);
    std::map<std::string, DecisionRec> recs;
    std::vector<std::string> order; // preserve first-seen decision order
    unsigned long total = 0;
    std::string line;
    while (std::getline(f, line)) {
        std::string s = trim(line);
        if (s.empty() || s[0] == '#') continue;
        std::vector<std::string> cols;
        std::stringstream ss(s);
        std::string c;
        while (std::getline(ss, c, '\t')) cols.push_back(c);
        if (cols.size() < 2) continue;
        total++;
        std::string event = cols[1];
        std::string pcs = cols.size() > 2 ? cols[2] : "-";
        std::string arg = cols.size() > 3 ? cols[3] : "-";
        std::string dec = cols.size() > 4 ? cols[4] : "unknown";
        if (recs.find(dec) == recs.end()) order.push_back(dec);
        DecisionRec& r = recs[dec];
        std::uint16_t pc = 0;
        try { pc = static_cast<std::uint16_t>(std::stoul(pcs, nullptr, 16)); } catch (...) {}
        if (event == "exec") { r.execs++; r.codeAddrs.insert(pc); }
        else if (event == "branch") { r.branches++; if (arg.find("taken=1") != std::string::npos) r.taken++; }
        else if (event == "read") r.reads++;
        else if (event == "write") r.writes++;
        else if (event == "jtbl") { r.jtbls++; r.jtblAddrs.insert(pc); }
    }

    std::ofstream o(outMd, std::ios::binary);
    if (!o) throw NesError("cannot write report: " + outMd);
    o << "# Strategy Evidence Report\n\n";
    o << "Source trace: `" << tracePath << "`  \nTotal events: " << total
      << "  \nDecision contexts: " << order.size() << "\n\n";
    o << "> This report reflects only the code paths the supplied trace actually\n"
         "> exercised (and, where cross-referenced, a static approximation of code).\n"
         "> It is evidence for a human analysis, not a complete or guaranteed\n"
         "> extraction of the game's strategy. Absence of evidence is not evidence\n"
         "> of absence.\n\n";
    o << std::hex << std::uppercase;
    for (const auto& name : order) {
        const DecisionRec& r = recs[name];
        o << "## Decision: `" << name << "`\n\n";
        o << std::dec;
        o << "- exec events: " << r.execs << " (distinct code addresses: " << r.codeAddrs.size() << ")\n";
        o << "- branches: " << r.branches << " (taken: " << r.taken << ")\n";
        o << "- reads: " << r.reads << ", writes: " << r.writes << "\n";
        o << "- jump-table dispatches: " << r.jtbls << " (distinct tables: " << r.jtblAddrs.size() << ")\n";
        o << std::hex;
        if (!r.jtblAddrs.empty()) {
            o << "- dispatch addresses:";
            for (auto a : r.jtblAddrs) { o << " " << a; if (smap && smap->inJumpTable(a)) o << "(static-jt)"; }
            o << "\n";
        }
        if (!r.codeAddrs.empty()) o << "- first exercised address under this decision: `" << *r.codeAddrs.begin() << "`\n";
        o << "\n";
    }
    o << std::dec << "---\nMethod: static structure (Part A) + dynamic trace (Part B). See `docs/METHOD.md`.\n";
}

} // namespace mearvk::nintendo::strategy
