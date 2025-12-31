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
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include "sim3/common_macros.hpp"

/**
 * Container that is designed to hold pointers to guest data, which can
 * be sequentialized in various ways.
**/

namespace riscv
{
	struct Buffer
	{
		bool is_sequential() const noexcept { return m_overflow.empty(); }
		const std::string_view& strview() const noexcept { return m_data; }
		const char* c_str() const noexcept { return strview().data(); }
		const char* data() const noexcept { return strview().data(); }
		size_t      size() const noexcept { return m_len; }

		size_t copy_to(char* dst, size_t dstlen) const;
		void   copy_to(std::vector<uint8_t>&) const;
    void _foreach(std::function<void(const char *, size_t)> cb);
    std::string to_string() const;

    Buffer() = default;
		void append_page(const char* data, size_t len);

	private:
		std::string_view m_data;
		std::vector<std::pair<const char*, size_t>> m_overflow;
		size_t m_len  = 0; /* Total length */
	};

	inline size_t Buffer::copy_to(char* dst, size_t maxlen) const
	{
		if (UNLIKELY(m_data.size() > maxlen))
			return 0;
		size_t len = m_data.size();
		std::copy(m_data.begin(), m_data.end(), dst);

		for (const auto& entry : m_overflow) {
			if (UNLIKELY(len + entry.second > maxlen)) break;
			std::copy(entry.first, entry.first + entry.second, &dst[len]);
			len += entry.second;
		}

		return len;
	}
	inline void Buffer::copy_to(std::vector<uint8_t>& vec) const
	{
		vec.insert(vec.end(), m_data.begin(), m_data.end());
		for (const auto& entry : m_overflow) {
			vec.insert(vec.end(), entry.first, entry.first + entry.second);
		}
	}

  inline void Buffer::_foreach(std::function<void(const char *, size_t)> cb) {
    cb(m_data.data(), m_data.size());
		for (const auto& entry : m_overflow) {
			cb(entry.first, entry.second);
		}
  }

  inline void Buffer::append_page(const char* buffer, size_t len)
	{
		if (m_data.empty())
		{
			m_data = {buffer, len};
			m_len = len;
			return;
		}
		else if (&*m_data.end() == buffer)
		{
			// In some cases we can continue the last entry
			m_len  += len;
			m_data = {m_data.data(), m_len};
			return;
		}

		// In some cases we can continue the last entry
		if (!m_overflow.empty()) {
			auto& last = m_overflow.back();
			if (last.first + last.second == buffer) {
				last.second += len;
				m_len += len;
				return;
			}
		}
		// Otherwise, append new entry
		m_len += len;
		m_overflow.emplace_back(buffer, len);
	}

	inline std::string Buffer::to_string() const
	{
		if (is_sequential()) {
			return std::string(m_data);
		}

		std::string result;
		result.reserve(this->m_len);
		result.append(m_data);
		for (const auto& entry : m_overflow) {
			result.append(entry.first, entry.first + entry.second);
		}
		return result;
	}
}
