// NesRom.cpp -- iNES ROM container + text/menu editing (C++17 implementation).
// Part of the mearvk/Nintendo Professional Editor. No copyrighted ROM content.
#include "NesRom.hpp"

#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace mearvk::nintendo {

namespace {
constexpr std::size_t kHeader = 16;
constexpr std::size_t kPrgBank = 16384;
constexpr std::size_t kChrBank = 8192;
constexpr std::size_t kTrainer = 512;

int hexNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
} // namespace

// ---- CharTable ----------------------------------------------------------

void CharTable::load(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw NesError("cannot open table: " + path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.size() < 4 || line[0] == '#') continue;
        int hi = hexNibble(line[0]), lo = hexNibble(line[1]);
        if (hi < 0 || lo < 0 || line[2] != '=') continue;
        auto byte = static_cast<std::uint8_t>(hi * 16 + lo);
        char glyph = line[3];
        if (glyph == '\r' || glyph == '\n') glyph = ' ';
        byteToGlyph_[byte] = glyph;
        glyphToByte_.emplace(glyph, byte); // first mapping wins
        if (glyph == ' ') fill_ = byte;
    }
    loaded_ = true;
}

std::optional<std::uint8_t> CharTable::encodeGlyph(char glyph) const {
    if (!loaded_) return static_cast<std::uint8_t>(glyph); // ASCII identity
    auto it = glyphToByte_.find(glyph);
    if (it == glyphToByte_.end()) return std::nullopt;
    return it->second;
}

char CharTable::decodeByte(std::uint8_t b) const {
    if (!loaded_) return (b >= 0x20 && b < 0x7F) ? static_cast<char>(b) : '.';
    auto it = byteToGlyph_.find(b);
    return it == byteToGlyph_.end() ? '.' : it->second;
}

// ---- NesRom -------------------------------------------------------------

NesRom NesRom::load(const std::string& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) throw NesError("cannot open ROM: " + path);
    auto sz = static_cast<std::size_t>(in.tellg());
    if (sz < kHeader) throw NesError("file too small to be iNES: " + path);
    in.seekg(0);
    NesRom rom;
    rom.data_.resize(sz);
    if (!in.read(reinterpret_cast<char*>(rom.data_.data()), static_cast<std::streamsize>(sz)))
        throw NesError("read failed: " + path);
    if (std::memcmp(rom.data_.data(), "NES\x1A", 4) != 0)
        throw NesError("not a valid iNES image (bad magic): " + path);
    rom.resolveRegions();
    if (rom.prgOffset_ + rom.prgSize_ > rom.data_.size())
        throw NesError("iNES PRG region exceeds file size: " + path);
    return rom;
}

void NesRom::resolveRegions() {
    const auto& h = data_;
    prgBanks_ = h[4];
    chrBanks_ = h[5];
    hasTrainer_ = (h[6] & 0x04) != 0;
    mapper_ = static_cast<unsigned>((h[6] >> 4) | (h[7] & 0xF0));
    prgOffset_ = kHeader + (hasTrainer_ ? kTrainer : 0);
    prgSize_ = static_cast<std::size_t>(prgBanks_) * kPrgBank;
    chrSize_ = static_cast<std::size_t>(chrBanks_) * kChrBank;
    chrOffset_ = chrSize_ ? (prgOffset_ + prgSize_) : 0;
}

void NesRom::save(const std::string& path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw NesError("cannot write ROM: " + path);
    out.write(reinterpret_cast<const char*>(data_.data()),
              static_cast<std::streamsize>(data_.size()));
    if (!out) throw NesError("write failed: " + path);
}

void NesRom::checkSlot(std::size_t offset, std::size_t len) const {
    if (offset > data_.size() || len > data_.size() || offset + len > data_.size()) {
        std::ostringstream os;
        os << "slot out of range: offset=0x" << std::hex << offset
           << " len=" << std::dec << len << " size=" << data_.size();
        throw NesError(os.str());
    }
}

std::string NesRom::decode(std::size_t offset, std::size_t len, const CharTable& tbl) const {
    checkSlot(offset, len);
    std::string s;
    s.reserve(len);
    for (std::size_t i = 0; i < len; ++i) s.push_back(tbl.decodeByte(data_[offset + i]));
    return s;
}

void NesRom::setText(std::size_t offset, std::size_t len, const std::string& text,
                     const CharTable& tbl) {
    checkSlot(offset, len);
    if (text.size() > len)
        throw NesError("replacement (" + std::to_string(text.size()) +
                       ") longer than slot (" + std::to_string(len) + ")");
    std::uint8_t fill = tbl.loaded() ? tbl.fillByte() : 0x00;
    for (std::size_t i = 0; i < len; ++i) {
        if (i < text.size()) {
            auto b = tbl.encodeGlyph(text[i]);
            if (!b) throw NesError(std::string("glyph not in table: '") + text[i] + "'");
            data_[offset + i] = *b;
        } else {
            data_[offset + i] = fill;
        }
    }
}

bool NesRom::expectText(std::size_t offset, std::size_t len, const std::string& expected,
                        const CharTable& tbl) const {
    checkSlot(offset, len);
    if (expected.size() > len) return false;
    std::string got = decode(offset, len, tbl);
    if (got.compare(0, expected.size(), expected) != 0) return false;
    for (std::size_t i = expected.size(); i < len; ++i)
        if (got[i] != '.' && got[i] != ' ') return false;
    return true;
}

// ---- integrity audit (recurve) & provenance refresh (refresh) ----------

const char* NesRom::severityStr(Severity s) {
    switch (s) {
        case Severity::Ok:    return "ok";
        case Severity::Warn:  return "warn";
        case Severity::Error: return "error";
    }
    return "?";
}

