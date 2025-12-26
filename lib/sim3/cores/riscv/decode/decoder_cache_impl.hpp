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
#include "decoded_exec_segment.hpp"
#include "decoder_cache.hpp"
#include "sim3/subsystems/ram/paged_pool.hpp"
#include "sim3/systems/notraced_riscv_isa3_system.hpp"

#include <cstdint>
#include <mutex>
namespace riscv {
// decoder_cache.cpp

template <AddressType address_t> static SharedExecuteSegments<address_t> shared_execute_segments;

template <AddressType address_t> static bool is_regular_compressed(uint16_t instr) {
  const rv32c_instruction ci{instr};
#define CI_CODE(x, y) ((x << 13) | (y))
  switch (ci.opcode()) {
  case CI_CODE(0b001, 0b01):
    if constexpr (sizeof(address_t) == 8) return true; // C.ADDIW
    return false;                      // C.JAL 32-bit
  case CI_CODE(0b101, 0b01):           // C.JMP
  case CI_CODE(0b110, 0b01):           // C.BEQZ
  case CI_CODE(0b111, 0b01):           // C.BNEZ
    return false;
  case CI_CODE(0b100, 0b10): { // VARIOUS
    const bool topbit = ci.whole & (1 << 12);
    if (!topbit && ci.CR.rd != 0 && ci.CR.rs2 == 0) {
      return false; // C.JR rd
    } else if (topbit && ci.CR.rd != 0 && ci.CR.rs2 == 0) {
      return false; // C.JALR ra, rd+0
    } // TODO: Handle C.EBREAK
    return true;
  }
  default: return true;
  }
}

template <AddressType address_t>
static inline void fill_entries(const std::array<DecoderEntryAndCount<address_t>, 256> &block_array,
                                size_t block_array_count, address_t block_pc, address_t current_pc) {
  const unsigned last_count = block_array[block_array_count - 1].count;
  unsigned count = (current_pc - block_pc) >> 1;
  count -= last_count;
  if (count > 255) throw MachineException(INVALID_PROGRAM, "Too many non-branching instructions in a row");

  for (size_t i = 0; i < block_array_count; i++) {
    const DecoderEntryAndCount<address_t> &tuple = block_array[i];
    DecoderData<address_t> *entry = tuple.entry;
    const int length = tuple.count;

    // Ends at instruction *before* last PC
    entry->idxend = count;
    entry->icount = block_array_count - i;

    if constexpr (VERBOSE_DECODER) {
      fprintf(stderr, "Block 0x%lX has %u instructions\n", block_pc, count);
    }

    count -= length;
  }
}

template <AddressType address_t>
static void realize_fastsim(address_t base_pc, address_t last_pc, const uint8_t *exec_segment,
                            DecoderData<address_t> *exec_decoder) {
  if constexpr (compressed_enabled) {
    if (UNLIKELY(base_pc >= last_pc)) throw MachineException(INVALID_PROGRAM, "The execute segment has an overflow");
    if (UNLIKELY(base_pc & 0x1)) throw MachineException(INVALID_PROGRAM, "The execute segment is misaligned");

    // Go through entire executable segment and measure lengths
    // Record entries while looking for jumping instruction, then
    // fill out data and opcode lengths previous instructions.
    std::array<DecoderEntryAndCount<address_t>, 256> block_array;
    address_t pc = base_pc;
    while (pc < last_pc) {
      size_t block_array_count = 0;
      const address_t block_pc = pc;
      DecoderData<address_t> *entry = &exec_decoder[pc / DecoderCache<address_t>::DIVISOR];
      const AlignedLoad16 *iptr = (AlignedLoad16 *)&exec_segment[pc];
      const AlignedLoad16 *iptr_begin = iptr;
      while (true) {
        const unsigned length = iptr->length();
        const int count = length >> 1;

        // Record the instruction
        block_array[block_array_count++] = {entry, count};

        // Make sure PC does not overflow
#ifdef _MSC_VER
        if (pc + length < pc) throw MachineException(INVALID_PROGRAM, "PC overflow during execute segment decoding");
#else
        [[maybe_unused]] address_t pc2;
        if (UNLIKELY(__builtin_add_overflow(pc, length, &pc2)))
          throw MachineException(INVALID_PROGRAM, "PC overflow during execute segment decoding");
#endif
        pc += length;

        // If ending up crossing last_pc, it's an invalid block although
        // it could just be garbage, so let's force-end with an invalid instruction.
        if (UNLIKELY(pc > last_pc)) {
          entry->m_bytecode = 0; // Invalid instruction
          entry->m_handler = 0;
          break;
        }

        // All opcodes that can modify PC
        if (length == 2) {
          if (!is_regular_compressed<address_t>(iptr->half())) break;
        } else {
          const unsigned opcode = iptr->opcode();
          if (opcode == RV32I_BRANCH || opcode == RV32I_SYSTEM || opcode == RV32I_JAL || opcode == RV32I_JALR) break;
        }

        // A last test for the last instruction, which should have been a block-ending
        // instruction. Since it wasn't we must force-end the block here.
        if (UNLIKELY(pc >= last_pc)) {
          entry->m_bytecode = 0; // Invalid instruction
          entry->m_handler = 0;
          break;
        }

        iptr += count;

        // Too large blocks are likely malicious (although could be many empty pages)
        if (UNLIKELY(iptr - iptr_begin >= 255)) {
          // NOTE: Reinsert original instruction, as long sequences will lead to
          // PC becoming desynched, as it doesn't get increased.
          // We use a new block-ending fallback function handler instead.
          rv32i_instruction instruction = read_instruction(exec_segment, pc - length, last_pc);
          entry->set_bytecode(RV32I_BC_FUNCBLOCK);
          entry->set_invalid_handler(); // Resolve lazily
          entry->instr = instruction.whole;
          break;
        }

        entry += count;
      }
      if constexpr (VERBOSE_DECODER) {
        fprintf(stderr, "Block 0x%lX to 0x%lX\n", block_pc, pc);
      }

      if (UNLIKELY(block_array_count == 0))
        throw MachineException(INVALID_PROGRAM, "Encountered empty block after measuring");

      fill_entries(block_array, block_array_count, block_pc, pc);
    }
  } else { // !compressed_enabled
    // Count distance to next branching instruction backwards
    // and fill in idxend for all entries along the way.
    // This is for uncompressed instructions, which are always
    // 32-bits in size. We can use the idxend value for
    // instruction counting.
    unsigned idxend = 0;
    address_t pc = last_pc - 4;
    // NOTE: The last check avoids overflow
    while (pc >= base_pc && pc < last_pc) {
      const rv32i_instruction instruction = read_instruction(exec_segment, pc, last_pc);
      DecoderData<address_t> &entry = exec_decoder[pc / DecoderCache<address_t>::DIVISOR];
      const unsigned opcode = instruction.opcode();

      // All opcodes that can modify PC and stop the machine
      if (opcode == RV32I_BRANCH || opcode == RV32I_SYSTEM || opcode == RV32I_JAL || opcode == RV32I_JALR) idxend = 0;
      if (UNLIKELY(idxend == 65535)) {
        // It's a long sequence of instructions, so end block here.
        entry.set_bytecode(RV32I_BC_FUNCBLOCK);
        entry.set_invalid_handler(); // Resolve lazily
        entry.instr = instruction.whole;
        idxend = 0;
      }

      // Ends at *one instruction before* the block ends
      entry.idxend = idxend;
      // Increment after, idx becomes block count - 1
      idxend++;

      pc -= 4;
    }
  }
}

template <AddressType address_t> RISCV_INTERNAL size_t DecoderData<address_t>::handler_index_for(Handler new_handler) {
  std::scoped_lock lock(handler_idx_mutex);

  auto it = handler_cache.find(new_handler);
  if (it != handler_cache.end()) return it->second;

  if (UNLIKELY(handler_count >= instr_handlers.size()))
    throw MachineException(INVALID_PROGRAM, "Too many instruction handlers");
  instr_handlers[handler_count] = new_handler;
  const size_t idx = handler_count++;
  handler_cache.emplace(new_handler, idx);
  return idx;
}
} // namespace riscv
