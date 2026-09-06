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

#pragma once
#include "core/ds/opaque_handle.hpp"

namespace pepp::bts {

// Big enough to store both an ELF32 word and an ELF64 word.
// Needs to be masked / truncated when serializing to ELF32.
using uxword = u64;
using sxword = i64;

// Specifically not a section index. During serialization we construct a mapping of SectionRefs to actual indices.
using SectionRef = OpaqueHandle<struct SectionRefTag>;
consteval void allow_opaque_handle_increment(SectionRef);

using SegmentRef = OpaqueHandle<struct SegmentRefTag>;
consteval void allow_opaque_handle_increment(SegmentRef);

} // namespace pepp::bts
