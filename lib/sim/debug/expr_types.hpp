#pragma once
#include <QString>
#include <compare>
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <zpp_bits.h>
#include "./expr_serialize.hpp"

// Forward declare all below types so I can create the variant of shared ptrs.
namespace pepp::debug::types {
struct Never;
struct Primitive;
struct Pointer;
struct Array;
struct Struct;
using BoxedType = std::variant<std::shared_ptr<Never>, std::shared_ptr<Primitive>, std::shared_ptr<Pointer>,
                               std::shared_ptr<Array>, std::shared_ptr<Struct>>;
struct SerializationHelper {
  quint16 get_index_for(const BoxedType &type) const {
    auto it = _type_to_index.find(type);
    if (it != _type_to_index.end()) return it->second;
    throw std::out_of_range("Type not registered in SerializationHelper");
  }
  quint32 string_index_for(const QString &);
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
  constexpr static zpp::bits::errc serialize(auto &archive, auto &self, SerializationHelper *helper) {
    return std::errc{};
  }
};

struct Primitive {
  static const MetaType meta = MetaType::Primitive;
  Primitives primitive;
  std::strong_ordering operator<=>(const Primitive &) const;
  bool operator==(const Primitive &) const;
  constexpr static zpp::bits::errc serialize(auto &archive, auto &self, SerializationHelper *helper) {
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
  BoxedType to = std::shared_ptr<Never>{nullptr};
  std::strong_ordering operator<=>(const Pointer &) const;
  bool operator==(const Pointer &) const;
  inline uint64_t pad_bits(uint64_t bits) const { return bits & ~(static_cast<uint64_t>(pointer_size) * 8 - 1); }
  constexpr static zpp::bits::errc serialize(auto &archive, auto &self, SerializationHelper *helper) {
    using archive_type = std::remove_cvref_t<decltype(archive)>;
    if constexpr (archive_type::kind() == zpp::bits::kind::out) {
      if (auto errc = archive(self.pointer_size); errc.code != std::errc()) return errc;
      return archive(helper->get_index_for(self.to)); // Use helper to convert our pointer to an index!
    } else if constexpr (archive_type::kind() == zpp::bits::kind::in && !std::is_const<decltype(self)>()) {
      if (auto errc = archive(self.pointer_size); errc.code != std::errc()) return errc;
      // TODO: use helper to convert int/index to a ptr.
      return std::errc{};
    } else if constexpr (archive_type::kind() == zpp::bits::kind::in) throw std::logic_error("Can't read into const");
    throw std::logic_error("Unreachable");
  }
};

struct Array {
  static const MetaType meta = MetaType::Array;
  quint8 pointer_size = 2;
  quint16 length = 2;
  BoxedType of = std::shared_ptr<Never>{nullptr};
  std::strong_ordering operator<=>(const Array &) const;
  bool operator==(const Array &) const;
  inline uint64_t pad_bits(uint64_t bits) const { return bits & ~(static_cast<uint64_t>(pointer_size) * 8 - 1); }
  constexpr static zpp::bits::errc serialize(auto &archive, auto &self, SerializationHelper *helper) {
    using archive_type = std::remove_cvref_t<decltype(archive)>;
    if constexpr (archive_type::kind() == zpp::bits::kind::out) {
      if (auto errc = archive(self.pointer_size); errc.code != std::errc()) return errc;
      else if (errc = archive(self.length); errc.code != std::errc()) return errc;
      return archive(helper->get_index_for(self.of)); // Use  helper to convert our pointer to an index!
    } else if constexpr (archive_type::kind() == zpp::bits::kind::in && !std::is_const<decltype(self)>()) {
      if (auto errc = archive(self.pointer_size); errc.code != std::errc()) return errc;
      else if (errc = archive(self.length); errc.code != std::errc()) return errc;
      // TODO: use helper to convert int/index to a ptr.
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
  inline uint64_t pad_bits(uint64_t bits) const { return bits & ~(static_cast<uint64_t>(pointer_size) * 8 - 1); }
  std::optional<std::pair<BoxedType, uint16_t>> find(const QString &member);
  constexpr static zpp::bits::errc serialize(auto &archive, auto &self, SerializationHelper *helper) {
    using archive_type = std::remove_cvref_t<decltype(archive)>;
    if constexpr (archive_type::kind() == zpp::bits::kind::out) {
      if (auto errc = archive(self.pointer_size); errc.code != std::errc()) return errc;
      for (const auto &[name, type, offset] : self.members) {
        if (auto errc = archive(helper->string_index_for(name)); errc.code != std::errc()) return errc;
        if (auto errc = archive(helper->get_index_for(type)); errc.code != std::errc()) return errc;
        if (auto errc = archive(offset); errc.code != std::errc()) return errc;
      }
      return std::errc{};
    } else if constexpr (archive_type::kind() == zpp::bits::kind::in && !std::is_const<decltype(self)>()) {
      if (auto errc = archive(self.pointer_size); errc.code != std::errc()) return errc;
      // TODO: use helper to convert int/index to a ptr.
      return std::errc{};
    } else if constexpr (archive_type::kind() == zpp::bits::kind::in) throw std::logic_error("Can't read into const");
    throw std::logic_error("Unreachable");
  }
};

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
static zpp::bits::errc serialize(auto &archive, auto &type, SerializationHelper *helper = nullptr) {
  if (archive.kind() == zpp::bits::kind::out) {
    if (auto errc = archive((quint8)type.index()); errc.code != std::errc()) return errc;
    return std::visit(detail::SerializeVistor{archive, helper}, type);
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
