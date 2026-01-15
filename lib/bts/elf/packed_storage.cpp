#include "packed_storage.hpp"
#include "packed_ops.hpp"

pepp::bts::AStorage::~AStorage() = default;

u64 pepp::bts::BlockStorage::append(bits::span<const u8> data) {
  auto offset = _storage.size();
  _storage.insert(_storage.end(), data.begin(), data.end());
  return offset;
}

u64 pepp::bts::BlockStorage::allocate(u64 size, u8 fill) {
  auto ret = _storage.size();
  _storage.resize(ret + size, fill);
  return ret;
}

void pepp::bts::BlockStorage::set(u64 offset, bits::span<const u8> data) {
  if (offset + data.size() > _storage.size()) throw std::out_of_range("BlockStorage::set out of range");
  std::memcpy(_storage.data() + offset, data.data(), data.size());
}

bits::span<u8> pepp::bts::BlockStorage::get(u64 offset, u64 length) noexcept {
  if (offset + length > _storage.size()) return {};
  return bits::span<u8>((u8 *)_storage.data() + offset, length);
}

bits::span<const u8> pepp::bts::BlockStorage::get(u64 offset, u64 length) const noexcept {
  if (offset + length > _storage.size()) return {};
  return bits::span<const u8>((const u8 *)_storage.data() + offset, length);
}

u64 pepp::bts::BlockStorage::size() const noexcept { return _storage.size(); }

void pepp::bts::BlockStorage::clear(u64 reserve) {
  _storage.clear();
  if (reserve > 0) _storage.reserve(reserve);
}

u64 pepp::bts::BlockStorage::calculate_layout(std::vector<LayoutItem> &layout, u32 dst_offset) const {
  if (_storage.empty()) return dst_offset;
  layout.emplace_back(LayoutItem{dst_offset, bits::span<const u8>{(const u8 *)_storage.data(), _storage.size()}});
  return dst_offset + _storage.size();
}

u64 pepp::bts::BlockStorage::find(bits::span<const u8> needle) const noexcept {
  auto it = std::search(_storage.begin(), _storage.end(), needle.begin(), needle.end());
  return (it == _storage.end()) ? 0 : static_cast<std::size_t>(it - _storage.begin());
}

u64 pepp::bts::BlockStorage::strlen(u64 offset) const noexcept {
  const char *start = (const char *)_storage.data() + offset, *end = start;
  while (end < (const char *)_storage.data() + _storage.size() && *end != '\0') ++end;
  return end - start;
}
