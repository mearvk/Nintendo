// NesRom.hpp -- iNES ROM container + text/menu editing (C++17).
// Part of the mearvk/Nintendo Professional Editor. No copyrighted ROM content.
//
// A modern RAII wrapper over the same editing model as the C library: load an
// iNES image the user owns, decode/edit fixed-width text and menu slots in
// PRG-ROM, and write it back. Replacements are length-bounded so pointers and
// bank boundaries are never disturbed.
#ifndef MEARVK_NINTENDO_NESROM_HPP
#define MEARVK_NINTENDO_NESROM_HPP

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace mearvk::nintendo {

// Thrown for format, range, encoding, and length-rule violations.
class NesError : public std::runtime_error {
public:
    explicit NesError(const std::string& what) : std::runtime_error(what) {}
};

// A character map (.tbl). Unloaded == ASCII identity.
class CharTable {
public:
    CharTable() = default;
    // Load a .tbl ("HH=glyph" lines). Throws NesError on I/O failure.
    void load(const std::string& path);
    bool loaded() const { return loaded_; }

    // Encode one glyph to its ROM byte; nullopt if the glyph is unmapped.
    std::optional<std::uint8_t> encodeGlyph(char glyph) const;
    // Decode one ROM byte to a glyph ('.' if unmapped / non-printable).
    char decodeByte(std::uint8_t b) const;
    std::uint8_t fillByte() const { return fill_; }

private:
    bool loaded_ = false;
    std::unordered_map<char, std::uint8_t> glyphToByte_;
    std::unordered_map<std::uint8_t, char> byteToGlyph_;
    std::uint8_t fill_ = 0x00;
};

class NesRom {
public:
    // Load an iNES image from disk. Throws NesError if it is not valid iNES.
    static NesRom load(const std::string& path);

    // Write the (possibly edited) image back to disk.
    void save(const std::string& path) const;

    // --- header / regions ---
    std::uint8_t prgBanks() const { return prgBanks_; }
    std::uint8_t chrBanks() const { return chrBanks_; }
    unsigned mapper() const { return mapper_; }
    bool hasTrainer() const { return hasTrainer_; }
    std::size_t size() const { return data_.size(); }
    std::size_t prgOffset() const { return prgOffset_; }
    std::size_t prgSize() const { return prgSize_; }
    std::size_t chrOffset() const { return chrOffset_; }
    std::size_t chrSize() const { return chrSize_; }

    // --- text / menu editing ---

    // Decode the `len`-byte slot at absolute file `offset` with `tbl`.
    std::string decode(std::size_t offset, std::size_t len,
                       const CharTable& tbl = CharTable{}) const;

    // Replace the slot at `offset` with `text` (encoded <= len, padded).
    // Throws NesError on range / over-length / unencodable glyph.
    void setText(std::size_t offset, std::size_t len, const std::string& text,
                 const CharTable& tbl = CharTable{});

    // Guard: true iff the slot currently decodes to `expected` (ignoring
    // trailing fill). Does not modify the ROM.
    bool expectText(std::size_t offset, std::size_t len, const std::string& expected,
                    const CharTable& tbl = CharTable{}) const;

private:
    void resolveRegions();
    void checkSlot(std::size_t offset, std::size_t len) const;

    std::vector<std::uint8_t> data_;
    std::uint8_t prgBanks_ = 0, chrBanks_ = 0;
    bool hasTrainer_ = false;
    unsigned mapper_ = 0;
    std::size_t prgOffset_ = 0, prgSize_ = 0, chrOffset_ = 0, chrSize_ = 0;
};

} // namespace mearvk::nintendo

#endif // MEARVK_NINTENDO_NESROM_HPP
