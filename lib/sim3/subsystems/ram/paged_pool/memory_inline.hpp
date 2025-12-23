/*
 * Copyright (c) 2025-2026 J. Stanley Warford, Matthew McRaven
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
 *
 * Copyright (c) 2024, Alf-André Walla
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the BSD 3-clause license
 * along with this program. If not, see
 * <https://opensource.org/license/bsd-3-clause>
 */
#pragma once
#include "../paged_pool.hpp"
#include "sim3/cores/riscv/rvv_registers.hpp"
namespace riscv {
// Force-align memory operations to their native alignments
template <typename T> constexpr inline size_t memory_align_mask() {
	if constexpr (force_align_memory)
		return size_t(Page::size() - 1) & ~size_t(sizeof(T)-1);
	else
		return size_t(Page::size() - 1);
}

template <AddressType address_t>
template <typename T> inline
T Memory<address_t>::read(address_t address)
{
	const auto offset = address & memory_align_mask<T>();
	if constexpr (unaligned_memory_slowpaths) {
		if (UNLIKELY(offset+sizeof(T) > Page::size())) {
			T value;
			memcpy_out(&value, address, sizeof(T));
			return value;
		}
	}
	else if constexpr (flat_readwrite_arena) {
		if (LIKELY(address - RWREAD_BEGIN < memory_arena_read_boundary())) {
			if constexpr (sizeof(T) >= 32) {
				// Reads and writes using vectors might have alignment requirements
				auto* arena = (VectorLane *)m_arena.data;
				return arena[RISCV_SPECSAFE(address / sizeof(VectorLane))];
			}
			return *(T *)&((const char*)m_arena.data)[RISCV_SPECSAFE(address)];
		}
		[[unlikely]];
	}

	const auto& pagedata = cached_readable_page(address, sizeof(T));
	return pagedata.template aligned_read<T>(offset);
}

template <AddressType address_t>
template <typename T> inline
T& Memory<address_t>::writable_read(address_t address)
{
	if constexpr (flat_readwrite_arena) {
		if (LIKELY(address - initial_rodata_end() < memory_arena_write_boundary())) {
			return *(T *)&((char*)m_arena.data)[RISCV_SPECSAFE(address)];
		}
		[[unlikely]];
	}

	auto& pagedata = cached_writable_page(address);
	return pagedata.template aligned_read<T>(address & memory_align_mask<T>());
}

template <AddressType address_t>
template <typename T> inline
void Memory<address_t>::write(address_t address, T value)
{
	const auto offset = address & memory_align_mask<T>();
	if constexpr (unaligned_memory_slowpaths) {
		if (UNLIKELY(offset+sizeof(T) > Page::size())) {
			memcpy(address, &value, sizeof(T));
			return;
		}
	}
	else if constexpr (flat_readwrite_arena) {
		if (LIKELY(address - initial_rodata_end() < memory_arena_write_boundary())) {
#ifdef RISCV_EXT_VECTOR
			if constexpr (sizeof(T) >= 32) {
				// Reads and writes using vectors might have alignment requirements
				auto* arena = (VectorLane *)m_arena.data;
				arena[RISCV_SPECSAFE(address / sizeof(VectorLane))] = value;
			} else
#endif
				*(T *)&((char*)m_arena.data)[RISCV_SPECSAFE(address)] = value;
			return;
		}
	}

	const auto pageno = page_number(address);
	auto& entry = m_wr_cache;
	if (entry.pageno == pageno) {
		entry.page->template aligned_write<T>(offset, value);
		return;
	}

	auto& page = create_writable_pageno(pageno);
	if (LIKELY(page.attr.is_cacheable())) {
		entry = {pageno, &page.page()};
	} else if constexpr (memory_traps_enabled && sizeof(T) <= 16) {
		if (UNLIKELY(page.has_trap())) {
			page.trap(offset, sizeof(T) | TRAP_WRITE, value);
			return;
		}
	}
	page.page().template aligned_write<T>(offset, value);
}

template <AddressType address_t>
template <typename T> inline
void Memory<address_t>::write_paging(address_t address, T value)
{
	const auto offset = address & memory_align_mask<T>();
	const auto pageno = page_number(address);
	auto& entry = m_wr_cache;
	if (entry.pageno == pageno) {
		entry.page->template aligned_write<T>(offset, value);
		return;
	}

	auto& page = create_writable_pageno(pageno);
	if (LIKELY(page.attr.is_cacheable())) {
		entry = {pageno, &page.page()};
	} else if constexpr (memory_traps_enabled && sizeof(T) <= 16) {
		if (UNLIKELY(page.has_trap())) {
			page.trap(offset, sizeof(T) | TRAP_WRITE, value);
			return;
		}
	}
	page.page().template aligned_write<T>(offset, value);
}


template <AddressType address_t>
inline address_t Memory<address_t>::resolve_address(std::string_view name) const
{
	auto* sym = resolve_symbol(name);
	return (sym) ? sym->st_value : 0x0;
}

template <AddressType address_t>
inline address_t Memory<address_t>::resolve_section(const char* name) const
{
	auto* shdr = this->section_by_name(name);
	if (shdr) return shdr->sh_addr;
	return 0x0;
}

template <AddressType address_t>
inline address_t Memory<address_t>::exit_address() const noexcept
{
	return this->m_exit_address;
}

template <AddressType address_t>
inline void Memory<address_t>::set_exit_address(address_t addr)
{
	this->m_exit_address = addr;
}

template <AddressType address_t>
inline std::shared_ptr<DecodedExecuteSegment<address_t>>& Memory<address_t>::exec_segment_for(address_t vaddr)
{
	// Check main execute segment first, it's always present
	if (m_main_exec_segment && m_main_exec_segment->is_within(vaddr)) return m_main_exec_segment;
	for (auto& segment : m_exec) {
		if (segment && segment->is_within(vaddr)) return segment;
	}
	return CPU<address_t>::empty_execute_segment();
}
} // namespace riscv
