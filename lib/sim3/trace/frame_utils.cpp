/*
 * /Copyright (c) 2023-2025. Stanley Warford, Matthew McRaven
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

#include "./frame_utils.hpp"

bool sim::trace2::is_frame_header(const sim::api2::trace::Fragment &f) { return std::visit(IsFrameHeader{}, f); }

sim::api2::trace::Fragment sim::trace2::as_fragment(const api2::frame::Header &hdr) {
  return std::visit(AsFragment{}, hdr);
}
