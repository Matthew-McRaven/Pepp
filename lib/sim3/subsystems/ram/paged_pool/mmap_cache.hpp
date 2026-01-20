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
#include <array>
#include <cstdint>
#include <vector>
#include "core/isa/riscv/rv_types.hpp"

namespace riscv
{
	template <AddressType address_t>
	struct MMapCache
	{
		struct Range {
			address_t addr = 0x0;
			address_t size = 0u;

			constexpr bool empty() const noexcept { return size == 0u; }
			// Invalidate if one of the ranges is in the other (both ways!)
			constexpr bool within(address_t mem, address_t memsize) const noexcept {
				return ((this->addr >= mem) && (this->addr + this->size <= mem + memsize))
					|| ((mem >= this->addr) && (mem + memsize <= this->addr + this->size));
			}
			constexpr bool equals(address_t mem, address_t memsize) const noexcept {
				return (this->addr == mem) && (this->addr + this->size == mem + memsize);
			}
		};

		Range find(address_t size)
		{
			auto it = m_lines.begin();
			while (it != m_lines.end())
			{
				auto& r = *it;
				if (!r.empty())
				{
					if (r.size >= size) {
						const Range result { r.addr, size };
						if (r.size > size) {
							r.addr += size;
							r.size -= size;
						} else {
							m_lines.erase(it);
						}
						return result;
					}
				}
				++it;
			}
			return Range{};
		}

		void invalidate(address_t addr, address_t size)
		{
			auto it = m_lines.begin();
			while (it != m_lines.end())
			{
				const auto r = *it;
				if (r.within(addr, size))
				{
					bool equals = r.equals(addr, size);
					it = m_lines.erase(it);
					if (equals) return;
				}
				else ++it;
			}
		}

		void insert(address_t addr, address_t size)
		{
			/* Extend the back range? */
			if (!m_lines.empty()) {
				if (m_lines.back().addr + m_lines.back().size == addr) {
					m_lines.back().size += size;
					return;
				}
			}

			m_lines.push_back({addr, size});
		}

	private:
		std::vector<Range> m_lines {};
	};

} // riscv
