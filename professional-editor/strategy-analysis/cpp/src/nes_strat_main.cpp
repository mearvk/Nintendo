// nes_strat_main.cpp -- NES strategy analysis CLI (C++17).
// Part of the mearvk/Nintendo Professional Editor.
//
// Usage:
//   nes-strat-cpp static   <rom.nes> <out.map.tsv>
//   nes-strat-cpp evidence <trace.tsv> <out.evidence.md> [rom.nes]
//   nes-strat-cpp full      <rom.nes> <trace.tsv> <out-prefix>
#include "NesStrategy.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace mearvk::nintendo::strategy;

static void usage() {
    std::cerr << "nes-strat-cpp -- NES ROM strategy analysis\n"
                 "usage:\n"
                 "  nes-strat-cpp static   <rom.nes> <out.map.tsv>\n"
                 "  nes-strat-cpp evidence <trace.tsv> <out.evidence.md> [rom.nes]\n"
                 "  nes-strat-cpp full      <rom.nes> <trace.tsv> <out-prefix>\n";
}

int main(int argc, char** argv) {
    std::vector<std::string> a(argv, argv + argc);
    if (a.size() < 2) { usage(); return 2; }
    try {
        if (a[1] == "static" && a.size() >= 4) {
            auto m = StaticMap::analyzeFile(a[2]);
            m.writeTsv(a[3]);
            std::size_t code = 0, jt = 0;
            for (const auto& s : m.spans()) {
                if (s.kind == SpanKind::Code) code++;
                else if (s.kind == SpanKind::JumpTable) jt++;
            }
            std::cout << std::hex << std::uppercase
                      << "static: reset=" << m.resetVec() << " nmi=" << m.nmiVec()
                      << " irq=" << m.irqVec() << std::dec
                      << "; " << code << " code blocks, " << jt << " jump tables -> " << a[3] << "\n";
            return 0;
        }
        if (a[1] == "evidence" && a.size() >= 4) {
            if (a.size() >= 5) {
                auto m = StaticMap::analyzeFile(a[4]);
                evidenceFromTrace(a[2], &m, a[3]);
            } else {
                evidenceFromTrace(a[2], nullptr, a[3]);
            }
            std::cout << "evidence: " << a[3] << "\n";
            return 0;
        }
        if (a[1] == "full" && a.size() >= 5) {
            std::string mapp = a[4] + ".map.tsv", evp = a[4] + ".evidence.md";
            auto m = StaticMap::analyzeFile(a[2]);
            m.writeTsv(mapp);
            evidenceFromTrace(a[3], &m, evp);
            std::cout << "full: " << mapp << " + " << evp << "\n";
            return 0;
        }
    } catch (const NesError& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    usage();
    return 2;
}
