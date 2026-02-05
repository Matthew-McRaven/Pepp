#include "core/math/geom/occupancy_grid.hpp"
#include <bit>
#include <bitset>
#include "core/math/bitmanip/swap.hpp"

pepp::core::OccupancyGrid::OccupancyGrid() noexcept : _grid(0) {}

pepp::core::OccupancyGrid::OccupancyGrid(u64 grid) noexcept : _grid(grid) {}

pepp::core::OccupancyGrid::OccupancyGrid(Point<u8> pt) noexcept : _grid(0) {
  u8 y = pt.y(), x = pt.x();
  const u8 index = bit_index(x, y);
  if (x >= 8 || y >= 8) _grid = 0;
  else _grid = u64{1} << index;
}

pepp::core::OccupancyGrid::OccupancyGrid(Rectangle<u8> rect) noexcept : _grid(0) {
  // Assume a rect at x==2..5, we would want the bit pattern 0011 1000 in each row
  const auto low_x = rect.x().lower(), high_x = rect.x().upper();
  // We can compute the first set bit by logical shifting ~0 right by the lower x value.
  // In the example, we would get the mask 0011 1111.
  const u8 low_mask = static_cast<u8>(-1) >> (7 - high_x);
  // The last set bit can be computed by logical shifting ~0 left by the upper x value.
  // In the example, we would get the mask 1111 1000.
  const u8 high_mask = static_cast<u8>(-1) << (low_x);
  // Combining those bitmasks with & gives us the row bitmask 0011 1000.
  const u8 row_bits = low_mask & high_mask;
  for (int row = rect.y().lower(); row <= rect.y().upper(); ++row) set_row_bits(row, row_bits);
}

pepp::core::OccupancyGrid pepp::core::OccupancyGrid::ones() noexcept { return OccupancyGrid(static_cast<u64>(-1)); }

pepp::core::OccupancyGrid pepp::core::OccupancyGrid::zeroes() noexcept { return OccupancyGrid(0); }

u8 pepp::core::OccupancyGrid::count() const noexcept { return static_cast<u8>(std::popcount(_grid)); }

bool pepp::core::OccupancyGrid::empty() const noexcept { return _grid == 0; }

bool pepp::core::OccupancyGrid::full() const noexcept { return _grid == ~u64{0}; }

u8 pepp::core::OccupancyGrid::row_bits(u8 row) const noexcept {
  if (row >= 8) return 0;
  return static_cast<u8>((_grid >> row_index(row)) & 0xFFu);
}

void pepp::core::OccupancyGrid::set_row_bits(u8 row, u8 bits) noexcept {

  if (row >= 8) return;
  const u64 mask = 0xFFuLL << row_index(row);
  _grid = (_grid & ~mask) | (static_cast<u64>(bits) << row_index(row));
}

pepp::core::OccupancyGrid pepp::core::OccupancyGrid::with_row_bits(u8 row, u8 bits) const noexcept {
  OccupancyGrid out = *this;
  out.set_row_bits(row, bits);
  return out;
}

namespace {
const u64 columns_least_bit_mask = 0x0101010101010101ULL;
}

u8 pepp::core::OccupancyGrid::column_bits(u8 column) const noexcept {
  if (column >= 8) return 0;
  const u64 shifted = _grid >> column;
  // Shift the target column bit to be in each row's "1" place, then select all those ones
  const u64 masked = shifted & columns_least_bit_mask;
  // For each bit, use a multiply to shift it to the correct position, which is a columnwise bit gather
  // All 8 bytes should have the same value, so just pick 1. This approach uses SWAR (SIMD Within A Register) / packed
  // SIMD techniques.
  // const u64 mult = masked * 0x8040201008040201ULL;
  const u64 mult = masked * 0x0102040810204080ULL; // Rotate bits at the same time as packing them
  //  usually we think of the LSB as being on the right, but we store it on the left.
  const u64 mult56 = mult >> 56;
  return static_cast<u8>(mult56);
}

void pepp::core::OccupancyGrid::set_column_bits(u8 column, u8 bits) noexcept {
  // Our internal representation stores LSB first, but typical byte format is MSB.
  // bits = bits::reverse_bits(bits);
  // Create a copy of the grid with the target column zeroed out
  const u64 column_mask = columns_least_bit_mask << column;
  u64 out = _grid & ~column_mask;
  // Do a byte-wise scatter of the bits into the target column
  for (unsigned row = 0; row < 8; ++row) {
    const u8 place = (bits >> row) & 1u;
    const u8 shift_amt = bit_index(column, row);
    out |= (u64(place) << shift_amt);
  }
  _grid = out;
}

