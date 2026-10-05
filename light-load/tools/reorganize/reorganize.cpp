// ============================================================================
// reorganize — verify/normalize the project layout.
// ----------------------------------------------------------------------------
// Our own code. Walks a project root and checks that the expected structure is
// present, reports stray build artifacts that should not be committed, and
// (optionally) prints the canonical layout. It does not move files unless
// --apply is given; by default it only reports, so it is safe to run.
//
// Usage:
//   reorganize <root> [--apply]
//     (default: report only; --apply would create missing standard dirs)
// ============================================================================
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

// The canonical directories we expect in a light-load NTS workspace.
const std::vector<std::string> kExpectedDirs = {
    "nts", "nts/c-ines-reader", "nts/cpp-discriminator", "nts/c-canvas-gen",
    "nts/java-rom-tool",
    "tools", "tools/asm6502", "tools/chr-encoder", "tools/audio-builder",
    "tools/exec-harness", "tools/rom-linker", "tools/nametable-builder",
    "tools/palette-tool", "tools/disasm", "tools/rom-splitter",
    "runtime", "runtime/reset", "runtime/ppu-driver", "runtime/input",
    "runtime/audio-engine",
    "build", "docs",
};

// Extensions that are build artifacts and should not live in the repo.
bool is_artifact(const fs::path& p) {
    static const std::vector<std::string> ext = {
        ".o", ".obj", ".class", ".bin", ".prg", ".chr", ".snd", ".nam", ".pal"
    };
    std::string e = p.extension().string();
    for (const auto& x : ext) if (e == x) return true;
    // compiled binaries have no extension; detect a few known names
    std::string name = p.filename().string();
    static const std::vector<std::string> bins = {
        "asm6502","chr_encode","audio_build","exec6502","rom_link",
        "nt_build","pal_build","disasm6502","rom_split","build_driver",
        "reorganize","finalize","nts-analyze","nts-canvas"
    };
    for (const auto& x : bins) if (name == x) return true;
    return false;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: reorganize <root> [--apply]\n";
        return 2;
    }
    fs::path root = argv[1];
    bool apply = (argc >= 3 && std::string(argv[2]) == "--apply");

    if (!fs::exists(root) || !fs::is_directory(root)) {
        std::cerr << "not a directory: " << root << "\n";
        return 1;
    }

    int missing = 0, created = 0, artifacts = 0;

    std::cout << "== reorganize: " << root << (apply ? " (apply)" : " (report)")
              << " ==\n";

    // 1. check expected directories
    std::cout << "-- expected directories --\n";
    for (const auto& d : kExpectedDirs) {
        fs::path full = root / d;
        if (fs::exists(full) && fs::is_directory(full)) {
            std::cout << "  ok    " << d << "\n";
        } else {
            ++missing;
            if (apply) {
                std::error_code ec;
                if (fs::create_directories(full, ec)) { ++created;
                    std::cout << "  made  " << d << "\n"; }
                else std::cout << "  FAIL  " << d << " (" << ec.message() << ")\n";
            } else {
                std::cout << "  miss  " << d << "\n";
            }
        }
    }

    // 2. scan for stray build artifacts
    std::cout << "-- stray build artifacts (should be git-ignored) --\n";
    for (auto it = fs::recursive_directory_iterator(root);
         it != fs::recursive_directory_iterator(); ++it) {
        const fs::path& p = it->path();
        // skip VCS and tool output dirs
        std::string s = p.string();
        if (s.find("/.git/") != std::string::npos) { it.disable_recursion_pending(); continue; }
        if (it->is_regular_file() && is_artifact(p)) {
            ++artifacts;
            std::cout << "  artifact  " << fs::relative(p, root).string() << "\n";
        }
    }
    if (artifacts == 0) std::cout << "  (none found)\n";

    // 3. summary
    std::cout << "-- summary --\n";
    std::cout << "  missing dirs : " << missing
              << (apply ? (" (created " + std::to_string(created) + ")") : "")
              << "\n";
    std::cout << "  artifacts    : " << artifacts << "\n";

    // non-zero exit if report mode found problems (useful in CI/finalize)
    if (!apply && (missing > 0 || artifacts > 0)) return 3;
    return 0;
}
