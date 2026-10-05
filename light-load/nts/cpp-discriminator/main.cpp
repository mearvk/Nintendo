// ============================================================================
// NTS CLI — reads iNES structure and emits merit/strategy/components analysis.
//
//   nts-analyze <file.nes> [--tsv]
//
// Reports structural metadata and the five discriminator conditions only.
// Does not read, extract, or emit any game content.
// ============================================================================
#include "discriminator.hpp"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: nts-analyze <file.nes> [--tsv | --ruth]\n";
        return 2;
    }
    std::string path = argv[1];
    std::string mode = (argc >= 3) ? argv[2] : "";

    nts::Analysis a = nts::analyze_file(path);
    if (!a.valid) {
        std::cerr << "error: " << a.error << " (" << path << ")\n";
        return 1;
    }
    if (mode == "--tsv")       std::cout << nts::to_tsv(a);
    else if (mode == "--ruth") std::cout << nts::to_ruth_diagram(a);
    else                       std::cout << nts::to_markdown(a);
    return 0;
}
