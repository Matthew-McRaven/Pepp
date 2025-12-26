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
#include "./memory_helpers_paging.hpp"
#include "sim3/common_macros.hpp"

namespace riscv {
template <AddressType address_t> inline const PageData &Memory<address_t>::cached_readable_page(address_t address, size_t len) const {
  const auto pageno = page_number(address);
  auto &entry = m_rd_cache;
  if (entry.pageno == pageno)
		return *entry.page;

	auto& page = get_readable_pageno(pageno);
	if (LIKELY(page.attr.is_cacheable())) {
		entry = {pageno, &page.page()};
	} else if constexpr (memory_traps_enabled) {
		if (UNLIKELY(page.has_trap())) {
			page.trap(address & (Page::size()-1), len | TRAP_READ, 0);
		}
	}
	return page.page();
}

template <AddressType address_t> inline
PageData& Memory<address_t>::cached_writable_page(address_t address)
{
	const auto pageno = page_number(address);
	auto& entry = m_wr_cache;
	if (entry.pageno == pageno)
		return *entry.page;
	auto& page = create_writable_pageno(pageno);
	if (LIKELY(page.attr.is_cacheable()))
		entry = {pageno, &page.page()};
	return page.page();
}

template <AddressType address_t>
inline const Page& Memory<address_t>::get_page(const address_t address) const
{
	const auto page = page_number(address);
	return get_pageno(page);
}

template <AddressType address_t>
inline const Page& Memory<address_t>::get_exec_pageno(const address_t pageno) const
{
	auto it = m_pages.find(pageno);
	if (LIKELY(it != m_pages.end())) {
		return it->second;
	}
	CPU<address_t>::trigger_exception(EXECUTION_SPACE_PROTECTION_FAULT, pageno * Page::size());
}

template <AddressType address_t>
inline const Page& Memory<address_t>::get_pageno(const address_t pageno) const
{
	auto it = m_pages.find(pageno);
	if (LIKELY(it != m_pages.end())) {
		return it->second;
	}

	return m_page_readf_handler(*this, pageno);
}

template <AddressType address_t> inline void
Memory<address_t>::invalidate_cache(address_t pageno, Page* page) const noexcept
{
	// NOTE: It is only possible to keep the write page as long as
	// the page tables are node-based. In that case, we only have
	// to invalidate the read page when it matches.
	if (m_rd_cache.pageno == pageno) {
		m_rd_cache.pageno = (address_t)-1;
	}
	(void)page;
}
template <AddressType address_t> inline void
Memory<address_t>::invalidate_reset_cache() const noexcept
{
	m_rd_cache.pageno = (address_t)-1;
	m_wr_cache.pageno = (address_t)-1;
}

template <AddressType address_t>
template <typename... Args> inline
Page& Memory<address_t>::allocate_page(const address_t page, Args&&... args)
{
	const auto it = m_pages.try_emplace(
		page,
		std::forward<Args> (args)...
	);
	// Invalidate only this page
	this->invalidate_cache(page, &it.first->second);
	// Return new default-writable page
	return it.first->second;
}

template <AddressType address_t>
inline size_t Memory<address_t>::owned_pages_active() const noexcept
{
	size_t count = 0;
	for (const auto& it : m_pages) {
		if (!it.second.attr.non_owning) count++;
	}
	return count;
}

template <AddressType address_t>
inline void Memory<address_t>::trap(address_t page_addr, mmio_cb_t callback)
{
	// This can probably be improved, but this will force-create
	// a page if it doesn't exist. At least this way the trap will
	// always work. Less surprises this way.
	auto& page = create_writable_pageno(page_number(page_addr));
	// Disabling caching will force the slow-path for the page,
	// and enables page traps when RISCV_DEBUG is enabled.
	page.attr.cacheable = false;
	page.set_trap(callback);
}
} // namespace riscv
