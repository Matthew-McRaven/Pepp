#include "tvm_tracebuffer.hpp"
#include <cassert>
#include <cstring>
#include "core/ds/hash/fnv.hpp"
#include "core/sim/debugger/tvm_encoding.hpp"

namespace tvm {

// --- Node ---

void TraceBuffer::Node::reset(pepp::bts::BufferManager &mgr) {
  if (code) code->clear();
  if (data) data->clear();
  if (locations) locations->clear();
  count = 0;
  in_use = false;
}

// --- Construction / Destruction ---

TraceBuffer::TraceBuffer(std::shared_ptr<pepp::bts::BufferManager> mgr, size_t ring_size) : _mgr(std::move(mgr)) {
  _ring.resize(ring_size);
  for (auto &node : _ring) {
    node.locations = _mgr->alloc_buffer();
    node.code = _mgr->alloc_chain();
    node.data = _mgr->alloc_chain();
  }
  _templates = _mgr->alloc_chain();
}

TraceBuffer::~TraceBuffer() noexcept {
  for (auto &node : _ring) {
    if (node.locations) _mgr->free_buffer(node.locations->id());
  }
}

// --- Recording lifecycle ---

TraceBuffer::Recording *TraceBuffer::find_recording(Device::ID initiator) {
  auto it = _recordings.find(initiator);
  return it == _recordings.end() ? nullptr : &it->second;
}

void TraceBuffer::begin(Device::ID initiator) {
  // First recording from this initiator creates its scratch state; later ones reuse it, which is why the vectors are
  // cleared rather than reconstructed -- clear() keeps the capacity earned by previous programs.
  auto &rec = _recordings[initiator];
  assert(!rec.active && "begin() called while this initiator is already recording");
  rec.prefix.clear();
  rec.body.clear();
  rec.postfix.clear();
  rec.active = true;
}

pepp::bts::Buffer::Location TraceBuffer::commit(Device::ID initiator) {
  // Unlike begin(), this must not create an entry: a commit() for an initiator that never began is a caller bug, and
  // default-constructing one here would silently commit an empty program.
  auto *rec = find_recording(initiator);
  assert(rec && "commit() called without a matching begin()");
  assert(rec->active && "commit() called without a matching begin()");

  // Append HALT as the final instruction in the postfix.
  auto halt = EncodedOp::Halt<0>{}.encode();
  rec->postfix.insert(rec->postfix.end(), halt.begin(), halt.end());

  auto resolution = resolve_body({rec->body.data(), rec->body.size()});
  auto ret = flush_to_ring(*rec, resolution);

  auto &node = current_node();
  node.count++;
  node.in_use = true;
  rec->active = false;
  _total_instructions++;

  if (node.count >= MAX_LOCATION_ENTRIES) advance_slot();
  return ret;
}

void TraceBuffer::emit_prefix(Device::ID initiator, bits::span<const u8> encoded) {
  auto *rec = find_recording(initiator);
  assert(rec && rec->active && "emit_prefix() outside a begin()/commit() pair");
  rec->prefix.insert(rec->prefix.end(), encoded.begin(), encoded.end());
}

void TraceBuffer::emit_body(Device::ID initiator, bits::span<const u8> encoded) {
  auto *rec = find_recording(initiator);
  assert(rec && rec->active && "emit_body() outside a begin()/commit() pair");
  rec->body.insert(rec->body.end(), encoded.begin(), encoded.end());
}

void TraceBuffer::emit_postfix(Device::ID initiator, bits::span<const u8> encoded) {
  auto *rec = find_recording(initiator);
  assert(rec && rec->active && "emit_postfix() outside a begin()/commit() pair");
  rec->postfix.insert(rec->postfix.end(), encoded.begin(), encoded.end());
}

pepp::bts::Buffer::Location TraceBuffer::append_data(Device::ID initiator, bits::span<const u8> data) {
  auto *rec = find_recording(initiator);
  assert(rec && rec->active && "append_data() outside a begin()/commit() pair");
  auto loc = current_node().data->append(data);
  rec->last_dp = loc;
  return loc;
}

pepp::bts::Buffer::Location TraceBuffer::last_dp(Device::ID initiator) const {
  // An initiator that has never recorded has no DP yet, which is the same answer as one that has recorded but never
  // written data -- both get {0,0}, since Buffer::ID{0} is effectively a nullptr of the buffer manager.
  auto it = _recordings.find(initiator);
  return it == _recordings.end() ? pepp::bts::Buffer::Location{} : it->second.last_dp;
}

// --- Backpressure ---

void TraceBuffer::on_watermark(float threshold, WatermarkCallback cb) {
  _watermarks.push_back({threshold, std::move(cb), false});
}

void TraceBuffer::acknowledge(Cursor up_to) {
  while (_tail < up_to.slot) {
    auto &node = _ring[_tail % _ring.size()];
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

// --- Data chain navigation ---

pepp::bts::Buffer::ID TraceBuffer::data_successor(pepp::bts::Buffer::ID id) const {
  for (auto &node : _ring) {
    if (!node.data) continue;
    auto succ = node.data->successor(id);
    if (succ != pepp::bts::Buffer::ID{0}) return succ;
  }
  return pepp::bts::Buffer::ID{0};
}

pepp::bts::Buffer::ID TraceBuffer::data_predecessor(pepp::bts::Buffer::ID id) const {
  for (auto &node : _ring) {
    if (!node.data) continue;
    auto pred = node.data->predecessor(id);
    if (pred != pepp::bts::Buffer::ID{0}) return pred;
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
  return it != _template_map.end() ? it->second.size : 0;
}

// --- Template dedup ---

TraceBuffer::BodyResolution TraceBuffer::resolve_body(bits::span<const u8> body) {
  if (body.empty())
    return {false, {}};

  u32 hash = static_cast<u32>(pepp::fnv_1a(body));

  // Already promoted?
  if (auto it = _template_map.find(hash); it != _template_map.end()) {
    it->second.hit_count++;
    return {true, it->second.location};
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
      auto loc = _templates->append(body);
      _templates->append({ret.data(), ret.size()});

      TemplateEntry entry{};
      entry.location = loc;
      entry.size = static_cast<u16>(body.size());
      entry.hit_count = 2;
      _template_map[hash] = entry;
      _pending_hashes.erase(hash);
      return {true, loc};
    }
    // Below threshold — inline every time, don't re-add to pending.
    return {false, {}};
  }

  // First occurrence.
  _pending_hashes.insert(hash);
  return {false, {}};
}

pepp::bts::Buffer::Location TraceBuffer::flush_to_ring(Recording &rec, BodyResolution resolution) {
  auto &node = current_node();

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

  // Record this subroutine's entry point in the locations buffer.
  write_location(node, node.count, subroutine_start);
  return subroutine_start;
}

// --- Ring management ---

void TraceBuffer::advance_slot() {
  _head++;

  // Fire watermark callbacks. Run before the overflow check below so a callback that frees the slot we are about to
  // land on can prevent overflow.
  float occ = ring_occupancy();
  for (auto &wm : _watermarks) {
    if (!wm.fired && occ >= wm.threshold) {
      wm.fired = true;
      wm.callback();
    }
  }

  // Prepare the new current node if it's been previously consumed.
  auto &node = current_node();
  // Still in use means nobody consumed this slot's trace. Overwriting it would silently discard history, so refuse.
  // _head stays advanced: acknowledge() resets this node, after which submission picks up from a clean slot.
  if (node.in_use) throw RingOverflow(_head);
  node.count = 0;
  // code/data chains should already be clear from acknowledge().
}

// --- Location buffer I/O ---

void TraceBuffer::write_location(Node &node, u16 entry, pepp::bts::Buffer::Location loc) {
  static_assert(sizeof(pepp::bts::Buffer::Location) == 4, "Location must be 4 bytes for location packing");
  // Backstop for a caller that swallowed a RingOverflow and kept submitting: the offset below is a u16, so an entry
  // at or past the maximum would wrap to 0 and quietly overwrite the oldest entry instead of failing.
  if (entry >= MAX_LOCATION_ENTRIES) throw RingOverflow(_head);
  u16 offset = entry * sizeof(pepp::bts::Buffer::Location);
  auto *dst = node.locations->data() + offset;
  std::memcpy(dst, &loc, sizeof(loc));
  // If the slab hasn't tracked this write, bump its used capacity.
  // We write sequentially (entry == count before increment), so allocate_uninitialized
  // on first use of each entry position.
  size_t required = offset + sizeof(pepp::bts::Buffer::Location);
  if (node.locations->used_capacity() < required)
    node.locations->allocate_uninitialized(required - node.locations->used_capacity());
}

pepp::bts::Buffer::Location TraceBuffer::read_location(const Node &node, u16 entry) const {
  u16 offset = entry * sizeof(pepp::bts::Buffer::Location);
  pepp::bts::Buffer::Location loc{};
  auto *src = node.locations->data() + offset;
  std::memcpy(&loc, src, sizeof(loc));
  return loc;
}

// --- Cursor / Iteration ---

Cursor TraceBuffer::cursor() const { return {_head, current_node().count}; }

TraceBuffer::CursorRange TraceBuffer::range(Cursor from, Cursor to) const {
  CursorRange r;
  r._begin = Iterator(this, from);
  r._end = Iterator(this, to);
  return r;
}

// --- Iterator ---

TraceBuffer::Iterator::Iterator(const TraceBuffer *tb, Cursor cursor) : _tb(tb), _cursor(cursor) {}

TraceBuffer::Iterator::reference TraceBuffer::Iterator::operator*() const {
  auto &node = _tb->node_at(_cursor.slot);
  return _tb->read_location(node, _cursor.entry);
}

TraceBuffer::Iterator &TraceBuffer::Iterator::operator++() {
  _cursor.entry++;
  auto &node = _tb->node_at(_cursor.slot);
  // Advance to the next slot only if we've exhausted this one AND we're
  // behind _head. At the head slot, entry == count is the past-the-end
  // sentinel that cursor() returns — don't normalize past it.
  if (_cursor.entry >= node.count && _cursor.slot < _tb->_head) {
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
  } else {
    // Move to the previous slot's last entry.
    assert(_cursor.slot > 0 && "decrement past beginning");
    _cursor.slot--;
    auto &node = _tb->node_at(_cursor.slot);
    assert(node.count > 0);
    _cursor.entry = node.count - 1;
  }
  return *this;
}

TraceBuffer::Iterator TraceBuffer::Iterator::operator--(int) {
  auto tmp = *this;
  --(*this);
  return tmp;
}

bool TraceBuffer::Iterator::operator==(const Iterator &other) const { return _cursor == other._cursor; }

} // namespace tvm
