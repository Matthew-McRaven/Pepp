#pragma once
#include "./interp.hpp"

// Dict header exactly as it exists in application memory.
// You should be able to cast (er.. start_lifetime_as or memcpy) a known NT to this data type
// Fixed-sized fields, preceded by variable-length header fields and followed by code.
struct RawDictHeader {
  // Offsets from canonical name_token pointer, which is currently "link".
  enum class StaticOffsets : u16 {
    LINK = 0x00,
    STRLEN = 0x02,
    FLAGS = 0x02,
    CODE = 0x04,
    DATA = 0x06,
  };
  u16 link;
  u8 strlen_flags;
  u8 pad;
  u16 pcode;
};
static_assert(sizeof(RawDictHeader) == 6, "RawDictHeader must be 8 bytes");
static_assert(std::is_trivially_copyable_v<RawDictHeader>, "RawDictHeader must be trivially copyable");

// Wrapper around the RawDictHeader to give you some more useful C++-ish information, like the relative address of a
// field. Does not match the memory layout of the actual dict header!!
class NiceDictHeader {
public:
  NiceDictHeader(Interpreter *interp, u16 nt_addr);
  std::string_view name() const;
  // Address of the pcode field
  u16 pcode_addr() const;
  // Value of the pcode field.
  u16 pcode() const;
  // Return the value at mem[pcode].
  u16 code0() const;

  u16 link() const;
  u16 link_addr() const;
  u8 strlen_flags() const;

  u16 nt() const { return _nt_addr; }
  void toggle_hidden();
  bool immediate() const;

private:
  Interpreter *_interp;
  u16 _nt_addr;
};

struct NativeDictEntry {
  NativeOpcode h;
  NiceDictHeader hdr;
  u16 code0() const { return hdr.code0(); }
  u16 pcode() const { return hdr.pcode(); }
};

// Implement using C++ iterator tags
struct DictionaryIterator {
  DictionaryIterator(Interpreter *interp, u16 start_addr) : _interp(interp), _link(start_addr) {}
  DictionaryIterator(Interpreter *interp) : _interp(interp), _link(interp->cb.latest) {}
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = NiceDictHeader;

  value_type operator*() const noexcept { return NiceDictHeader(_interp, _link); }
  DictionaryIterator &operator++() noexcept {
    auto hdr = std::bit_cast<const RawDictHeader *>(_interp->memory.data() + _link);
    _link = hdr->link;
    return *this;
  }
  DictionaryIterator operator++(int) {
    auto prev = *this;
    ++*this;
    return prev;
  }
  bool operator==(const DictionaryIterator &other) const noexcept = default;
  u16 link() const { return _link; }

private:
  Interpreter *_interp;
  u16 _link;
};

inline DictionaryIterator begin(Interpreter *interp) { return DictionaryIterator(interp, interp->cb.latest); }
inline DictionaryIterator end(Interpreter *interp) { return DictionaryIterator(interp, 0); }

// If 0, will auto-fill codeword
NiceDictHeader dict_insert(Interpreter *i, std::string_view name, Flags flags, std::span<const u16> code = {});
// Write out the header, up-to and not including codeword.
NiceDictHeader dict_header(Interpreter *i, std::string_view name, Flags flags);

NativeDictEntry dict_insert_native(Interpreter *i, NativeOpcode h, Flags flags, std::string_view name = "");