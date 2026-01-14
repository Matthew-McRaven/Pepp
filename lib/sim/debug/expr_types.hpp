#pragma once
#include <QString>
#include <compare>
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <zpp_bits.h>
#include "./expr_serialize.hpp"

namespace pepp::debug::types {

// Derived from: https://www.foonathan.net/2022/05/recursive-variant-box
// It allows recursive data structures to have value semantics.
// Box is nullable, and dereferencing a null Box is undefined behavior.
template <typename T> class Box {
  std::shared_ptr<T> _impl; // May be null!

public:
  using element_type = T;
  Box() : _impl(nullptr) {}
  // Construction from a `T`, not a `T*`.
  // Marked explicit because
  explicit Box(T &&obj) : _impl(std::make_shared<T>(std::move(obj))) {}
  explicit Box(const T &obj) : _impl(new T(obj)) {}
  explicit Box(std::shared_ptr<T> &&obj) : _impl(std::move(obj)) {}
  // Copy constructor copies `T`.
  Box(const Box &other) : Box(*other._impl) {}
  Box &operator=(const Box &other) {
    *_impl = *other._impl;
    return *this;
  }
  // Move will destroy _impl in other, leaving a nulled-out box.
  Box(Box &&other) noexcept = default;
  Box &operator=(Box &&other) noexcept = default;
  ~Box() = default;

  // Access propagates constness.
  T &operator*() { return *_impl; }
  const T &operator*() const { return *_impl; }

  T *operator->() { return _impl.get(); }
  const T *operator->() const { return _impl.get(); }
};

// Changes to arg list must be mirrored in `using Type=...` later in file.
using BoxedType =
    std::variant<Box<struct Never>, Box<struct Primitive>, Box<struct Pointer>, Box<struct Array>, Box<struct Struct>>;
struct SerializationHelper {
  quint16 index_for_type(const BoxedType &type) const {
    auto it = _type_to_index.find(type);
    if (it != _type_to_index.end()) return it->second;
    throw std::out_of_range("Type not registered in SerializationHelper");
  }
  BoxedType type_for_index(quint16 index) const {
    for (const auto &[type, idx] : _type_to_index)
      if (idx == index) return type;
    throw std::out_of_range("No type found for index in SerializationHelper");
  }
  quint32 index_for_string(const QString &);
  QString string_for_index(quint32);
  friend class TypeInfo;

private:
  StringInternPool _strs;
  std::map<BoxedType, quint16> _type_to_index;
};
} // namespace pepp::debug::types

