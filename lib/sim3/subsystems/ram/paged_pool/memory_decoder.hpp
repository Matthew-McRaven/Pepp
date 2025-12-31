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
#include "sim3/common_macros.hpp"
#include "sim3/cores/riscv/decode/decoder_cache_impl.hpp"
#include "sim3/subsystems/ram/paged_pool.hpp"

namespace riscv {
template <AddressType address_t> void Memory<address_t>::evict_execute_segments() {
  // destructor could throw, so let's invalidate early
  machine().cpu.set_execute_segment(*CPU<address_t>::empty_execute_segment());

  auto &main_segment = m_main_exec_segment;
  if (main_segment) {
    const SegmentKey key = SegmentKey::from(*main_segment, memory_arena_size());
    main_segment = nullptr;
    shared_execute_segments<address_t>.remove_if_unique(key);
  }

  while (!m_exec.empty()) {
    try {
      auto &segment = m_exec.back();
      if (segment) {
        const SegmentKey key = SegmentKey::from(*segment, memory_arena_size());
        segment = nullptr;
        shared_execute_segments<address_t>.remove_if_unique(key);
      }
      m_exec.pop_back();
    } catch (...) {
      // Ignore exceptions
    }
  }
}

template <AddressType address_t>
void Memory<address_t>::evict_execute_segment(DecodedExecuteSegment<address_t> &segment) {
  const SegmentKey key = SegmentKey::from(segment, memory_arena_size());
  for (auto &seg : m_exec) {
    if (seg.get() == &segment) {
      seg = nullptr;
      if (&seg == &m_exec.back()) m_exec.pop_back();
      break;
    }
  }
  shared_execute_segments<address_t>.remove_if_unique(key);
}

// An execute segment contains a sequential array of raw instruction bits
// belonging to a set of sequential pages with +exec permission.
// It also contains a decoder cache that is produced from this instruction data.
// It is not strictly necessary to store the raw instruction bits, however, it
// enables step by step simulation as well as CLI- and remote debugging without
// rebuilding the emulator.
// Crucially, because of page alignments and 4 extra bytes, the necessary checks
// when reading from the execute segment is reduced. You can always read 4 bytes
// no matter where you are in the segment, a whole instruction unchecked.
template <AddressType address_t>
RISCV_INTERNAL DecodedExecuteSegment<address_t> &
Memory<address_t>::create_execute_segment(const MachineOptions<address_t> &options, const void *vdata, address_t vaddr,
                                          size_t exlen, bool is_initial, bool is_likely_jit) {
  if (UNLIKELY(exlen % (compressed_enabled ? 2 : 4)))
    throw MachineException(INVALID_PROGRAM, "Misaligned execute segment length");

  constexpr address_t PMASK = Page::size() - 1;
  const address_t pbase = vaddr & ~PMASK;
  const size_t prelen = vaddr - pbase;
  // Make 4 bytes of extra room to avoid having to validate 4-byte reads
  // when reading at 2 bytes before the end of the execute segment.
  const size_t midlen = exlen + prelen + 2; // Extra room for reads
  const size_t plen = (midlen + PMASK) & ~PMASK;
  // Because postlen uses midlen, we end up zeroing the extra 4 bytes in the end
  const size_t postlen = plen - midlen;
  // printf("Addr 0x%X Len %zx becomes 0x%X->0x%X PRE %zx MIDDLE %zu POST %zu TOTAL %zu\n",
  //	vaddr, exlen, pbase, pbase + plen, prelen, exlen, postlen, plen);
  if (UNLIKELY(prelen > plen || prelen + exlen > plen)) {
    throw MachineException(INVALID_PROGRAM, "Segment virtual base was bogus");
  }
#ifdef _MSC_VER
  if (UNLIKELY(pbase + plen < pbase)) throw MachineException(INVALID_PROGRAM, "Segment virtual base was bogus");
#else
  [[maybe_unused]] address_t pbase2;
  if (UNLIKELY(__builtin_add_overflow(pbase, plen, &pbase2)))
    throw MachineException(INVALID_PROGRAM, "Segment virtual base was bogus");
#endif
  // Create the whole executable memory range
  auto current_exec = std::make_shared<DecodedExecuteSegment<address_t>>(pbase, plen, vaddr, exlen);

  auto *exec_data = current_exec->exec_data(pbase);
  // This is a zeroed prologue in order to be able to use whole pages
  std::memset(&exec_data[0], 0, prelen);
  // This is the actual instruction bytes
  std::memcpy(&exec_data[prelen], vdata, exlen);
  // This memset() operation will end up zeroing the extra 4 bytes
  std::memset(&exec_data[prelen + exlen], 0, postlen);

  // Create CRC32-C hash of the execute segment
  const uint32_t hash = crc32c(exec_data, current_exec->exec_end() - current_exec->exec_begin());

  // Get a free slot to reference the execute segment
  auto &free_slot = this->next_execute_segment();

  if (options.use_shared_execute_segments) {
    // We have to key on the base address of the execute segment as well as the hash
    const SegmentKey key{uint64_t(current_exec->exec_begin()), hash, memory_arena_size()};

    // In order to prevent others from creating the same execute segment
    // we need to lock the shared execute segments mutex.
    auto &segment = shared_execute_segments<address_t>.get_segment(key);
    std::scoped_lock lock(segment.mutex);

    if (segment.segment != nullptr) {
      free_slot = segment.segment;
      return *free_slot;
    }

    // We need to create a new execute segment, as there is no shared
    // execute segment with the same hash.
    free_slot = std::move(current_exec);
    free_slot->set_likely_jit(is_likely_jit);
#if defined(RISCV_BINARY_TRANSLATION) && defined(RISCV_DEBUG)
    free_slot->set_record_slowpaths(options.record_slowpaths_to_jump_hints && !is_likely_jit);
#endif
    // Store the hash in the decoder cache
    free_slot->set_crc32c_hash(hash);

    this->generate_decoder_cache(options, free_slot, is_initial);

    // Share the execute segment
    shared_execute_segments<address_t>.get_segment(key).unlocked_set(free_slot);
  } else {
    free_slot = std::move(current_exec);
    free_slot->set_likely_jit(is_likely_jit);
    // Store the hash in the decoder cache
    free_slot->set_crc32c_hash(hash);

    this->generate_decoder_cache(options, free_slot, is_initial);
  }

  return *free_slot;
}

template <AddressType address_t>
std::shared_ptr<DecodedExecuteSegment<address_t>> &Memory<address_t>::next_execute_segment() {
  if (!m_main_exec_segment) {
    return m_main_exec_segment;
  }
  if (LIKELY(m_exec.size() < RISCV_MAX_EXECUTE_SEGS)) {
    m_exec.push_back(nullptr);
    return m_exec.back();
  }
  throw MachineException(INVALID_PROGRAM, "Max execute segments reached");
}

template <AddressType address_t>
const std::shared_ptr<DecodedExecuteSegment<address_t>> &Memory<address_t>::exec_segment_for(address_t vaddr) const {
  return const_cast<Memory<address_t> *>(this)->exec_segment_for(vaddr);
}

// The decoder cache is a sequential array of DecoderData<address_t> entries
// each of which (currently) serves a dual purpose of enabling
// threaded dispatch (m_bytecode) and fallback to callback function
// (m_handler). This enables high-speed emulation, precise simulation,
// CLI debugging and remote GDB debugging without rebuilding the emulator.
//
// The decoder cache covers all pages that the execute segment belongs
// in, so that all legal jumps (based on page +exec permission) will
// result in correct execution (including invalid instructions).
//
// The goal of the decoder cache is to allow uninterrupted execution
// with minimal bounds-checking, while also enabling accurate
// instruction counting.
template <AddressType address_t>
RISCV_INTERNAL void
Memory<address_t>::generate_decoder_cache([[maybe_unused]] const MachineOptions<address_t> &options,
                                          std::shared_ptr<DecodedExecuteSegment<address_t>> &shared_segment,
                                          [[maybe_unused]] bool is_initial) {
  TIME_POINT(t0);
  auto &exec = *shared_segment;
  if (exec.exec_end() < exec.exec_begin()) throw MachineException(INVALID_PROGRAM, "Execute segment was invalid");

  const auto pbase = exec.pagedata_base();
  const auto addr = exec.exec_begin();
  const auto len = exec.exec_end() - exec.exec_begin();
  constexpr size_t PMASK = Page::size() - 1;
  // We need to allocate room for at least one more decoder cache entry.
  // This is because jump and branch instructions don't check PC after
  // not branching. The last entry is an invalid instruction.
  const size_t prelen = addr - pbase;
  const size_t midlen = len + prelen + 4; // Extra entry
  const size_t plen = (midlen + PMASK) & ~PMASK;
  // printf("generate_decoder_cache: Addr 0x%X Len %zx becomes 0x%X->0x%X PRE %zx MIDDLE %zu TOTAL %zu\n",
  //	addr, len, pbase, pbase + plen, prelen, midlen, plen);

  const size_t n_pages = plen / Page::size();
  if (n_pages == 0) {
    throw MachineException(INVALID_PROGRAM, "Program produced empty decoder cache");
  }
  // Here we allocate the decoder cache which is page-sized
  auto *decoder_cache = exec.create_decoder_cache(new DecoderCache<address_t>[n_pages], n_pages);
  // Clear the decoder cache! (technically only needed when binary translation is enabled)
  std::memset(decoder_cache, 0, n_pages * sizeof(DecoderCache<address_t>));
  // Get a base address relative pointer to the decoder cache
  // Eg. exec_decoder[pbase] is the first entry in the decoder cache
  // so that PC with a simple shift can be used as a direct index.
  auto *exec_decoder = decoder_cache[0].get_base() - pbase / DecoderCache<address_t>::DIVISOR;
  exec.set_decoder(exec_decoder);

  DecoderData<address_t> invalid_op;
  invalid_op.set_handler(this->machine().cpu.decode({0}));
  if (UNLIKELY(invalid_op.m_handler != 0)) {
    throw MachineException(INVALID_PROGRAM, "The invalid instruction did not have the index zero",
                           invalid_op.m_handler);
  }

  // PC-relative pointer to instruction bits
  auto *exec_segment = exec.exec_data();
  TIME_POINT(t1);

  // When compressed instructions are enabled, many decoder
  // entries are illegal because they are between instructions.
  bool was_full_instruction = true;

  /* Generate all instruction pointers for executable code.
     Cannot step outside of this area when pregen is enabled,
     so it's fine to leave the boundries alone. */
  TIME_POINT(t2);
  address_t dst = addr;
  const address_t end_addr = addr + len;
  for (; dst < addr + len;) {
    auto &entry = exec_decoder[dst / DecoderCache<address_t>::DIVISOR];
    entry.m_handler = 0;
    entry.idxend = 0;

    // Load unaligned instruction from execute segment
    const auto instruction = read_instruction(exec_segment, dst, end_addr);
    rv32i_instruction rewritten = instruction;

    if (!compressed_enabled || was_full_instruction) {
      // Cache the (modified) instruction bits
      auto bytecode = CPU<address_t>::computed_index_for(instruction);
      // Threaded rewrites are **always** enabled
      bytecode = exec.threaded_rewrite(bytecode, dst, rewritten);
      entry.set_bytecode(bytecode);
      entry.instr = rewritten.whole;
    } else {
      // WARNING: If we don't ignore this instruction,
      // it will get *wrong* idxend values, and cause *invalid jumps*
      entry.m_handler = 0;
      entry.set_bytecode(0);
      // ^ Must be made invalid, even if technically possible to jump to!
    }
    if constexpr (VERBOSE_DECODER) {
      if (entry.get_bytecode() >= RV32I_BC_BEQ && entry.get_bytecode() <= RV32I_BC_BGEU) {
        fprintf(stderr, "Detected branch bytecode at 0x%lX\n", dst);
      }
      if (entry.get_bytecode() == RV32I_BC_BEQ_FW || entry.get_bytecode() == RV32I_BC_BNE_FW) {
        fprintf(stderr, "Detected forward branch bytecode at 0x%lX\n", dst);
      }
    }

    // Increment PC after everything
    if constexpr (compressed_enabled) {
      // With compressed we always step forward 2 bytes at a time
      dst += 2;
      if (was_full_instruction) {
        // For it to be a full instruction again,
        // the length needs to match.
        was_full_instruction = (instruction.length() == 2);
      } else {
        // If it wasn't a full instruction last time, it
        // will for sure be one now.
        was_full_instruction = true;
      }
    } else dst += 4;
  }
  // Make sure the last entry is an invalid instruction
  // This simplifies many other sub-systems
  auto &entry = exec_decoder[(addr + len) / DecoderCache<address_t>::DIVISOR];
  entry.set_bytecode(0);
  entry.m_handler = 0;
  entry.idxend = 0;
  TIME_POINT(t3);

  realize_fastsim<address_t>(addr, dst, exec_segment, exec_decoder);

  // Debugging: EBREAK locations
  for (auto &loc : options.ebreak_locations) {
    address_t addr = 0;
    if (std::holds_alternative<address_t>(loc)) addr = std::get<address_t>(loc);
    else addr = machine().address_of(std::get<std::string>(loc));

    if (addr != 0x0 && addr >= exec.exec_begin() && addr < exec.exec_end()) {
      CPU<address_t>::install_ebreak_for(exec, addr);
      if (options.verbose_loader) {
        printf("libriscv: Added ebreak location at 0x%" PRIx64 "\n", uint64_t(addr));
      }
    }
  }

  TIME_POINT(t4);
#ifdef ENABLE_TIMINGS
  const long t1t0 = nanodiff(t0, t1);
  const long t2t1 = nanodiff(t1, t2);
  const long t3t2 = nanodiff(t2, t3);
  const long t3t4 = nanodiff(t3, t4);
  printf("libriscv: Decoder cache allocation took %ld ns\n", t1t0);
  if constexpr (binary_translation_enabled) printf("libriscv: Decoder cache bintr activation took %ld ns\n", t2t1);
  printf("libriscv: Decoder cache generation took %ld ns\n", t3t2);
  printf("libriscv: Decoder cache realization took %ld ns\n", t3t4);
  printf("libriscv: Decoder cache totals: %ld us\n", nanodiff(t0, t4) / 1000);
#endif
}

} // namespace riscv
