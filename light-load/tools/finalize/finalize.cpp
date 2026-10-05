// ============================================================================
// finalize — the release gate for a built .nes.
// ----------------------------------------------------------------------------
// Our own code. Runs the full validation chain on an image we built and, only
// if every gate passes, writes a release manifest (RELEASE.txt) recording the
// image name, size, structural verdict, and a stable fingerprint. Shells out
// only to OUR OWN tools in this repo.
//
// Gates (all must pass):
//   1. structure — nts-analyze returns success (play/chemistry/guarantee).
//   2. split     — rom_split can decompose it (header + PRG present, vectors).
//   3. size      — within a declared max (default 1 MiB; override with --max).
//
// Usage:
//   finalize <image.nes> [--nts <nts-analyze>] [--split <rom_split>]
//            [--max BYTES] [--out <manifest>]
// ============================================================================
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

namespace {

int run(const std::string& cmd) {
    std::cout << "  $ " << cmd << "\n";
    int rc = std::system(cmd.c_str());
    return (rc == 0) ? 0 : (rc >> 8 ? (rc >> 8) : 1);
}

// Stable additive fingerprint (same FNV-style accumulator used elsewhere).
uint32_t fingerprint(const std::string& path, long& sizeOut) {
    std::ifstream f(path, std::ios::binary);
    uint32_t h = 2166136261u;
    sizeOut = 0;
    char c;
    while (f.get(c)) { h = (h ^ (uint8_t)c) * 16777619u; ++sizeOut; }
    return h;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: finalize <image.nes> [--nts <p>] [--split <p>] "
                     "[--max BYTES] [--out <manifest>]\n";
        return 2;
    }
    std::string image = argv[1];
    std::string nts = "../nts/nts-analyze";
    std::string split = "../tools/rom-splitter/rom_split";
    long maxBytes = 1024 * 1024;
    std::string out = "RELEASE.txt";

    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--nts" && i+1 < argc) nts = argv[++i];
        else if (a == "--split" && i+1 < argc) split = argv[++i];
        else if (a == "--max" && i+1 < argc) maxBytes = std::atol(argv[++i]);
        else if (a == "--out" && i+1 < argc) out = argv[++i];
    }

    if (!fs::exists(image)) { std::cerr << "no such image: " << image << "\n"; return 1; }

    std::cout << "== finalize: " << image << " ==\n";
    bool ok = true;

    // Gate 1: structural verdict
    std::cout << "-- gate 1: structure --\n";
    if (run(nts + " " + image + " --ruth") != 0) {
        std::cout << "  FAIL structure\n"; ok = false;
    } else std::cout << "  pass structure\n";

    // Gate 2: decomposable (vectors present)
    std::cout << "-- gate 2: split --\n";
    {
        std::string prefix = image + ".fin";
        if (run(split + " " + image + " " + prefix) != 0) {
            std::cout << "  FAIL split\n"; ok = false;
        } else std::cout << "  pass split\n";
        // clean the probe outputs
        for (const char* ext : {".hdr", ".prg", ".chr"}) {
            std::error_code ec; fs::remove(prefix + ext, ec);
        }
    }

    // Gate 3: size budget
    std::cout << "-- gate 3: size --\n";
    long sz = 0;
    uint32_t fp = fingerprint(image, sz);
    if (sz > maxBytes) {
        std::cout << "  FAIL size " << sz << " B > max " << maxBytes << " B\n";
        ok = false;
    } else {
        std::cout << "  pass size " << sz << " B (<= " << maxBytes << " B)\n";
    }

    if (!ok) {
        std::cout << "== NOT finalized: one or more gates failed ==\n";
        return 3;
    }

    // Write the release manifest.
    std::ostringstream m;
    m << "NTS release manifest\n";
    m << "image=" << image << "\n";
    m << "size_bytes=" << sz << "\n";
    char hex[16]; std::snprintf(hex, sizeof(hex), "0x%08X", fp);
    m << "fingerprint=" << hex << "\n";
    m << "gates=structure,split,size\n";
    m << "status=FINALIZED\n";

    std::ofstream of(out);
    of << m.str();
    of.close();

    std::cout << "-- manifest --\n" << m.str();
    std::cout << "== finalized: wrote " << out << " ==\n";
    return 0;
}