pepp::core::OccupancyGrid pepp::core::OccupancyGrid::with_column_bits(u8 column, u8 bits) const noexcept {
  OccupancyGrid out = *this;
  out.set_column_bits(column, bits);
  return out;
}
pepp::core::OccupancyGrid pepp::core::OccupancyGrid::shift_left(u8 amount) const noexcept {
  if (amount >= 8) return OccupancyGrid(0);
  std::uint64_t out = 0;
  for (int it = 0; it < 8; ++it) {
    u8 row = row_bits(it) << amount;
    out |= (std::uint64_t(row) << row_index(it));
  }
  return OccupancyGrid{out};
}

pepp::core::OccupancyGrid &pepp::core::OccupancyGrid::shifted_left(u8 amount) noexcept {
  return *this = shift_left(amount);
}

pepp::core::OccupancyGrid pepp::core::OccupancyGrid::shift_right(u8 amount) const noexcept {
  if (amount >= 8) return OccupancyGrid(0);
  std::uint64_t out = 0;
  for (int it = 0; it < 8; ++it) {
    u8 row = row_bits(it) >> amount;
    out |= (std::uint64_t(row) << row_index(it));
  }
  return OccupancyGrid{out};
}

pepp::core::OccupancyGrid &pepp::core::OccupancyGrid::shifted_right(u8 amount) noexcept {
  return *this = shift_right(amount);
}

pepp::core::OccupancyGrid pepp::core::OccupancyGrid::shift_up(u8 amount) const noexcept {
  if (amount >= 8) return OccupancyGrid(0);
  return OccupancyGrid(_grid >> (amount * 8));
}

pepp::core::OccupancyGrid &pepp::core::OccupancyGrid::shifted_up(u8 amount) noexcept {
  return *this = shift_up(amount);
}

pepp::core::OccupancyGrid pepp::core::OccupancyGrid::shift_down(u8 amount) const noexcept {
  if (amount >= 8) return OccupancyGrid(0);
  return OccupancyGrid(_grid << (amount * 8));
}

pepp::core::OccupancyGrid &pepp::core::OccupancyGrid::shifted_down(u8 amount) noexcept {
  return *this = shift_down(amount);
}

pepp::core::OccupancyGrid pepp::core::OccupancyGrid::operator~() const noexcept { return OccupancyGrid(~_grid); }

pepp::core::OccupancyGrid pepp::core::OccupancyGrid::operator&(const OccupancyGrid &other) const noexcept {
  return OccupancyGrid(_grid & other._grid);
}
pepp::core::OccupancyGrid pepp::core::OccupancyGrid::operator-(const OccupancyGrid &other) const noexcept {
  return OccupancyGrid(_grid & ~other._grid);
}
pepp::core::OccupancyGrid pepp::core::OccupancyGrid::operator|(const OccupancyGrid &other) const noexcept {
  return OccupancyGrid(_grid | other._grid);
}
pepp::core::OccupancyGrid pepp::core::OccupancyGrid::operator^(const OccupancyGrid &other) const noexcept {
  return OccupancyGrid(_grid ^ other._grid);
}

pepp::core::OccupancyGrid pepp::core::OccupancyGrid::operator&(const Rectangle<u8> &other) const noexcept {
  return *this & OccupancyGrid(other);
}
pepp::core::OccupancyGrid pepp::core::OccupancyGrid::operator-(const Rectangle<u8> &other) const noexcept {
  return *this - OccupancyGrid(other);
}
pepp::core::OccupancyGrid pepp::core::OccupancyGrid::operator|(const Rectangle<u8> &other) const noexcept {
  return *this | OccupancyGrid(other);
}
pepp::core::OccupancyGrid pepp::core::OccupancyGrid::operator^(const Rectangle<u8> &other) const noexcept {
  return *this ^ OccupancyGrid(other);
}

