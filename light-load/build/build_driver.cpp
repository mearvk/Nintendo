// ============================================================================
// build_driver — orchestrates our own tools into a complete, bootable .nes.
// ----------------------------------------------------------------------------
// Our own code. Reads a tiny project manifest and runs the pipeline:
//   asm6502 (code) -> chr_encode (tiles) -> audio_build (score)
//   -> rom_link (bootable image) -> nts-analyze (structural validation).
//
// It shells out only to OUR OWN tools in this repo (no third-party programs).
// This is the one-command build that ties docs/EDITIONS.md together.
//
// Manifest (simple key = value lines; '#' comments):
//   out      = game.nes
//   prg_src  = ../runtime/reset/reset.s        (one or more, repeatable)
//   chr_src  = ../tools/examples/tile.txt      (optional)
//   score    = ../tools/examples/score.txt     (optional)
//   mapper   = 0
//   org      = 8000
//
// Usage:  build_driver <manifest> [--tools <dir>] [--nts <nts-analyze path>]
// ============================================================================
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Manifest {
    std::string out = "game.nes";
    std::vector<std::string> prgSrc;
    std::string chrSrc;
    std::string score;
    int mapper = 0;
    std::string org;      // hex, optional
    bool battery = false;
};

std::string trim(std::string s) {
    auto issp = [](unsigned char c){ return std::isspace(c); };
    while (!s.empty() && issp(s.front())) s.erase(s.begin());
    while (!s.empty() && issp(s.back())) s.pop_back();
    return s;
}

int run(const std::string& cmd) {
    std::cout << "  $ " << cmd << "\n";
    int rc = std::system(cmd.c_str());
    return rc == 0 ? 0 : (rc >> 8 ? (rc >> 8) : 1);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: build_driver <manifest> [--tools <dir>] "
                     "[--nts <path>]\n";
        return 2;
    }
    std::string manifestPath = argv[1];
    std::string tools = "../tools";
    std::string nts = "../nts/nts-analyze";
    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--tools" && i + 1 < argc) tools = argv[++i];
        else if (a == "--nts" && i + 1 < argc) nts = argv[++i];
    }

    std::ifstream in(manifestPath);
    if (!in) { std::cerr << "cannot open manifest: " << manifestPath << "\n"; return 1; }

    Manifest m;
    std::string line;
    while (std::getline(in, line)) {
        auto h = line.find('#'); if (h != std::string::npos) line = line.substr(0, h);
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));
        if (key.empty() || val.empty()) continue;
        if (key == "out") m.out = val;
        else if (key == "prg_src") m.prgSrc.push_back(val);
        else if (key == "chr_src") m.chrSrc = val;
        else if (key == "score") m.score = val;
        else if (key == "mapper") m.mapper = std::stoi(val);
        else if (key == "org") m.org = val;
        else if (key == "battery") m.battery = (val == "1" || val == "true");
    }
    if (m.prgSrc.empty()) { std::cerr << "manifest: at least one prg_src required\n"; return 1; }

    std::cout << "== NTS build driver ==\n";
    std::cout << "output: " << m.out << "\n";

    // Stage 1: assemble each PRG source; concatenate the resulting blobs.
    std::string prgBlob = m.out + ".prg";
    { std::ofstream clr(prgBlob, std::ios::binary | std::ios::trunc); }
    for (size_t i = 0; i < m.prgSrc.size(); ++i) {
        std::string part = m.out + ".part" + std::to_string(i) + ".bin";
        if (run(tools + "/asm6502/asm6502 " + m.prgSrc[i] + " " + part)) {
            std::cerr << "assemble failed: " << m.prgSrc[i] << "\n"; return 1;
        }
        // append part -> blob
        std::ifstream src(part, std::ios::binary);
        std::ofstream dst(prgBlob, std::ios::binary | std::ios::app);
        dst << src.rdbuf();
    }

    // Stage 2: CHR (optional)
    std::string chrBlob;
    if (!m.chrSrc.empty()) {
        chrBlob = m.out + ".chr";
        if (run(tools + "/chr-encoder/chr_encode " + m.chrSrc + " " + chrBlob)) {
            std::cerr << "chr encode failed\n"; return 1;
        }
    }

    // Stage 3: audio (optional) — produced as a data file for inclusion.
    if (!m.score.empty()) {
        std::string snd = m.out + ".snd";
        if (run(tools + "/audio-builder/audio_build " + m.score + " " + snd)) {
            std::cerr << "audio build failed\n"; return 1;
        }
    }

    // Stage 4: link a bootable image with correct vectors.
    std::ostringstream link;
    link << tools << "/rom-linker/rom_link " << m.out
         << " --prg " << prgBlob
         << " --mapper " << m.mapper;
    if (m.battery) link << " --battery";
    if (!chrBlob.empty()) link << " --chr " << chrBlob;
    if (!m.org.empty())   link << " --org " << m.org;
    if (run(link.str())) { std::cerr << "link failed\n"; return 1; }

    // Stage 5: structural validation with our own analyzer.
    std::cout << "== validate ==\n";
    if (run(nts + " " + m.out + " --ruth")) {
        std::cerr << "WARNING: structural validation reported an error\n";
    }

    std::cout << "== done: " << m.out << " ==\n";
    return 0;
}
