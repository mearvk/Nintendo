// NesRom.cpp -- iNES ROM container + text/menu editing (C++17 implementation).
// Part of the mearvk/Nintendo Professional Editor. No copyrighted ROM content.
#include "NesRom.hpp"

#include <cstring>
#include <fstream>
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

} // namespace mearvk::nintendo
