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
  in_use = false;
}

// --- Construction / Destruction ---

TraceBuffer::TraceBuffer(std::shared_ptr<pepp::bts::BufferManager> mgr, size_t ring_size) : _mgr(std::move(mgr)) {
  _ring.resize(ring_size);
  for (auto &node : _ring) {
    node.code = _mgr->alloc_chain();
    // Lazily allocate node.data to avoid paying for all devices up front.
    // Also lazily allocate node.locations, which is a full 64KiB buffer that is only needed if we enable tracing.
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

pepp::bts::BufferChain &TraceBuffer::data_chain(Recording &rec) {
  // Re-resolve whenever the head entry has changed. commit() can advance the slot while another initiator is still
  // recording, and a chain memoized against the old slot put payloads into the previous entry instead of the current
  // one, leading to use-after-frees.
  if (rec.chain == nullptr || rec.chain_slot != _head) {
    auto &slot = current_node().data[rec.id];
    if (!slot) slot = _mgr->alloc_chain();
    // The chain is owned by a unique_ptr, so rehashing the map moves the unique_pointer, not the chain.
    rec.chain = slot.get();
    rec.chain_slot = _head;
  }
  return *rec.chain;
}

void TraceBuffer::begin(Device::ID initiator) {
  // First recording from this initiator creates its scratch state; later ones reuse it, which is why the vectors are
  // cleared rather than reconstructed -- clear() keeps the capacity earned by previous programs.
  auto &rec = _recordings[initiator];
  assert(!rec.active && "begin() called while this initiator is already recording");
  rec.id = initiator;
  rec.prefix.clear();
  rec.body.clear();
  rec.postfix.clear();
  rec.active = true;
  rec.data_start = {};
  // A program may not assume DP survived the previous commit(), so the first emitter in this recording has to state
  // it absolutely.
  rec.dp = {};
}

tvm::ProgramLocation TraceBuffer::commit(Device::ID initiator) {
  // Unlike begin(), this must not create an entry: a commit() for an initiator that never began is a caller bug, and
  // default-constructing one here would silently commit an empty program.
  auto *rec = find_recording(initiator);
  assert(rec && "commit() called without a matching begin()");
  assert((rec == nullptr || rec->active) && "commit() called without a matching begin()");
  // Committing something that was never begun would flush a default-constructed recording, or dereference nothing at
  // all. Report the null location instead; the caller gets a program it cannot run rather than undefined behaviour.
  if (rec == nullptr || !rec->active) return {};

  // Append HALT as the final instruction in the postfix.
  auto halt = EncodedOp::Halt<0>{}.encode();
  rec->postfix.insert(rec->postfix.end(), halt.begin(), halt.end());

  auto resolution = resolve_body({rec->body.data(), rec->body.size()});
  auto ret = flush_to_ring(*rec, resolution);

  auto &node = current_node();
  node.count++;
  node.in_use = true;
  rec->active = false;
  _footprint.programs++;

  if (node.count >= MAX_LOCATION_ENTRIES) advance_slot();
  return ret;
}

void TraceBuffer::emit_prefix(Recording &rec, bits::span<const u8> encoded) {
  rec.prefix.insert(rec.prefix.end(), encoded.begin(), encoded.end());
}
void TraceBuffer::abort(Device::ID initiator) {
  auto *rec = find_recording(initiator);
  if (rec == nullptr || !rec->active) return;
  // Same clear-but-keep-capacity treatment begin() gives them, so aborting costs nothing the next recording has to
  // earn back.
  rec->prefix.clear();
  rec->body.clear();
  rec->postfix.clear();
  rec->dp = {};
  rec->data_start = {};
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
  rec->last_dp = loc;
  return loc;
}

TraceBuffer::DataSlot TraceBuffer::append_data_uninitialized(Recording &rec, std::size_t len) {
  const auto res = data_chain(rec).reserve(len);
  _footprint.data += len;
  // The first payload of a record is where the driver will point DP before entering the program, so it has to be
  // remembered separately from last_dp, which keeps moving.
  if (rec.data_start.id == pepp::bts::Buffer::ID{0}) rec.data_start = res.loc;
  rec.last_dp = res.loc;
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

  // The number of bytes actuallly written to the code chain vs the bytes.
  _footprint.code += total;
  // What if this body was inlined instead of promoted? Provides a metric for how much promotion is saving us rather
  // than making promotion an optional feature.
  _footprint.code_if_inlined += rec.prefix.size() + rec.body.size() + rec.postfix.size();

  // Record where this program starts and where its data starts.
  const tvm::ProgramLocation program{subroutine_start, rec.data_start};
  write_location(node, node.count, program);
  return program;
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

void TraceBuffer::write_location(Node &node, u16 entry, tvm::ProgramLocation program) {
  static_assert(sizeof(tvm::ProgramLocation) == 8, "ProgramLocation must be 8 bytes for location packing");
  // Backstop for a caller that swallowed a RingOverflow and kept submitting: the offset below is a u16, so an entry
  // at or past the maximum would wrap to 0 and quietly overwrite the oldest entry instead of failing.
  if (entry >= MAX_LOCATION_ENTRIES) throw LocationBufferFull(_head);
  // Acquired on first write into this slot rather than at construction or reset, so an unused ring slot holds no
  // buffer at all.
  if (node.locations == nullptr) node.locations = _mgr->alloc_buffer();
  u16 offset = entry * sizeof(tvm::ProgramLocation);
  auto *dst = node.locations->data() + offset;
  std::memcpy(dst, &program, sizeof(program));
  // If the slab hasn't tracked this write, bump its used capacity.
  // We write sequentially (entry == count before increment), so allocate_uninitialized
  // on first use of each entry position.
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
