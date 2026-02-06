/*
 *  Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once
#include <iostream>
#include "core/integers.h"
#include "core/math/geom/point.hpp"
#include "core/math/geom/rectangle.hpp"

namespace pepp::core {
// Represents an 8x8 grid of occupancy bits, stored as a single u64 for ease of copying.
// It is stored in a row-major order, with bit 0 representing (0,0) and bit 63 representing (7,7).
// Thus, the first byte represents row 0, with bit 0 being column 0 and bit 7 being column 7.
// This is reversed compared to how bytes are usually printed, so care must be taken when visualizing the grid or
// modifying row bits directly.
// Constructors and operators for geometry primitives are provided for ease of use.
class DenseOccupancyGrid {
  // A generally helpful primer on some of the evil bit-twiddling techniques used here:
  // http://graphics.stanford.edu/~seander/bithacks.html
  // These bithacks are also mirrored in our local /docs/research folder
public:
  DenseOccupancyGrid() noexcept;
  explicit DenseOccupancyGrid(u64 grid) noexcept;
  explicit DenseOccupancyGrid(Point<u8> pt) noexcept;
  explicit DenseOccupancyGrid(Rectangle<u8> rect) noexcept;
  // If you want an all 0 or all 1 grid, please use these methods rather than the u64 ctor.
  // These methods better describe intent, and do not rely on the caller knowing the underlying implementation.
  static DenseOccupancyGrid zeroes() noexcept;
  static DenseOccupancyGrid ones() noexcept;
  DenseOccupancyGrid(const DenseOccupancyGrid &) noexcept = default;
  DenseOccupancyGrid &operator=(const DenseOccupancyGrid &) noexcept = default;
  DenseOccupancyGrid(DenseOccupancyGrid &&) noexcept = default;
  DenseOccupancyGrid &operator=(DenseOccupancyGrid &&) noexcept = default;

  // Number of 1 bits in the grid
  u8 count() const noexcept;
  // Is the grid all 0s? Conceptually equivalent to count() == 0, but will compile to a faster instruction.
  bool empty() const noexcept;
  // Is the grid all 1s? Conceptually equivalent to count() == 64, but will compile to a faster instruction.
  bool full() const noexcept;

  // Translate the grid in the given direction by the given amount.
  // shift_ create a new instance with the shift applied
  // shifted_ modified this instance in place and returns *this.
  // Shift amounts >= 8 will result in an all-zero grid.
  DenseOccupancyGrid shift_left(u8 amount) const noexcept;
  DenseOccupancyGrid &shifted_left(u8 amount) noexcept;
  DenseOccupancyGrid shift_right(u8 amount) const noexcept;
  DenseOccupancyGrid &shifted_right(u8 amount) noexcept;
  DenseOccupancyGrid shift_up(u8 amount) const noexcept;
  DenseOccupancyGrid &shifted_up(u8 amount) noexcept;
  DenseOccupancyGrid shift_down(u8 amount) const noexcept;
  DenseOccupancyGrid &shifted_down(u8 amount) noexcept;

  // Return the bits that comprise a row, sign bit/msb is column 0, lsb is column 7.
  // WARNING: this bit order is reversed compared to a normal byte.
  u8 row_bits(u8 row) const noexcept;
  // Replace this instace's row bits with the given bits.
  void set_row_bits(u8 row, u8 bits) noexcept;
  // Return a copy of this instance with the given row bits set.
  DenseOccupancyGrid with_row_bits(u8 row, u8 bits) const noexcept;
  // Same API as row_bits, but for columns.
  u8 column_bits(u8 column) const noexcept;
  void set_column_bits(u8 column, u8 bits) noexcept;
  DenseOccupancyGrid with_column_bits(u8 column, u8 bits) const noexcept;

  // ~ is elementwise flip / negate
  // & is an elementwise and (i.e., a test)
  // - is equivalent to lhs & ~rhs (i.e., a clear)
  // | is an elementwise or (i.e., a set)
  // ^ is an elementwise xor (i.e., a toggle)

  /*
   * Begin bitwise operations, organized by if it's a self-modifying operator, then RHS type,
   * then operator type. Intentionally do not provide bit index version, because I'm afraid of DenseOccupancyGrid&5
   * compiling would be a terrible idea.
   *
   * Note the absence of << and >>, because we can shift in 4 directions.
   * Nor do we include a bool conversion -- use the more explicit empty() or full()
   */
  DenseOccupancyGrid operator~() const noexcept;

  DenseOccupancyGrid operator&(const DenseOccupancyGrid &other) const noexcept;
  DenseOccupancyGrid operator-(const DenseOccupancyGrid &other) const noexcept;
  DenseOccupancyGrid operator|(const DenseOccupancyGrid &other) const noexcept;
  DenseOccupancyGrid operator^(const DenseOccupancyGrid &other) const noexcept;

  DenseOccupancyGrid operator&(const Rectangle<u8> &other) const noexcept;
  DenseOccupancyGrid operator-(const Rectangle<u8> &other) const noexcept;
  DenseOccupancyGrid operator|(const Rectangle<u8> &other) const noexcept;
  DenseOccupancyGrid operator^(const Rectangle<u8> &other) const noexcept;

  DenseOccupancyGrid operator&(const Point<u8> &other) const noexcept;
  DenseOccupancyGrid operator-(const Point<u8> &other) const noexcept;
  DenseOccupancyGrid operator|(const Point<u8> &other) const noexcept;
  DenseOccupancyGrid operator^(const Point<u8> &other) const noexcept;

  DenseOccupancyGrid &operator&=(const DenseOccupancyGrid &other) noexcept;
  DenseOccupancyGrid &operator-=(const DenseOccupancyGrid &other) noexcept;
  DenseOccupancyGrid &operator|=(const DenseOccupancyGrid &other) noexcept;
  DenseOccupancyGrid &operator^=(const DenseOccupancyGrid &other) noexcept;

  DenseOccupancyGrid &operator&=(const Rectangle<u8> &other) noexcept;
  DenseOccupancyGrid &operator-=(const Rectangle<u8> &other) noexcept;
  DenseOccupancyGrid &operator|=(const Rectangle<u8> &other) noexcept;
  DenseOccupancyGrid &operator^=(const Rectangle<u8> &other) noexcept;

  DenseOccupancyGrid &operator&=(const Point<u8> &other) noexcept;
  DenseOccupancyGrid &operator-=(const Point<u8> &other) noexcept;
  DenseOccupancyGrid &operator|=(const Point<u8> &other) noexcept;
  DenseOccupancyGrid &operator^=(const Point<u8> &other) noexcept;
  // There is no obvious partial/total ordering to me, so delete.
  auto operator<=>(const DenseOccupancyGrid &other) const noexcept = delete;
  // But equality is fairly obvious.
  bool operator==(const DenseOccupancyGrid &other) const noexcept = default;

  /* Here as some examples of these tansformations affect a sample matrix:
   * Initial    Transposed  Mirrored_X  Mirrored_Y
   * 00000000    00000000    00000000    00000000
   * 00111110    01100000    00000000    01111100
   * 00111110    01100000    00000000    01111100
   * 00000000    01100000    00000000    00000000
   * 00000000    01100000    00000000    00000000
   * 00000000    01100000    00111110    00000000
   * 00000000    00000000    00111110    00000000
   * 00000000    00000000    00000000    00000000
   *
   * Transpose swaps rows and columns, where mirror "flips" about an axis.
   * The methods that return by value leave the original instance unmodified
   * while those thatreturn references modify *this in place and return it.
   */
  DenseOccupancyGrid transpose() const noexcept;
  DenseOccupancyGrid &transposed() noexcept;
  DenseOccupancyGrid mirror_x() const noexcept;
  DenseOccupancyGrid &mirrored_x() noexcept;
  DenseOccupancyGrid mirror_y() const noexcept;
  DenseOccupancyGrid &mirrored_y() noexcept;

private:
  // Bits stored 0..7 within a byte and rows stored 7..0 in increasing significance.
  // Printing to std streams should print as if [0,0] is top right.
  u64 _grid;
};

// Compute the bit position in the u64 for the given (x,y) coordinate.
// Would usually hide implementation detail in cpp, but I want this to be easy to inline.
inline constexpr u8 bit_index(u8 x, u8 y) noexcept { return 8 * y + x; }
// Bit index of the first bit in the given row.
inline constexpr u8 row_index(u8 row_num) noexcept { return bit_index(0, row_num); }
// Helper which formats a grid into an 8x8 grid of 0/1 characters.
std::ostream &operator<<(std::ostream &os, const DenseOccupancyGrid &grid) noexcept;

} // namespace pepp::core