namespace pepp::debug::types {
Q_NAMESPACE
enum class Primitives : uint8_t { i8, u8, i16, u16, i32, u32 };
Q_ENUM_NS(Primitives);
uint32_t bitness(Primitives t);
bool is_unsigned(Primitives t);
Primitives make_unsigned(Primitives);
Primitives common_type(Primitives lhs, Primitives rhs);
QString to_string(Primitives);

enum class MetaType : quint16 { Never = 0, Primitive = 1, Pointer = 2, Array = 3, Struct = 4 };
struct Never {
  static const MetaType meta = MetaType::Never;
  std::strong_ordering operator<=>(const Never &) const;
  bool operator==(const Never &) const;
  constexpr static zpp::bits::errc serialize(auto &, auto &, SerializationHelper *) { return std::errc{}; }
};

struct Primitive {
  static const MetaType meta = MetaType::Primitive;
  Primitives primitive;
  std::strong_ordering operator<=>(const Primitive &) const;
  bool operator==(const Primitive &) const;
  constexpr static zpp::bits::errc serialize(auto &archive, auto &self, SerializationHelper *) {
    if (archive.kind() == zpp::bits::kind::out) {
      quint8 tmp = static_cast<quint8>(self.primitive);
      return archive(tmp);
    } else if (archive.kind() == zpp::bits::kind::in && !std::is_const<decltype(self)>()) {
      quint8 tmp;
      auto ret = archive(tmp);
      self.primitive = static_cast<Primitives>(tmp);
      return ret;
    } else if (archive.kind() == zpp::bits::kind::in) throw std::logic_error("Can't read into const");

    throw std::logic_error("Unreachable");
  }
};

struct Pointer {
  static const MetaType meta = MetaType::Pointer;
  quint8 pointer_size = 2;
  BoxedType to = Box<Never>();
  std::strong_ordering operator<=>(const Pointer &) const;
  bool operator==(const Pointer &) const;
  constexpr static zpp::bits::errc serialize(auto &archive, auto &self, SerializationHelper *helper) {
    using archive_type = std::remove_cvref_t<decltype(archive)>;
    if constexpr (archive_type::kind() == zpp::bits::kind::out) {
      if (auto errc = archive(self.pointer_size); errc.code != std::errc()) return errc;
      return archive(helper->index_for_type(self.to)); // Use helper to convert our pointer to an index!
    } else if constexpr (archive_type::kind() == zpp::bits::kind::in && !std::is_const<decltype(self)>()) {
      if (auto errc = archive(self.pointer_size); errc.code != std::errc()) return errc;
      quint16 index;
      if (auto errc = archive(index); errc.code != std::errc()) return errc;
      self.to = helper->type_for_index(index);
      return std::errc{};
    } else if constexpr (archive_type::kind() == zpp::bits::kind::in) throw std::logic_error("Can't read into const");
    throw std::logic_error("Unreachable");
  }
};

struct Array {
  static const MetaType meta = MetaType::Array;
  quint8 pointer_size = 2;
  quint16 length = 2;
  BoxedType of = Box<Never>{nullptr};
  std::strong_ordering operator<=>(const Array &) const;
  bool operator==(const Array &) const;

  constexpr static zpp::bits::errc serialize(auto &archive, auto &self, SerializationHelper *helper) {
    using archive_type = std::remove_cvref_t<decltype(archive)>;
    if constexpr (archive_type::kind() == zpp::bits::kind::out) {
      if (auto errc = archive(self.pointer_size); errc.code != std::errc()) return errc;
      else if (errc = archive(self.length); errc.code != std::errc()) return errc;
      return archive(helper->index_for_type(self.of)); // Use  helper to convert our pointer to an index!
    } else if constexpr (archive_type::kind() == zpp::bits::kind::in && !std::is_const<decltype(self)>()) {
      if (auto errc = archive(self.pointer_size); errc.code != std::errc()) return errc;
      else if (errc = archive(self.length); errc.code != std::errc()) return errc;
      quint16 index;
      if (auto errc = archive(index); errc.code != std::errc()) return errc;
      self.of = helper->type_for_index(index);
      return std::errc{};
    } else if constexpr (archive_type::kind() == zpp::bits::kind::in) throw std::logic_error("Can't read into const");
    throw std::logic_error("Unreachable");
  }
};

struct Struct {
  static const MetaType meta = MetaType::Struct;
  quint8 pointer_size = 2;
  // Map names to types + offsets
  using Tuple = std::tuple<QString, BoxedType, uint16_t>;
  std::vector<std::tuple<QString, BoxedType, uint16_t>> members;
  std::strong_ordering operator<=>(const Struct &) const;
  bool operator==(const Struct &) const;

