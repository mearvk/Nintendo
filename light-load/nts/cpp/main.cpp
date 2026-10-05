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
        std::cerr << "usage: nts-analyze <file.nes> [--tsv]\n";
        return 2;
    }
    std::string path = argv[1];
    bool tsv = (argc >= 3 && std::string(argv[2]) == "--tsv");

    nts::Analysis a = nts::analyze_file(path);
    if (!a.valid) {
        std::cerr << "error: " << a.error << " (" << path << ")\n";
        return 1;
    }
    std::cout << (tsv ? nts::to_tsv(a) : nts::to_markdown(a));
    return 0;
}
