#pragma once
#include "expr_eval.hpp"
#include "expr_parser.hpp"
#include "toolchain/pas/obj/trace_tags.hpp"
namespace pepp::debug {
// Empty baseclass so that Slot/Frame/Stack can be identified via dynamic_cast.
class LayoutNode {};
class Stack;
class Frame;

class Slot {
public:
  Slot(quint32 address, quint32 size, QString name, std::shared_ptr<pepp::debug::Term> expr, Frame *parent);

  quint32 address() const;
  quint32 size() const;
  QString name() const;
  std::shared_ptr<pepp::debug::Term> expr();
  std::shared_ptr<const pepp::debug::Term> expr() const;
  Frame *parent();
  const Frame *parent() const;

  // "Stringify" the slot for debugging purposes.
  operator std::string() const;

private:
  // Size is a cached value from _expr.
  quint32 _address, _size;
  QString _name;
  std::shared_ptr<pepp::debug::Term> _expr;
  Frame *_parent = nullptr;
};

class Frame {
  using container = std::vector<Slot>;
  using iterator = typename container::iterator;
  using const_iterator = typename container::const_iterator;

public:
  Frame(quint32 baseAddress, Stack *parent);
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
  container _slots = {};
  bool _active = false;
  quint32 _baseAddress = -1;
  Stack *_parent = nullptr;
};

class Stack {
  using container = std::vector<Frame>;
  using iterator = typename container::iterator;
  using const_iterator = typename container::const_iterator;

public:
  Stack(quint32 baseAddress);

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
