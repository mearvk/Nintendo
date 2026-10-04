// nes_edit_main.cpp -- command-line NES ROM text/menu editor (C++17).
// Part of the mearvk/Nintendo Professional Editor.
//
// Usage:
//   nes-edit-cpp info    <rom.nes>
//   nes-edit-cpp dump    <rom.nes> <offset-hex> <len> [table.tbl]
//   nes-edit-cpp apply   <in.nes> <script.edit> <out.nes>
//   nes-edit-cpp recurve <rom.nes>
//   nes-edit-cpp refresh <in.nes> <out.nes>
//
// Shares the edit-script grammar with the C / SLeeLa / Java implementations:
//   table <path.tbl>
//   find  <offset-hex> <len> <text>
//   set   <offset-hex> <len> <text>
//   menu  <name> <offset-hex> <len> <text>
#include "NesRom.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace mearvk::nintendo;

namespace {

std::size_t parseHex(const std::string& s) { return std::stoul(s, nullptr, 16); }
std::size_t parseDec(const std::string& s) { return std::stoul(s, nullptr, 10); }

// Split into up to `maxTokens`; the token at index `textFrom` captures the rest
// of the line (minus optional surrounding quotes), preserving inner spaces.
std::vector<std::string> tokenize(const std::string& line, std::size_t maxTokens,
                                  std::size_t textFrom) {
    std::vector<std::string> out;
    std::size_t i = 0, n = line.size();
    while (i < n && out.size() < maxTokens) {
        while (i < n && (line[i] == ' ' || line[i] == '\t')) ++i;
        if (i >= n) break;
        if (out.size() == textFrom) {
            std::string rest = line.substr(i);
            if (!rest.empty() && rest.front() == '"') {
                rest.erase(rest.begin());
                auto q = rest.find('"');
                if (q != std::string::npos) rest = rest.substr(0, q);
            }
            out.push_back(rest);
            break;
        }
        std::size_t start = i;
        while (i < n && line[i] != ' ' && line[i] != '\t') ++i;
        out.push_back(line.substr(start, i - start));
    }
    return out;
}

int cmdInfo(const std::string& path) {
    auto rom = NesRom::load(path);
    std::cout << "iNES image: " << path << "\n"
              << "  size      : " << rom.size() << " bytes\n"
              << "  PRG banks : " << int(rom.prgBanks()) << " (" << rom.prgSize()
              << " bytes @ 0x" << std::hex << rom.prgOffset() << std::dec << ")\n"
              << "  CHR banks : " << int(rom.chrBanks()) << " (" << rom.chrSize()
              << " bytes @ 0x" << std::hex << rom.chrOffset() << std::dec << ")\n"
              << "  mapper    : " << rom.mapper() << "\n"
              << "  trainer   : " << (rom.hasTrainer() ? "yes" : "no") << "\n";
    return 0;
}

int cmdDump(const std::vector<std::string>& a) {
    if (a.size() < 5) { std::cerr << "dump: need <rom> <offset-hex> <len> [table]\n"; return 2; }
    auto rom = NesRom::load(a[2]);
    CharTable tbl;
    if (a.size() >= 6) tbl.load(a[5]);
    auto off = parseHex(a[3]);
    auto len = parseDec(a[4]);
    std::cout << "0x" << std::hex << off << std::dec << " [" << len << "] = \""
              << rom.decode(off, len, tbl) << "\"\n";
    return 0;
}

int cmdApply(const std::string& in, const std::string& script, const std::string& out) {
    auto rom = NesRom::load(in);
    std::ifstream f(script);
    if (!f) { std::cerr << "apply: cannot open script " << script << "\n"; return 1; }
    CharTable tbl;
    std::string line;
    int lineno = 0, edits = 0;
    while (std::getline(f, line)) {
        ++lineno;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::size_t h = line.find_first_not_of(" \t");
        if (h == std::string::npos || line[h] == '#') continue;
        std::string body = line.substr(h);
        std::istringstream head(body);
        std::string kw; head >> kw;
        try {
            if (kw == "table") {
                auto t = tokenize(body, 2, 1);
                tbl.load(t.at(1));
            } else if (kw == "find") {
                auto t = tokenize(body, 4, 3);
                if (t.size() < 4) throw NesError("find needs offset len text");
                if (!rom.expectText(parseHex(t[1]), parseDec(t[2]), t[3], tbl))
                    throw NesError("find guard did not match");
            } else if (kw == "set") {
                auto t = tokenize(body, 4, 3);
                if (t.size() < 4) throw NesError("set needs offset len text");
                rom.setText(parseHex(t[1]), parseDec(t[2]), t[3], tbl);
                ++edits;
            } else if (kw == "menu") {
                auto t = tokenize(body, 5, 4);
                if (t.size() < 5) throw NesError("menu needs name offset len text");
                rom.setText(parseHex(t[2]), parseDec(t[3]), t[4], tbl);
                std::cout << "menu '" << t[1] << "' set @ " << t[2] << "\n";
                ++edits;
            } else {
                throw NesError("unknown directive '" + kw + "'");
            }
        } catch (const NesError& e) {
            std::cerr << "apply:" << lineno << ": " << e.what() << "\n"
                      << "apply: aborted; " << out << " not written\n";
            return 1;
        }
    }
    rom.save(out);
    std::cout << "applied " << edits << " edit(s) -> " << out << "\n";
    return 0;
}

int cmdRecurve(const std::string& path) {
    auto rom = NesRom::load(path);
    auto rep = rom.recurve();
    std::cout << "recurve: integrity audit of " << path << "\n";
    for (const auto& f : rep.findings) {
        std::ostringstream sev;
        sev << std::left << std::setw(5) << NesRom::severityStr(f.severity);
        std::cout << "  [" << sev.str() << "] " << std::left << std::setw(8)
                  << f.check << " " << f.message << "\n";
    }
    std::ostringstream fp;
    fp << std::uppercase << std::hex << std::setw(16) << std::setfill('0') << rom.fingerprint();
    std::cout << "  fingerprint fnv1a64:" << fp.str() << "\n";
    std::cout << "verdict: "
              << (rep.worst == NesRom::Severity::Ok   ? "HEALTHY"
                : rep.worst == NesRom::Severity::Warn ? "HEALTHY (with warnings)"
                                                      : "DAMAGED")
              << "\n";
    return rep.worst == NesRom::Severity::Error ? 1 : 0;
}

int cmdRefresh(const std::string& in, const std::string& out) {
    auto rom = NesRom::load(in);
    std::uint64_t fp = rom.refresh(out);
    std::ostringstream fph;
    fph << std::uppercase << std::hex << std::setw(16) << std::setfill('0') << fp;
    std::cout << "refreshed " << in << " -> " << out << " (" << rom.size() << " bytes)\n"
              << "  fingerprint fnv1a64:" << fph.str() << "\n"
              << "  provenance  " << out << ".provenance\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    std::vector<std::string> a(argv, argv + argc);
    if (a.size() < 2) {
        std::cerr << "nes-edit-cpp -- NES ROM text/menu editor\n"
                     "usage:\n"
                     "  nes-edit-cpp info    <rom.nes>\n"
                     "  nes-edit-cpp dump    <rom.nes> <offset-hex> <len> [table.tbl]\n"
                     "  nes-edit-cpp apply   <in.nes> <script.edit> <out.nes>\n"
                     "  nes-edit-cpp recurve <rom.nes>\n"
                     "  nes-edit-cpp refresh <in.nes> <out.nes>\n";
        return 2;
    }
    try {
        if (a[1] == "info" && a.size() >= 3) return cmdInfo(a[2]);
        if (a[1] == "dump") return cmdDump(a);
        if (a[1] == "apply" && a.size() >= 5) return cmdApply(a[2], a[3], a[4]);
        if (a[1] == "recurve" && a.size() >= 3) return cmdRecurve(a[2]);
        if (a[1] == "refresh" && a.size() >= 4) return cmdRefresh(a[2], a[3]);
    } catch (const NesError& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    std::cerr << "nes-edit-cpp: unknown or incomplete command\n";
    return 2;
}
