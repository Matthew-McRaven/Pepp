/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "core/formats/elf/packed_ops.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <ostream>
#include "core/math/bitmanip/copy.hpp"

u64 pepp::bts::size_for_layout(const std::vector<pepp::bts::LayoutItem> &layout) noexcept {
  u64 ret = 0;
  for (const auto &item : layout) ret = std::max(ret, item.offset + item.data.size());
  return ret;
}

void pepp::bts::write(std::span<u8> out, const std::vector<LayoutItem> &layout) {
  for (const auto &item : layout) {
    if (item.offset + item.data.size() > out.size())
      throw std::runtime_error("Elf::write: layout item exceeds output size");
    std::span<u8> chunk = out.subspan(item.offset, item.data.size());
    bits::memcpy<u8, u8>(chunk, {item.data});
  }
}

void pepp::bts::write(std::ostream &out, std::vector<LayoutItem> layout) {
  static constexpr std::array<char, 256> zeros{};
  const u64 end = size_for_layout(layout);
  u64 at = 0;
  const auto pad_to = [&](u64 offset) {
    while (at < offset) {
      const auto count = std::min<u64>(offset - at, zeros.size());
      out.write(zeros.data(), static_cast<std::streamsize>(count));
      at += count;
    }
  };
  std::ranges::sort(layout, {}, &LayoutItem::offset);
  for (const auto &item : layout) {
    if (item.data.empty()) continue;
    if (item.offset < at) throw std::runtime_error("Elf::write: layout items overlap");
    pad_to(item.offset);
    out.write(reinterpret_cast<const char *>(item.data.data()), static_cast<std::streamsize>(item.data.size()));
    at += item.data.size();
  }
  pad_to(end); // An empty trailing item still extends the file, as it does for the span overload.
}

std::shared_ptr<pepp::bts::MappedFile> pepp::bts::write_mmap(const std::string &path,
                                                             const std::vector<LayoutItem> &layout) {
  // Truncate first: a mapping only ever grows its file, so a longer previous image would leave a stale tail behind.
  if (std::ofstream create(path, std::ios::binary | std::ios::trunc); !create)
    throw std::runtime_error("Elf::write_image: could not open " + path);
  const u64 size = size_for_layout(layout);
  auto file = MappedFile::open_readwrite(path);
  if (size == 0) return file;

  auto slice = file->slice(0, size);
  auto span = slice->get();
  if (span.size() < size) throw std::runtime_error("Elf::write_image: could not map " + path);
  std::ranges::fill(span, u8(0)); // The gaps a layout leaves between its items.
  write(span, layout);
  slice->flush();
  return file;
}
