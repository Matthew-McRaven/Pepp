#pragma once
#include "expr_eval.hpp"
#include "expr_parser.hpp"
#include "toolchain/pas/obj/trace_tags.hpp"
namespace pepp::debug {
// Empty baseclass so that Slot/Frame/Stack can be identified via dynamic_cast.
class LayoutNode {
public:
  LayoutNode() = default;
  // Disable copy-swap to prevent slicing.
  LayoutNode(const LayoutNode &) = delete;
  LayoutNode &operator=(const LayoutNode &) = delete;
  LayoutNode(LayoutNode &&) = delete;
  LayoutNode &operator=(LayoutNode &&) = delete;
  virtual ~LayoutNode() = default;
};
class Stack;
class Frame;

class Slot final : public LayoutNode {
public:
  Slot(quint32 address, quint32 size, QString name, std::shared_ptr<pepp::debug::Term> expr, Frame *parent);
  // Copying is not safe because we cannot clone expr, but we want to keep move for speed.
  Slot(const Slot &) = delete;
  Slot &operator=(const Slot &) = delete;
  Slot(Slot &&other);
  Slot &operator=(Slot &&other);
  ~Slot() override = default;

  quint32 address() const;
  quint32 size() const;
  QString name() const;
  std::shared_ptr<pepp::debug::Term> expr();
  std::shared_ptr<const pepp::debug::Term> expr() const;
  Frame *parent();
  const Frame *parent() const;
  // Helpers to inspect the current value of expr without modifying it.
  std::string value() const;
  bool is_value_dirty() const;

  // "Stringify" the slot for debugging purposes.
  operator std::string() const;

private:
  // Size is a cached value from _expr.
  quint32 _address, _size;
  QString _name;
  std::shared_ptr<pepp::debug::Term> _expr;
  Frame *_parent = nullptr;
};

class Frame final : public LayoutNode {
  using container = std::vector<Slot>;
  using iterator = typename container::iterator;
  using const_iterator = typename container::const_iterator;

public:
  Frame(quint32 baseAddress, Stack *parent);
  // Records cannot be copied, and we are a container of records. Keep move for performance
  Frame(const Frame &) = delete;
  Frame &operator=(const Frame &) = delete;
  Frame(Frame &&other);
  Frame &operator=(Frame &&other);
  ~Frame() override = default;

  Stack *parent();
  bool active() const;
  void setActive(bool active);
  quint32 base_address() const;
  quint32 top_address() const;

  void pushSlot(Slot &&slot);
  Slot popSlot();

  // Helpers to make this class act like a container of `Slot`s
  const_iterator cbegin() const;
  const_iterator cend() const;
  const_iterator begin() const;
  const_iterator end() const;
  iterator begin();
  iterator end();
  std::size_t size() const;
  bool empty() const;
  const Slot *at(std::size_t index) const;
  const Slot *top() const;

  operator std::vector<std::string>() const;

private:
  bool _active = false;
  quint32 _baseAddress = -1;
  Stack *_parent = nullptr;
  container _slots = {};
};

class Stack final : public LayoutNode {
  using container = std::vector<Frame>;
  using iterator = typename container::iterator;
  using const_iterator = typename container::const_iterator;

public:
  Stack(quint32 baseAddress);
  // Same note on rule-of-5 as Frame
  Stack(const Stack &) = delete;
  Stack &operator=(const Stack &) = delete;
  Stack(Stack &&other);
  Stack &operator=(Stack &&other);
  ~Stack() override = default;

  quint32 base_address() const;
  quint32 top_address() const;

  Frame &pushFrame();
  void popFrame();

  const_iterator cbegin() const;
  const_iterator cend() const;
  const_iterator begin() const;
  const_iterator end() const;
  iterator begin();
  iterator end();
  std::size_t size() const;
  bool empty() const;
  bool contains(quint32 address) const;
  const Frame *at(std::size_t index) const;
  Frame *top();

  std::vector<std::string> to_string(int left_pad) const;

private:
  container _frames = {};
  quint32 _baseAddress = -1;
};

} // namespace pepp::debug