  std::optional<std::pair<BoxedType, uint16_t>> find(const QString &member);
  static zpp::bits::errc serialize(auto &archive, auto &self, SerializationHelper *helper) {
    using archive_type = std::remove_cvref_t<decltype(archive)>;
    if constexpr (archive_type::kind() == zpp::bits::kind::out) {
      if (auto errc = archive(self.pointer_size); errc.code != std::errc()) return errc;
      else if (errc = archive((quint8)self.members.size()); errc.code != std::errc()) return errc;
      for (const auto &[name, type, offset] : self.members) {
        if (auto errc = archive(helper->index_for_string(name)); errc.code != std::errc()) return errc;
        else if (errc = archive(helper->index_for_type(type)); errc.code != std::errc()) return errc;
        else if (errc = archive(offset); errc.code != std::errc()) return errc;
      }
      return std::errc{};
    } else if constexpr (archive_type::kind() == zpp::bits::kind::in && !std::is_const<decltype(self)>()) {
      if (auto errc = archive(self.pointer_size); errc.code != std::errc()) return errc;
      quint8 tmp = 0;
      if (auto errc = archive(tmp); errc.code != std::errc()) return errc;
      self.members.resize(tmp);
      for (int it = 0; it < tmp; ++it) {
        quint32 string_idx = 0;
        if (auto errc = archive(string_idx); errc.code != std::errc()) return errc;
        QString name = helper->string_for_index(string_idx);

        quint16 type_index = 0;
        if (auto errc = archive(type_index); errc.code != std::errc()) return errc;
        auto boxed_type = helper->type_for_index(type_index);

        quint16 offset = 0;
        if (auto errc = archive(offset); errc.code != std::errc()) return errc;

        self.members[it] = std::make_tuple(name, boxed_type, offset);
      }
      return std::errc{};
    } else if constexpr (archive_type::kind() == zpp::bits::kind::in) throw std::logic_error("Can't read into const");
    throw std::logic_error("Unreachable");
  }
};

uint64_t mask_pointer_bits(uint8_t pointer_byte_size, uint64_t bits);

// Updating order or # of types requires a change to our serialization switch below.
using Type = std::variant<Never, Primitive, Pointer, Array, Struct>;

BoxedType box(Primitives type);
BoxedType box(const Type &type);
Type unbox(const BoxedType &type);
bool is_unsigned(const Type &type);
quint8 bitness(const Type &type);
QString to_string(const Type &type);
MetaType metatype(const Type &type);
namespace detail {
template <typename T> struct SerializeVistor {
  T &archive;
  SerializationHelper *helper;
  template <typename U> std::errc operator()(U &v) { return v.serialize(archive, v, helper); }
};
} // namespace detail
static zpp::bits::errc serialize(auto &archive, auto &type, SerializationHelper *helper) {
  using archive_type = std::remove_cvref_t<decltype(archive)>;
  if constexpr (archive_type::kind() == zpp::bits::kind::out) {
    if (auto errc = archive((quint8)type.index()); errc.code != std::errc()) return errc;
    return std::visit(detail::SerializeVistor{archive, helper}, type);
  } else {
    quint8 index = 0;
    if (auto errc = archive(index); errc.code != std::errc()) return errc;
    switch (index) {
    case 0: type = Never{}; return std::errc();
    case 1: {
      Primitive tmp;
      if (auto errc = tmp.serialize(archive, tmp, helper); errc.code != std::errc()) return errc;
      type = std::move(tmp);
      return std::errc();
    }
    case 2: {
      Pointer tmp;
      if (auto errc = tmp.serialize(archive, tmp, helper); errc.code != std::errc()) return errc;
      type = std::move(tmp);
      return std::errc();
    }
    case 3: {
      Array tmp;
      if (auto errc = tmp.serialize(archive, tmp, helper); errc.code != std::errc()) return errc;
      type = std::move(tmp);
      return std::errc();
    }
    case 4: {
      Struct tmp;
      if (auto errc = tmp.serialize(archive, tmp, helper); errc.code != std::errc()) return errc;
      type = std::move(tmp);
      return std::errc();
    }
    default: return std::errc::protocol_error;
    }
  }
  throw std::logic_error("Not implemented");
}

std::strong_ordering operator<=>(const Type &lhs, const Type &rhs);
std::strong_ordering operator<=>(const BoxedType &lhs, const BoxedType &rhs);
std::strong_ordering operator<=>(const BoxedType &lhs, const Type &rhs);
std::strong_ordering operator<=>(const Type &lhs, const BoxedType &rhs);
bool operator==(const Type &lhs, const Type &rhs);
bool operator==(const BoxedType &lhs, const BoxedType &rhs);
bool operator==(const BoxedType &lhs, const Type &rhs);
bool operator==(const Type &lhs, const BoxedType &rhs);

} // namespace pepp::debug::types