pepp::core::OccupancyGrid pepp::core::OccupancyGrid::operator&(const Point<u8> &other) const noexcept {
  return *this & OccupancyGrid(other);
}
pepp::core::OccupancyGrid pepp::core::OccupancyGrid::operator-(const Point<u8> &other) const noexcept {
  return *this - OccupancyGrid(other);
}
pepp::core::OccupancyGrid pepp::core::OccupancyGrid::operator|(const Point<u8> &other) const noexcept {
  return *this | OccupancyGrid(other);
}
pepp::core::OccupancyGrid pepp::core::OccupancyGrid::operator^(const Point<u8> &other) const noexcept {
  return *this ^ OccupancyGrid(other);
}

pepp::core::OccupancyGrid &pepp::core::OccupancyGrid::operator&=(const OccupancyGrid &other) noexcept {
  return *this = *this & other;
}
pepp::core::OccupancyGrid &pepp::core::OccupancyGrid::operator-=(const OccupancyGrid &other) noexcept {
  return *this = *this - other;
}
pepp::core::OccupancyGrid &pepp::core::OccupancyGrid::operator|=(const OccupancyGrid &other) noexcept {
  return *this = *this | other;
}
pepp::core::OccupancyGrid &pepp::core::OccupancyGrid::operator^=(const OccupancyGrid &other) noexcept {
  return *this = *this ^ other;
}

pepp::core::OccupancyGrid pepp::core::OccupancyGrid::transpose() const noexcept {
  return OccupancyGrid(_grid).transposed();
}

pepp::core::OccupancyGrid &pepp::core::OccupancyGrid::transposed() noexcept {
  // From: Hacker's Delight, Section 7–3 “Transposing a Bit Matrix
  // swap across distance 7
  std::uint64_t t = (_grid ^ (_grid >> 7)) & 0x00AA00AA00AA00AAull;
  _grid ^= t ^ (t << 7);

  // swap across distance 14
  t = (_grid ^ (_grid >> 14)) & 0x0000CCCC0000CCCCull;
  _grid ^= t ^ (t << 14);

  // swap across distance 28
  t = (_grid ^ (_grid >> 28)) & 0x00000000F0F0F0F0ull;
  _grid ^= t ^ (t << 28);
  return *this;
}

pepp::core::OccupancyGrid pepp::core::OccupancyGrid::mirror_x() const noexcept {
  return OccupancyGrid(_grid).mirrored_x();
}

pepp::core::OccupancyGrid &pepp::core::OccupancyGrid::mirrored_x() noexcept {
  _grid = bits::byteswap(_grid);
  return *this;
}

pepp::core::OccupancyGrid pepp::core::OccupancyGrid::mirror_y() const noexcept {
  return OccupancyGrid(_grid).mirrored_y();
}

pepp::core::OccupancyGrid &pepp::core::OccupancyGrid::mirrored_y() noexcept {
  _grid = bits::byteswap(bits::reverse_bits(_grid));
  return *this;
}

pepp::core::OccupancyGrid &pepp::core::OccupancyGrid::operator&=(const Rectangle<u8> &other) noexcept {
  return *this = *this & other;
}
pepp::core::OccupancyGrid &pepp::core::OccupancyGrid::operator-=(const Rectangle<u8> &other) noexcept {
  return *this = *this - other;
}
pepp::core::OccupancyGrid &pepp::core::OccupancyGrid::operator|=(const Rectangle<u8> &other) noexcept {
  return *this = *this | other;
}
pepp::core::OccupancyGrid &pepp::core::OccupancyGrid::operator^=(const Rectangle<u8> &other) noexcept {
  return *this = *this ^ other;
}

pepp::core::OccupancyGrid &pepp::core::OccupancyGrid::operator&=(const Point<u8> &other) noexcept {
  return *this = *this & other;
}
pepp::core::OccupancyGrid &pepp::core::OccupancyGrid::operator-=(const Point<u8> &other) noexcept {
  return *this = *this - other;
}
pepp::core::OccupancyGrid &pepp::core::OccupancyGrid::operator|=(const Point<u8> &other) noexcept {
  return *this = *this | other;
}
pepp::core::OccupancyGrid &pepp::core::OccupancyGrid::operator^=(const Point<u8> &other) noexcept {
  return *this = *this ^ other;
}

std::ostream &pepp::core::operator<<(std::ostream &os, const OccupancyGrid &grid) noexcept {
  for (int it = 0; it < 8; it++) {
    const u8 row_bits = bits::reverse_bits(grid.row_bits(it));
    os << std::bitset<8>(row_bits) << "\n";
  }
  return os;
}
