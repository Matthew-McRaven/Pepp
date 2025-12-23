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
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "decoded_exec_segment.hpp"
#include "enums/isa/rv_instruction_list.hpp"
#include "enums/isa/rv_types.hpp"
#include "sim3/common_macros.hpp"
#include "sim3/cores/riscv/instructions/safe_instr_loader.hpp"
#include "sim3/subsystems/ram/paged_pool.hpp"
#include "sim3/utils/crc32.hpp"

namespace riscv {
static constexpr bool VERBOSE_DECODER = false;
static std::mutex handler_idx_mutex;
#ifdef ENABLE_TIMINGS
static inline timespec time_now();
static inline long nanodiff(timespec, timespec);
#define TIME_POINT(x)                                                                                                  \
  [[maybe_unused]] timespec x;                                                                                         \
  if (true) {                                                                                                          \
    asm("" : : : "memory");                                                                                            \
    x = time_now();                                                                                                    \
    asm("" : : : "memory");                                                                                            \
  }
#else
#define TIME_POINT(x) /* x */
#endif

template <AddressType address_t>
struct DecoderData {
	using Handler = instruction_handler<address_t>;

	uint8_t  m_bytecode;
	uint8_t  m_handler;
#ifdef RISCV_EXT_COMPRESSED
	uint16_t idxend  : 8;
	uint16_t icount  : 8;
#else
	uint16_t idxend;
#endif

	uint32_t instr;

	// Switch-based and threaded simulation uses bytecodes.
	PEPP_ALWAYS_INLINE
	auto get_bytecode() const noexcept {
		return this->m_bytecode;
	}
	void set_bytecode(uint16_t num) noexcept {
		this->m_bytecode = num;
	}

	void set_handler(Instruction<address_t> insn) noexcept {
		this->set_insn_handler(insn.handler);
	}
	void set_insn_handler(instruction_handler<address_t> ih) noexcept {
		this->m_handler = handler_index_for(ih);
	}
	void set_invalid_handler() noexcept {
		this->m_handler = 0;
	}
	bool is_invalid_handler() const noexcept {
		return this->m_handler == 0;
	}

	// Used by live-patching to set both bytecode and handler index.
	void set_atomic_bytecode_and_handler(uint8_t bytecode, uint8_t handler_idx) noexcept {
		// XXX: Assumes little-endian
		*(uint16_t* )&m_bytecode = ( handler_idx << 8 ) | bytecode;
	}

  PEPP_ALWAYS_INLINE
  auto block_bytes() const noexcept { return idxend * (compressed_enabled ? 2 : 4); }
  PEPP_ALWAYS_INLINE
  auto instruction_count() const noexcept {
#ifdef RISCV_EXT_COMPRESSED
    return icount;
#else
    return idxend + 1;
#endif
  }

  bool operator==(const DecoderData<address_t> &other) const noexcept {
    return m_bytecode == other.m_bytecode && m_handler == other.m_handler && idxend == other.idxend &&
           instr == other.instr;
  }

  static size_t handler_index_for(Handler new_handler);
  static Handler *get_handlers() noexcept { return &instr_handlers[0]; }

  void atomic_overwrite(const DecoderData<address_t> &other) noexcept {
    static_assert(sizeof(DecoderData<address_t>) == 8, "DecoderData size mismatch");
    *(uint64_t *)this = *(uint64_t *)&other;
  }

private:
  static inline std::array<Handler, 256> instr_handlers;
  static inline std::size_t handler_count = 0;
  static inline std::unordered_map<Handler, size_t> handler_cache;
};

template <AddressType address_t> struct DecoderCache {
  static constexpr size_t DIVISOR = (compressed_enabled) ? 2 : 4;
  static constexpr unsigned SHIFT = (compressed_enabled) ? 1 : 2;

  inline auto &get(size_t idx) noexcept { return cache[idx]; }

  inline auto *get_base() noexcept { return &cache[0]; }

  std::array<DecoderData<address_t>, PageSize / DIVISOR> cache;
};

template <AddressType address_t> struct DecoderEntryAndCount {
  DecoderData<address_t> *entry;
  int count;
};

struct SegmentKey {
  uint64_t pc;
  uint32_t crc;
  uint64_t arena_size = 0;

  template <AddressType address_t> static SegmentKey from(const riscv::DecodedExecuteSegment<address_t> &segment, uint64_t arena_size) {
    SegmentKey key;
    key.pc = uint64_t(segment.exec_begin());
    key.crc = segment.crc32c_hash();
    key.arena_size = arena_size;
    return key;
  }

  bool operator==(const SegmentKey &other) const;
  bool operator<(const SegmentKey &other) const;
};

template <AddressType address_t> struct SharedExecuteSegments {
  SharedExecuteSegments() = default;
  SharedExecuteSegments(const SharedExecuteSegments &) = delete;
  SharedExecuteSegments &operator=(const SharedExecuteSegments &) = delete;
  using key_t = SegmentKey;

  struct Segment {
    std::shared_ptr<DecodedExecuteSegment<address_t>> segment;
    std::mutex mutex;

    std::shared_ptr<DecodedExecuteSegment<address_t>> get() {
      std::lock_guard<std::mutex> lock(mutex);
      return segment;
    }

    void unlocked_set(std::shared_ptr<DecodedExecuteSegment<address_t>> segment) { this->segment = std::move(segment); }
  };

  // Remove a segment if it is the last reference
  void remove_if_unique(key_t key) {
    std::lock_guard<std::mutex> lock(mutex);
    // We are not able to remove the Segment itself, as the mutex
    // may be locked by another thread. We can, however, lock the
    // Segments mutex and set the segment to nullptr.
    auto it = m_segments.find(key);
    if (it != m_segments.end()) {
      std::scoped_lock lock(it->second.mutex);
      if (it->second.segment.use_count() == 1) it->second.segment = nullptr;
    }
  }

  auto &get_segment(key_t key) {
    std::scoped_lock lock(mutex);
    auto &entry = m_segments[key];
    return entry;
  }

private:
  std::unordered_map<key_t, Segment> m_segments;
  std::mutex mutex;
};

} // namespace riscv

namespace std {
template <> struct hash<riscv::SegmentKey> {
  size_t operator()(const riscv::SegmentKey &key) const { return key.pc ^ key.crc ^ key.arena_size; }
};
} // namespace std
