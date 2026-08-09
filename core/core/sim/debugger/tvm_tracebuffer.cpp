#include "tvm_tracebuffer.hpp"
#include <algorithm>
#include <cassert>
#include <iterator>
#include <cstring>
#include "core/ds/hash/fnv.hpp"
#include "core/sim/debugger/tvm_encoding.hpp"

namespace tvm {

// Ensure that the concept is modeled as expected.
static_assert(std::input_iterator<TraceBuffer::Iterator>);
static_assert(std::bidirectional_iterator<TraceBuffer::Iterator>);

// --- Node ---

void TraceBuffer::Node::reset(pepp::bts::BufferManager &mgr) {
  // acknowledge() is the only caller, and it already ensures that all recordings are closed. This is just being
  // defensive.
  assert(open == 0 && "reset() of a slot with open recordings");
  if (code) code->clear();
  // Return pages to pool while keeping the map entries
  for (auto &[initiator, chain] : data)
    if (chain) chain->clear();
  // Return location buffer to pool rather than retaining it. After being reset, it's UB to access this node anyway,
  // and allowing the buffer to re-use this data should lower total occupancy of the ring.
  // Re-acquired on write_location().
  if (locations) {
    mgr.free_buffer(locations->id());
    locations = nullptr;
  }
  count = 0;
  // Cursors can no longer point to this node, and this node can be used by begin().
  slot = NO_SLOT;
}

// --- Construction / Destruction ---

TraceBuffer::TraceBuffer(std::shared_ptr<pepp::bts::BufferManager> mgr, size_t ring_size) : _mgr(std::move(mgr)) {
  // Every slot lookup is `absolute_slot % _ring.size()`, so an empty ring is a division by zero on first use rather
  // than a buffer that simply holds nothing. Refuse it here, where the cause is still visible.
  if (ring_size == 0) throw std::invalid_argument("TraceBuffer: ring_size must be at least 1");
  _ring.resize(ring_size);
  for (auto &node : _ring) {
    node.code = _mgr->alloc_chain();
    // Lazily allocate node.data to avoid paying for all devices up front.
    // Also lazily allocate node.locations, which is a full 64KiB buffer that is only needed if we enable tracing.
  }
  _templates = _mgr->alloc_chain();
  // Create a tombstone entry in the template chain so reserved-but-unwritten location entries can point to a valid
  // program. Not counted in _footprint.templates. It's only two bytes, and they are a functional requirement of the
  // reservation system.
  const auto halt = EncodedOp::Halt<0>{}.encode();
  _tombstone.code = _templates->append({halt.data(), halt.size()});
}

TraceBuffer::~TraceBuffer() noexcept {
  for (auto &node : _ring) {
    if (node.locations) _mgr->free_buffer(node.locations->id());
  }
}

// --- Recording lifecycle ---

TraceBuffer::Recording *TraceBuffer::find_recording(Device::ID initiator) {
  auto it = _recordings.find(initiator);
  // Closed recordings are not findable. Entries outlive their commit() so the scratch vectors keep their capacity,
  // which means a bare map hit says "this initiator has recorded before", not "this initiator is recording now" --
  // and every caller wants the second. Returning the stale one let a write that arrived outside any instruction
  // append its payload to a finished record: the bytes were charged to the footprint and reserved in the ring, but no
  // commit ever referenced them.
  if (it == _recordings.end() || !it->second.active) return nullptr;
  return &it->second;
}

pepp::bts::BufferChain &TraceBuffer::data_chain(Recording &rec) {
  // Resolved on first use and then retained. The recording pinned its slot at begin(), so we do not need to re-resolve.
  if (rec.chain == nullptr) {
    auto &slot = node_at(rec.slot).data[rec.id];
    if (!slot) slot = _mgr->alloc_chain();
    // The chain is owned by a unique_ptr, so rehashing the map moves the unique_pointer, not the chain.
    rec.chain = slot.get();
  }
  return *rec.chain;
}

void TraceBuffer::begin(Device::ID initiator) {
  // Advance if the current slot's location buffer is full. Guarded on residency because a one-slot ring
  // current_node() is the same node before and after an advance; without it a full slot would advance twice.
  if (auto &head = current_node(); head.slot == _head && head.count >= MAX_LOCATION_ENTRIES) advance_slot();

  // Re-read the node: a watermark callback fired by advance_slot() may have acknowledged this very slot.
  auto &node = current_node();
  if (node.slot != NO_SLOT && node.slot != _head) throw RingOverflow(_head);

  // Allocate the location buffer now, so that neither commit() nor abort() will allocate it.
  if (node.locations == nullptr) node.locations = _mgr->alloc_buffer();

  // clear() keeps the capacity earned by previous programs.
  auto &rec = _recordings[initiator];
  assert(!rec.active && "begin() called while this initiator is already recording");
  rec.id = initiator;
  rec.prefix.clear();
  rec.body.clear();
  rec.postfix.clear();
  rec.active = true;
  rec.data_start = {};
  // Claim the slot and index after everything that could throw. The data chain is resolved lazily on first write.
  node.slot = _head;
  rec.slot = _head;
  rec.entry = node.count++;
  rec.chain = nullptr;
  node.open++;
  rec.dp = {};

  // Write a tombstone so that a reserved-but-uncommitted entry dereferences to a program that immediately halts,
  // rather than stale data from a previous occupant. commit() overwrites this; abort() leaves it.
  write_location(node, rec.entry, _tombstone);
}

tvm::ProgramLocation TraceBuffer::commit(Device::ID initiator) {
  auto *rec = find_recording(initiator);
  assert(rec && "commit() called without a matching begin()");
  assert((rec == nullptr || rec->active) && "commit() called without a matching begin()");
  if (rec == nullptr || !rec->active) return {};

  auto halt = EncodedOp::Halt<0>{}.encode();
  rec->postfix.insert(rec->postfix.end(), halt.begin(), halt.end());

  // Release the reservation before anything that can throw. If resolve_body or flush_to_ring fails, the entry keeps
  // its tombstone and replays as a halt.
  auto &node = node_at(rec->slot);
  assert(node.open > 0 && "commit() without a matching begin() reservation");
  node.open--;
  rec->active = false;

  auto resolution = resolve_body({rec->body.data(), rec->body.size()});
  auto ret = flush_to_ring(*rec, resolution);
  _footprint.programs++;

  // Advance once the slot is full and no recordings are open. Must be after flush_to_ring: advance_slot() runs
  // watermark callbacks, and a callback that acknowledges would reset this node while we're still writing to it.
  if (rec->slot == _head && node.count >= MAX_LOCATION_ENTRIES && node.open == 0) advance_slot();
  return ret;
}

void TraceBuffer::emit_prefix(Recording &rec, bits::span<const u8> encoded) {
  rec.prefix.insert(rec.prefix.end(), encoded.begin(), encoded.end());
}

void TraceBuffer::abort(Device::ID initiator) {
  auto *rec = find_recording(initiator);
  if (rec == nullptr || !rec->active) return;
  // Release the reservation, but leave the entry itself alone. begin() wrote a tombstone there so that the index of an
  // aborted recording is a no-op rather than requiring special iteration behavior. It is allocation-free because we
  // want to avoid throwing. This executes inside Recorder::Instruction's destructor, where a throw would
  // std::terminate. We still throw because of asserts, but if you hit this assert, fix your buggy program.
  auto &node = node_at(rec->slot);
  assert(node.open > 0 && "abort() without a matching begin() reservation");
  node.open--;
  // Whatever payload this record wrote stays in the data chain, unreferenced, until the slot is reclaimed. Reclaiming
  // it would need a chain rewind, which does not exist. Keep the same clear-but-keep-capacity treatment as begin(), so
  // that aborting costs does not incur additional memory allocations.
  rec->prefix.clear();
  rec->body.clear();
  rec->postfix.clear();
  rec->dp = {};
  rec->data_start = {};
  rec->chain = nullptr;
  rec->active = false;
}

void TraceBuffer::emit_body(Recording &rec, bits::span<const u8> encoded) {
  rec.body.insert(rec.body.end(), encoded.begin(), encoded.end());
}

void TraceBuffer::emit_postfix(Recording &rec, bits::span<const u8> encoded) {
  rec.postfix.insert(rec.postfix.end(), encoded.begin(), encoded.end());
}

TraceBuffer::DpAnchor TraceBuffer::dp_anchor(Recording &rec) const { return rec.dp; }

void TraceBuffer::set_dp_anchor(Recording &rec, pepp::bts::Buffer::Location at, u16 size, u16 stride) {
  rec.dp = DpAnchor{true, at, size, stride};
}

void TraceBuffer::emit_prefix(Device::ID initiator, bits::span<const u8> encoded) {
  auto rec = find_recording(initiator);
  assert(rec && "emit_prefix() outside a begin()/commit() pair");
  emit_prefix(*rec, encoded);
}

void TraceBuffer::emit_body(Device::ID initiator, bits::span<const u8> encoded) {
  auto rec = find_recording(initiator);
  assert(rec && "emit_body() outside a begin()/commit() pair");
  emit_body(*rec, encoded);
}

void TraceBuffer::emit_postfix(Device::ID initiator, bits::span<const u8> encoded) {
  auto rec = find_recording(initiator);
  assert(rec && "emit_postfix() outside a begin()/commit() pair");
  emit_postfix(*rec, encoded);
}

pepp::bts::Buffer::Location TraceBuffer::append_data(Device::ID initiator, bits::span<const u8> data) {
  auto *rec = find_recording(initiator);
  assert(rec && rec->active && "append_data() outside a begin()/commit() pair");
  if (rec == nullptr || !rec->active) return {};
  auto loc = data_chain(*rec).append(data);
  _footprint.data += data.size();
  if (rec->data_start.id == pepp::bts::Buffer::ID{0}) rec->data_start = loc;
  return loc;
}

TraceBuffer::DataSlot TraceBuffer::append_data_uninitialized(Recording &rec, std::size_t len) {
  const auto res = data_chain(rec).reserve(len);
  _footprint.data += len;
  // The first payload of a record is where the driver points DP before entering the program, so it is remembered
  // separately from the anchor, which keeps moving as further payloads are appended.
  if (rec.data_start.id == pepp::bts::Buffer::ID{0}) rec.data_start = res.loc;
  return DataSlot{res.loc, res.bytes};
}

TraceBuffer::DataSlot TraceBuffer::append_data_uninitialized(Device::ID initiator, std::size_t len) {
  auto rec = find_recording(initiator);
  assert(rec && "append_data_uninitialized() outside a begin()/commit() pair");
  return append_data_uninitialized(*rec, len);
}

bool TraceBuffer::is_recording(Device::ID initiator) const {
  auto it = _recordings.find(initiator);
  return it != _recordings.end() && it->second.active;
}

// --- Backpressure ---

void TraceBuffer::on_watermark(float threshold, WatermarkCallback cb) {
  _watermarks.push_back({threshold, std::move(cb), false});
}

void TraceBuffer::acknowledge(Cursor up_to) {
  // Clamp to the head rather than trusting the caller. _tail running past _head would make ring_occupancy() evaluate
  // (_head - _tail) on unsigned values, underflow, and report an occupancy in the billions -- after which watermarks
  // fire arbitrarily and never reset. Worse, the loop below would reset() the node _head is still recording into,
  // returning its chains to the pool while an open Recording holds a pointer into them. A cursor held from before the
  // ring moved, or one taken from a different buffer, is an easy way to arrive here.
  const size_t limit = std::min(up_to.slot, _head);
  while (_tail < limit) {
    auto &node = _ring[_tail % _ring.size()];
    // A recording that reserved an entry here has not closed yet, and is still appending to this node's chains.
    // reset() would hand those buffers back underneath it. Stop rather than skip: _tail has to stay contiguous, and
    // the caller can acknowledge the rest once the recording closes.
    if (node.open > 0) break;
    node.reset(*_mgr);
    _tail++;
  }
  // Reset watermarks that are now below the current occupancy.
  float occ = ring_occupancy();
  for (auto &wm : _watermarks) {
    if (occ < wm.threshold)
      wm.fired = false;
  }
}

float TraceBuffer::ring_occupancy() const {
  if (_ring.empty()) return 0.0f;
  return static_cast<float>(_head - _tail) / static_cast<float>(_ring.size());
}

// --- Footprint accounting ---

TraceBuffer::Footprint TraceBuffer::footprint() const { return _footprint; }

void TraceBuffer::reset_footprint() { _footprint = {}; }

std::size_t TraceBuffer::buffer_footprint() const {
  // Whole buffers, not bytes written -- this is the number that answers "how much memory is this actually holding",
  // which Footprint deliberately does not, since a half-empty buffer costs the same as a full one.
  std::size_t buffers = 0;
  for (auto &node : _ring) {
    if (node.locations) ++buffers;
    if (node.code) buffers += node.code->buffer_count();
    for (auto &[initiator, chain] : node.data)
      if (chain) buffers += chain->buffer_count();
  }
  if (_templates) buffers += _templates->buffer_count();
  return buffers * pepp::bts::Buffer::SIZE;
}

// --- Data chain navigation ---

// Both of these scan every slot's chains because replay has no notion of an initiator -- the machine holds a DP, not
// a Device::ID -- so there is nothing to narrow the search with. A buffer belongs to exactly one chain at a time, so
// the first hit is the right answer; the scan is bounded by (ring slots x initiators that recorded), which is a
// handful. Note this remains unscoped across slots, so a recycled Buffer::ID can still match a chain in a slot other
// than the one being replayed.
pepp::bts::Buffer::ID TraceBuffer::data_successor(pepp::bts::Buffer::ID id) const {
  for (auto &node : _ring) {
    for (auto &[initiator, chain] : node.data) {
      if (!chain) continue;
      auto succ = chain->successor(id);
      if (succ != pepp::bts::Buffer::ID{0}) return succ;
    }
  }
  return pepp::bts::Buffer::ID{0};
}

pepp::bts::Buffer::ID TraceBuffer::data_predecessor(pepp::bts::Buffer::ID id) const {
  for (auto &node : _ring) {
    for (auto &[initiator, chain] : node.data) {
      if (!chain) continue;
      auto pred = chain->predecessor(id);
      if (pred != pepp::bts::Buffer::ID{0}) return pred;
    }
  }
  return pepp::bts::Buffer::ID{0};
}

// --- Inspection ---

u32 TraceBuffer::hash(bits::span<const u8> data) { return static_cast<u32>(pepp::fnv_1a(data)); }

u32 TraceBuffer::template_hits(u32 h) const {
  auto it = _template_map.find(h);
  return it != _template_map.end() ? it->second.hit_count : 0;
}

u16 TraceBuffer::template_size(u32 h) const {
  auto it = _template_map.find(h);
  return it != _template_map.end() ? static_cast<u16>(it->second.body.size()) : 0;
}

// --- Template dedup ---

bool TraceBuffer::template_matches(const TemplateEntry &entry, bits::span<const u8> body) {
  return std::ranges::equal(entry.body, body);
}

TraceBuffer::BodyResolution TraceBuffer::resolve_body(bits::span<const u8> body) {
  if (body.empty())
    return {false, {}};

  u32 hash = static_cast<u32>(pepp::fnv_1a(body));

  // Already promoted? A hash match is not proof of a body match -- the hash is a truncated 32-bit FNV, so two distinct
  // bodies can collide. Substituting a CALL on a collision would replay someone else's memory writes in place of this
  // program's, which is silent and unrecoverable, so confirm the bytes before trusting the entry.
  if (auto it = _template_map.find(hash); it != _template_map.end() && template_matches(it->second, body)) {
    it->second.hit_count++;
    return {true, it->second.location};
  } else if (it != _template_map.end()) {
    // Collision: this body is not the promoted one. Inline it rather than calling the wrong template. It can never be
    // templatized itself, since the hash slot is taken, but correctness beats footprint here.
    return {false, {}};
  }

  // Seen once before?
  if (_pending_hashes.contains(hash)) {
    if (body.size() >= PROMOTION_THRESHOLD) {
      // Promote: copy body to template chain.
      // Must append RET to ensure that the caller has an opportunity to run its own postifx.
      // The RET is reached by falling out of the body, so the two must land in the same buffer. A chain append that
      // does not fit rolls over to a fresh buffer, which would strand the RET and leave the template running off the
      // end -- so reserve both up front, exactly as flush_to_ring does for a subroutine.
      auto ret = EncodedOp::Ret<0>{}.encode();
      _templates->ensure_capacity(body.size() + ret.size());
      // reserve() rather than append() so the copy hands back a pointer to where the body landed. Resolving that
      // afterwards would mean walking the chain, and doing it here -- once per promotion -- keeps it off the hit
      // path entirely.
      const auto res = _templates->reserve(body.size());
      bits::memcpy(res.bytes, body);
      _templates->append({ret.data(), ret.size()});
      // The fixed cost of promotion is the unbounded lifetime of the template chain, which is amortized over  re-uses.
      _footprint.templates += body.size() + ret.size();

      TemplateEntry entry{};
      entry.location = res.loc;
      entry.body = res.bytes;
      entry.hit_count = 2;
      _template_map[hash] = entry;
      _pending_hashes.erase(hash);
      return {true, res.loc};
    }
    // Below threshold — inline every time, don't re-add to pending.
    return {false, {}};
  }

  // Maybe first occurrence? Cap the size of pending to some reasonable number so that it doesn't grow unboundedly.
  if (_pending_hashes.size() >= MAX_PENDING_HASHES) _pending_hashes.clear();
  _pending_hashes.insert(hash);
  return {false, {}};
}

tvm::ProgramLocation TraceBuffer::flush_to_ring(Recording &rec, BodyResolution resolution) {
  auto &node = node_at(rec.slot);

  // The subroutine is: [prefix][body or CALL][postfix]
  // There are no separators or terminators between these sections.
  // Postfix contains caller-injected instructions (if any) followed by HALT.
  // Location buffer programs must be complete (e.g., terminate), so the caller must ensure postfix terminates with a
  // HALT.
  // All parts of a subroutine must land in the same buffer — we must not split code across a buffer boundary.

  // Pre-encode CALL so we can measure its size before committing.
  auto call_enc = EncodedOp::Call<2>{
      .next_ip = SegmentPair{.hi = resolution.location.id.value, .lo = resolution.location.offset}}
                      .encode();

  // Compute total size so we can ensure all parts land in one buffer.
  size_t total = rec.prefix.size() + rec.postfix.size();
  if (resolution.is_template) total += call_enc.size();
  else total += rec.body.size();

  node.code->ensure_capacity(total);

  pepp::bts::Buffer::Location subroutine_start{};
  bool have_start = false;

  auto append = [&](bits::span<const u8> bytes) {
    auto loc = node.code->append(bytes);
    if (!have_start) {
      subroutine_start = loc;
      have_start = true;
    }
  };

  if (!rec.prefix.empty()) append({rec.prefix.data(), rec.prefix.size()});

  if (resolution.is_template) {
    append({call_enc.data(), call_enc.size()});
  } else if (!rec.body.empty()) append({rec.body.data(), rec.body.size()});

  append({rec.postfix.data(), rec.postfix.size()});

  // The number of bytes actuallly written to the code chain vs the bytes.
  _footprint.code += total;
  // What if this body was inlined instead of promoted? Provides a metric for how much promotion is saving us rather
  // than making promotion an optional feature.
  _footprint.code_if_inlined += rec.prefix.size() + rec.body.size() + rec.postfix.size();

  // Record where this program starts and where its data starts.
  const tvm::ProgramLocation program{subroutine_start, rec.data_start};
  // Overwrites the tombstone begin() put at this index.
  write_location(node, rec.entry, program);
  return program;
}

// --- Ring management ---

void TraceBuffer::advance_slot() {
  _head++;

  // Evaulate watermark callbacks
  float occ = ring_occupancy();
  for (auto &wm : _watermarks)
    if (!wm.fired && occ >= wm.threshold) wm.fired = true, wm.callback();

  // Overflow checking is handled by begin(ID).
}

// --- Location buffer I/O ---

void TraceBuffer::write_location(Node &node, u16 entry, tvm::ProgramLocation program) {
  static_assert(sizeof(tvm::ProgramLocation) == 8, "ProgramLocation must be 8 bytes for location packing");
  // begin() advances the slot before handing out an index that would overflow, so every reserved entry fits.
  assert(entry < MAX_LOCATION_ENTRIES && "location buffer overflow: entry written without a matching begin() reservation");
  // begin() allocates the location buffer when it reserves an index, so the buffer is always present here.
  assert(node.locations != nullptr && "write_location() into a slot with no location buffer");
  u16 offset = entry * sizeof(tvm::ProgramLocation);
  auto *dst = node.locations->data() + offset;
  std::memcpy(dst, &program, sizeof(program));
  // Interleaved recordings may commit out of order, so an entry at index 5 can be written before index 3. Extend
  // the buffer's used_capacity to cover this entry if it's the high-water mark so far.
  size_t required = offset + sizeof(tvm::ProgramLocation);
  if (node.locations->used_capacity() < required)
    node.locations->allocate_uninitialized(required - node.locations->used_capacity());
}

tvm::ProgramLocation TraceBuffer::read_location(const Node &node, u16 entry) const {
  // A slot that was never written, or one acknowledge() has reclaimed, holds no buffer. Reading from it yields the
  // null location rather than dereferencing nothing.
  if (node.locations == nullptr) return {};
  u16 offset = entry * sizeof(tvm::ProgramLocation);
  tvm::ProgramLocation program{};
  auto *src = node.locations->data() + offset;
  std::memcpy(&program, src, sizeof(program));
  return program;
}

// --- Cursor / Iteration ---

Cursor TraceBuffer::cursor() const { return {_head, current_node().count}; }

Cursor TraceBuffer::committed_cursor() const {
  // Walk back to the earliest reservation still open. _recordings holds one entry per initiator that has ever
  // recorded — one or two in practice — so a scan is cheaper than maintaining a running minimum that would have to
  // be recomputed whenever the holder of the minimum closed.
  Cursor stable{_head, current_node().count};
  for (const auto &[id, rec] : _recordings) {
    if (!rec.active) continue;
    if (const Cursor at{rec.slot, rec.entry}; at < stable) stable = at;
  }
  return stable;
}

TraceBuffer::CursorRange TraceBuffer::range(Cursor from, Cursor to) const {
  CursorRange r;
  r._begin = Iterator(this, from);
  r._end = Iterator(this, to);
  return r;
}

// --- Iterator ---

TraceBuffer::Iterator::Iterator(const TraceBuffer *tb, Cursor cursor) : _tb(tb), _cursor(cursor) {}

TraceBuffer::Iterator::reference TraceBuffer::Iterator::operator*() const {
  // If not resident, returns nullptr rather than returning a pointer to an overwritten location.
  const Node *node = _tb->resident_node(_cursor.slot);
  if (node == nullptr) return {};
  return _tb->read_location(*node, _cursor.entry);
}

TraceBuffer::Iterator &TraceBuffer::Iterator::operator++() {
  _cursor.entry++;
  // A slot that is no longer resident reports no entries, so iteration goes to the next one.
  const u16 count = _tb->count_at(_cursor.slot);
  // Advance to the next slot only if we've exhausted this one AND we're
  // behind _head. At the head slot, entry == count is the past-the-end
  // sentinel that cursor() returns — don't normalize past it.
  if (_cursor.entry >= count && _cursor.slot < _tb->_head) {
    _cursor.slot++;
    _cursor.entry = 0;
  }
  return *this;
}

TraceBuffer::Iterator TraceBuffer::Iterator::operator++(int) {
  auto tmp = *this;
  ++(*this);
  return tmp;
}

TraceBuffer::Iterator &TraceBuffer::Iterator::operator--() {
  if (_cursor.entry > 0) {
    _cursor.entry--;
    return *this;
  }
  // Step back into the previous slot's last entry.
  // Both gaurds ensure that you don't hit UB in release, with assert for debug where I ought to fix my code.
  assert(_cursor.slot > 0 && "decrement past beginning");
  if (_cursor.slot == 0) return *this;
  const u16 count = _tb->count_at(_cursor.slot - 1);
  assert(count > 0 && "decrement into a slot that is empty or no longer resident");
  if (count == 0) return *this; // Don't over-decrement, else loop might run infinitely.
  _cursor.slot--;
  _cursor.entry = count - 1;
  return *this;
}

TraceBuffer::Iterator TraceBuffer::Iterator::operator--(int) {
  auto tmp = *this;
  --(*this);
  return tmp;
}

bool TraceBuffer::Iterator::operator==(const Iterator &other) const { return _cursor == other._cursor; }

} // namespace tvm
