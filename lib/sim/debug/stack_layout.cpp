#include "stack_layout.hpp"
#include <spdlog/spdlog.h>
#include "expr_ast.hpp"
#include "fmt/ranges.h"

namespace {
static constexpr int SLOT_VALUE_WIDTH = 6;
static constexpr int SLOT_NAME_WIDTH = 7;
static constexpr int SLOT_ADDRESS_WIDTH = 4;
static constexpr int SLOT_PADDING = 2;
static constexpr int SLOT_RENDER_WIDTH = SLOT_NAME_WIDTH + SLOT_VALUE_WIDTH + SLOT_ADDRESS_WIDTH + 2 * SLOT_PADDING;
// 1 space on either side, plus 1 = or - on either side
static constexpr int FRAME_RENDER_WIDTH = SLOT_RENDER_WIDTH + 4;
} // namespace

pepp::debug::Slot::Slot(quint32 address, quint32 size, QString name, std::shared_ptr<Term> expr, Frame *parent)
    : _address(address), _size(size), _name(std::move(name)), _expr(std::move(expr)), _parent(parent) {}

pepp::debug::Slot::Slot(Slot &&other)
    : _address(other._address), _size(other._size), _name(std::move(other._name)), _expr(std::move(other._expr)),
      _parent(other._parent) {
  other._address = 0;
  other._size = 0;
  other._parent = nullptr;
}

pepp::debug::Slot &pepp::debug::Slot::operator=(Slot &&other) {
  if (this != &other) {
    _address = other._address;
    _size = other._size;
    _name = std::move(other._name);
    _expr = std::move(other._expr);
    _parent = other._parent;

    other._address = 0;
    other._size = 0;
    other._parent = nullptr;
  }
  return *this;
}

quint32 pepp::debug::Slot::address() const { return _address; }

quint32 pepp::debug::Slot::size() const { return _size; }

QString pepp::debug::Slot::name() const { return _name; }

std::shared_ptr<pepp::debug::Term> pepp::debug::Slot::expr() { return _expr; }

std::shared_ptr<const pepp::debug::Term> pepp::debug::Slot::expr() const { return _expr; }

std::string pepp::debug::Slot::value() const {
  auto cached = _expr ? _expr->evaluator().cache() : pepp::debug::EvaluationCache{};
  std::string value_as_string = "";
  if (cached.value) {
    value_as_string = fmt::format("{:x}", pepp::debug::value_bits<quint16>(*cached.value));
  }
  return value_as_string;
}

bool pepp::debug::Slot::is_value_dirty() const { return !_expr || _expr->evaluator().dirty(); }

const pepp::debug::Frame *pepp::debug::Slot::parent() const { return _parent; }

pepp::debug::Frame *pepp::debug::Slot::parent() { return _parent; }

pepp::debug::Slot::operator std::string() const {
  // Updates to this format require updates to globals at the top of this file
  return fmt::format("{: <7} |{: ^6}| {:04x}", _name.toStdString(), value(), _address);
}

pepp::debug::Frame::Frame(quint32 baseAddress, Stack *parent) : _baseAddress(baseAddress), _parent(parent) {}

pepp::debug::Frame::Frame(Frame &&other)
    : _baseAddress(other._baseAddress), _parent(other._parent), _active(other._active),
      _slots(std::move(other._slots)) {
  other._baseAddress = 0;
  other._parent = nullptr;
  other._active = false;
}

pepp::debug::Frame &pepp::debug::Frame::operator=(Frame &&other) {
  if (this != &other) {
    _baseAddress = other._baseAddress;
    _parent = other._parent;
    _active = other._active;
    _slots = std::move(other._slots);

    other._baseAddress = 0;
    other._parent = nullptr;
    other._active = false;
  }
  return *this;
}

pepp::debug::Stack *pepp::debug::Frame::parent() { return _parent; }

bool pepp::debug::Frame::active() const { return _active; }

void pepp::debug::Frame::setActive(bool active) { _active = active; }

quint32 pepp::debug::Frame::base_address() const { return _baseAddress; }

quint32 pepp::debug::Frame::top_address() const {
  quint32 address = 0;
  for (const auto &it : std::as_const(_slots)) address = qMax(address, it.address() + it.size());
  return address;
}

void pepp::debug::Frame::pushSlot(Slot &&slot) { _slots.push_back(std::move(slot)); }

pepp::debug::Slot pepp::debug::Frame::popSlot() {
  if (_slots.empty()) throw std::runtime_error("No slots to pop");
  Slot slot = std::move(_slots.back());
  _slots.pop_back();
  return slot;
}

