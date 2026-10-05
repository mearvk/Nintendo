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
    // --declare: report the DECLARED capacity from the header even when the
    // body is absent (header-only capacity file). Truncation is expected here.
    if (mode == "--declare") {
        nts_rom rom;
        nts_status st = nts_read_file(path.c_str(), &rom);
        if (st != NTS_OK && st != NTS_ERR_SIZE_MISMATCH) {
            std::cerr << "error: " << nts_status_str(st) << " (" << path << ")\n";
            return 1;
        }
        double prg_mb = (double)rom.prg_bytes / (1024.0 * 1024.0);
        double chr_mb = (double)rom.chr_bytes / (1024.0 * 1024.0);
        std::cout << "declared capacity for " << path << ":\n";
        std::cout << "  format : "
                  << (rom.format == NTS_FMT_NES20 ? "NES 2.0" : "iNES") << "\n";
        std::cout << "  mapper : " << rom.mapper << "\n";
        std::cout << "  PRG    : " << prg_mb << " MB"
                  << (rom.exponent_prg ? " (exponent notation)" : "") << "\n";
        std::cout << "  CHR    : " << chr_mb << " MB"
                  << (rom.exponent_chr ? " (exponent notation)" : "") << "\n";
        std::cout << "  body   : "
                  << (rom.truncated ? "absent (header-only declaration)"
                                    : "present")
                  << "\n";
        return 0;
    }

    if (!a.valid) {
        std::cerr << "error: " << a.error << " (" << path << ")\n";
        return 1;
    }
    if (mode == "--tsv")       std::cout << nts::to_tsv(a);
    else if (mode == "--ruth") std::cout << nts::to_ruth_diagram(a);
    else                       std::cout << nts::to_markdown(a);
    return 0;
}
