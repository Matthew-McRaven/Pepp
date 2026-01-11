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

#pragma once
#include "../utils.hpp"
#include "sim3/api/traced/trace_buffer.hpp"
#include "sim3/api/traced/trace_frame.hpp"
#include "bts/bitmanip/copy.hpp"
namespace sim::trace2 {

template <typename T>
concept FrameHeader = is_variant_member<T, sim::api2::frame::Header>::value && !std::is_same<T, std::monostate>::value;
template <typename T>
concept NotFrameHeader =
    !is_variant_member<T, sim::api2::frame::Header>::value || std::is_same<T, std::monostate>::value;

struct AsFrameHeader {
  template <FrameHeader T> api2::frame::Header operator()(const T &hdr) const { return hdr; }
  template <NotFrameHeader T> api2::frame::Header operator()(const T &) { return {}; }
};

struct IsFrameHeader {
  template <FrameHeader T> bool operator()(const T &) const { return true; }
  template <NotFrameHeader T> bool operator()(const T &) { return false; }
};
bool is_frame_header(const sim::api2::trace::Fragment &f);

struct AsFragment {
  template <FrameHeader T> api2::trace::Fragment operator()(const T &hdr) const { return hdr; }
  template <NotFrameHeader T> api2::trace::Fragment operator()(const T &) { return {}; }
};

api2::trace::Fragment as_fragment(const sim::api2::frame::Header &hdr);

template <typename T>
concept HasLength = requires(T t) {
  { t.length } -> std::convertible_to<decltype(t.length)>;
};

class UpdateFrameLength {
public:
  UpdateFrameLength(quint16 length, sim::api2::frame::Header &out) : _length(length), _out(out){};
  template <HasLength Header> void operator()(const Header &header) {
    Header copy = header;
    copy.length = _length;
    _out = copy;
  }
  void operator()(const auto &header){};

private:
  quint16 _length = 0;
  sim::api2::frame::Header &_out;
};

struct GetFrameLength {
  template <HasLength Header> quint16 operator()(const Header &header) const { return header.length; }
  quint16 operator()(const auto &header) const { return 0; };
};

template <typename T>
concept HasBackOffset = requires(T t) {
  { t.back_offset } -> std::convertible_to<decltype(t.back_offset)>;
};
class UpdateFrameBackOffset {
public:
  UpdateFrameBackOffset(quint16 back_offset, sim::api2::frame::Header &out) : _back_offset(back_offset), _out(out){};
  template <HasBackOffset Header> void operator()(const Header &header) {
    Header copy = header;
    copy.back_offset = _back_offset;
    _out = copy;
  }
  void operator()(const auto &header){};

private:
  quint16 _back_offset = 0;
  sim::api2::frame::Header &_out;
};

struct GetFrameBackOffset {
  template <HasBackOffset Header> quint16 operator()(const Header &header) const { return header.back_offset; }
  quint16 operator()(const auto &header) const { return 0; };
};
} // namespace sim::trace2