pepp::debug::Frame::const_iterator pepp::debug::Frame::cend() const { return _slots.cend(); }

pepp::debug::Frame::const_iterator pepp::debug::Frame::cbegin() const { return _slots.cbegin(); }

pepp::debug::Frame::const_iterator pepp::debug::Frame::end() const { return _slots.cend(); }

pepp::debug::Frame::const_iterator pepp::debug::Frame::begin() const { return _slots.cbegin(); }

pepp::debug::Frame::iterator pepp::debug::Frame::end() { return _slots.end(); }

pepp::debug::Frame::iterator pepp::debug::Frame::begin() { return _slots.begin(); }

std::size_t pepp::debug::Frame::size() const { return _slots.size(); }

bool pepp::debug::Frame::empty() const { return _slots.empty(); }

const pepp::debug::Slot *pepp::debug::Frame::at(std::size_t index) const {
  if (index >= _slots.size()) return nullptr;
  return &_slots[index];
}

const pepp::debug::Slot *pepp::debug::Frame::top() const {
  if (_slots.empty()) return nullptr;
  return &_slots.back();
}

pepp::debug::Frame::operator std::vector<std::string>() const {
  const auto end = _slots.size();
  const char fill_char = _active ? '=' : '-';
  if (end == 0) return {};
  std::vector<std::string> inner(end + 2);

  // Must reverse order to maintain correct order.
  for (int it = 0; it < end; it++)
    inner[(end - it - 1) + 1] = fmt::format("{0} {1} {0}", fill_char, std::string(_slots[it]));
  inner[0] = inner.back() = std::string(FRAME_RENDER_WIDTH, fill_char);
  return inner;
}

pepp::debug::Stack::Stack(quint32 baseAddress) : _baseAddress(baseAddress) {}

pepp::debug::Stack::Stack(Stack &&other) : _baseAddress(other._baseAddress), _frames(std::move(other._frames)) {
  other._baseAddress = 0;
}

pepp::debug::Stack &pepp::debug::Stack::operator=(Stack &&other) {
  if (this != &other) {
    _baseAddress = other._baseAddress;
    _frames = std::move(other._frames);
    other._baseAddress = 0;
  }
  return *this;
}

quint32 pepp::debug::Stack::base_address() const { return _baseAddress; }

quint32 pepp::debug::Stack::top_address() const {
  quint32 address = 0;
  for (const auto &it : std::as_const(_frames)) address = qMin(address, it.top_address());
  return address;
}

void pepp::debug::Stack::popFrame() { _frames.pop_back(); }

pepp::debug::Frame &pepp::debug::Stack::pushFrame() {
  // Consolidate consecutive empty frames.
  if (_frames.empty() || !_frames.back().empty()) {
    auto newFrame = Frame{top_address(), this};
    _frames.emplace_back(std::move(newFrame));
  };
  return _frames.back();
}

pepp::debug::Stack::const_iterator pepp::debug::Stack::cbegin() const { return _frames.cbegin(); }

pepp::debug::Stack::const_iterator pepp::debug::Stack::cend() const { return _frames.cend(); }

pepp::debug::Stack::const_iterator pepp::debug::Stack::begin() const { return _frames.cbegin(); }

pepp::debug::Stack::const_iterator pepp::debug::Stack::end() const { return _frames.cend(); }

pepp::debug::Stack::iterator pepp::debug::Stack::begin() { return _frames.begin(); }

pepp::debug::Stack::iterator pepp::debug::Stack::end() { return _frames.end(); }

std::size_t pepp::debug::Stack::size() const { return _frames.size(); }

bool pepp::debug::Stack::empty() const { return _frames.empty(); }

bool pepp::debug::Stack::contains(quint32 address) const {
  if (address > base_address()) return false;
  return address >= top_address();
}

const pepp::debug::Frame *pepp::debug::Stack::at(std::size_t index) const {
  if (index >= _frames.size()) return nullptr;
  return &_frames[index];
}

pepp::debug::Frame *pepp::debug::Stack::top() {
  if (_frames.size() == 0) return nullptr;
  return &_frames.back();
}

std::vector<std::string> pepp::debug::Stack::to_string(int left_pad) const {
  int count = 0;
  for (const auto &frame : _frames) count += frame.size() + 2;
  std::vector<std::string> ret(count);
  auto lpad = std::string(left_pad, ' ');

  int it = 0;
  for (auto frame = _frames.rbegin(); frame != _frames.rend(); ++frame) {
    auto lines = std::vector<std::string>(*frame);
    for (const auto &line : lines) ret[it++] = lpad + line;
  }
  return ret;
}