std::uint64_t NesRom::fingerprint() const {
    std::uint64_t h = 0xcbf29ce484222325ULL;      // FNV-1a offset basis
    for (std::uint8_t b : data_) { h ^= b; h *= 1099511628211ULL; }
    return h;
}

namespace {
// Classify a region: 0 = mixed, 1 = all 0x00, 2 = all 0xFF.
int regionFill(const std::uint8_t* p, std::size_t n) {
    if (n == 0) return 0;
    std::uint8_t first = p[0];
    if (first != 0x00 && first != 0xFF) return 0;
    for (std::size_t i = 1; i < n; ++i) if (p[i] != first) return 0;
    return first == 0x00 ? 1 : 2;
}
std::string hex64(std::uint64_t v) {
    std::ostringstream os;
    os << std::uppercase << std::hex << std::setw(16) << std::setfill('0') << v;
    return os.str();
}
} // namespace

NesRom::Report NesRom::recurve() const {
    Report rep;
    auto add = [&](Severity sev, std::string check, std::string msg) {
        if (sev > rep.worst) rep.worst = sev;
        rep.findings.push_back({sev, std::move(check), std::move(msg)});
    };

    // 1. Header / magic.
    if (std::memcmp(data_.data(), "NES\x1A", 4) != 0)
        add(Severity::Error, "header", "iNES magic 'NES\\x1A' missing");
    else
        add(Severity::Ok, "header", "iNES magic present");

    // 2. Bank counts / mapper.
    if (prgBanks_ == 0)
        add(Severity::Error, "prg", "PRG bank count is 0 (no program)");
    else {
        std::ostringstream os;
        os << int(prgBanks_) << " PRG bank(s), " << prgSize_
           << " bytes; mapper " << mapper_;
        add(Severity::Ok, "prg", os.str());
    }

    // 3. Size consistency.
    std::size_t expect = kHeader + (hasTrainer_ ? kTrainer : 0) + prgSize_ + chrSize_;
    if (expect > data_.size()) {
        std::ostringstream os;
        os << "truncated: header declares " << expect
           << " bytes but file is " << data_.size();
        add(Severity::Error, "size", os.str());
    } else if (expect < data_.size()) {
        std::ostringstream os;
        os << (data_.size() - expect) << " trailing byte(s) after declared regions";
        add(Severity::Warn, "size", os.str());
    } else {
        add(Severity::Ok, "size", "file length matches header");
    }

    // 4. Embedded images: audit each CHR (tile) bank.
    if (chrBanks_ == 0) {
        add(Severity::Warn, "chr",
            "no CHR-ROM banks (CHR-RAM title or program-only image)");
    } else {
        for (unsigned b = 0; b < chrBanks_; ++b) {
            std::size_t off = chrOffset_ + static_cast<std::size_t>(b) * kChrBank;
            std::string name = "chr#" + std::to_string(b);
            if (off + kChrBank > data_.size()) {
                add(Severity::Error, name,
                    "CHR bank " + std::to_string(b) + " extends past end of file (damaged)");
                continue;
            }
            const std::uint8_t* p = data_.data() + off;
            int fill = regionFill(p, kChrBank);
            std::uint64_t h = 0xcbf29ce484222325ULL;
            for (std::size_t i = 0; i < kChrBank; ++i) { h ^= p[i]; h *= 1099511628211ULL; }
            if (fill == 1)
                add(Severity::Warn, name,
                    "CHR bank " + std::to_string(b) + " is entirely 0x00 (blank/no tiles)");
            else if (fill == 2)
                add(Severity::Warn, name,
                    "CHR bank " + std::to_string(b) + " is entirely 0xFF (erased/undamaged check)");
            else
                add(Severity::Ok, name,
                    "CHR bank " + std::to_string(b) + " intact, tiles present (fp " + hex64(h) + ")");
        }
    }

    // 5. PRG tail observation.
    if (prgSize_ >= 16) {
        const std::uint8_t* tail = data_.data() + prgOffset_ + prgSize_ - 16;
        if (regionFill(tail, 16) == 2)
            add(Severity::Ok, "prg-tail", "PRG tail is 0xFF fill (typical of unused space)");
    }

    return rep;
}

std::uint64_t NesRom::refresh(const std::string& outPath) const {
    // Re-emit a clean, canonical copy of the owned ROM (byte-identical).
    save(outPath);

    std::uint64_t fp = fingerprint();

    // ISO-8601 UTC timestamp.
    std::time_t t = std::time(nullptr);
    std::tm g{};
#if defined(_WIN32)
    gmtime_s(&g, &t);
#else
    gmtime_r(&t, &g);
#endif
    char ts[32];
    std::strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", &g);

    // Provenance sidecar: "<outPath>.provenance". No ROM bytes.
    std::ofstream out(outPath + ".provenance", std::ios::binary);
    if (!out) throw NesError("cannot write provenance: " + outPath + ".provenance");
    out << "# mearvk/Nintendo Professional Editor -- refresh provenance record\n"
        << "# This records WHEN a ROM you own was last re-emitted and its content\n"
        << "# fingerprint. It contains no copyrighted ROM data.\n"
        << "tool        = nes-edit-cpp (C++)\n"
        << "action      = refresh\n"
        << "refreshed   = " << ts << "\n"
        << "image       = " << outPath << "\n"
        << "size        = " << data_.size() << "\n"
        << "prg_banks   = " << int(prgBanks_) << "\n"
        << "chr_banks   = " << int(chrBanks_) << "\n"
        << "mapper      = " << mapper_ << "\n"
        << "fingerprint = fnv1a64:" << hex64(fp) << "\n";
    if (!out) throw NesError("write failed: " + outPath + ".provenance");
    return fp;
}

} // namespace mearvk::nintendo
