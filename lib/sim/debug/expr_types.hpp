#pragma once
#include <QString>
#include <compare>
#include <cstdint>
#include <memory>
#include "expr_cache.hpp"

namespace pepp::debug::types {
Q_NAMESPACE
enum class Primitives : uint8_t { i8, u8, i16, u16, i32, u32 };
Q_ENUM_NS(Primitives);
uint32_t bitness(Primitives t);
bool is_unsigned(Primitives t);
Primitives common_type(Primitives lhs, Primitives rhs);

enum class MetaType { Never = 0, Primitive, Pointer, Array, Struct };

struct TNever {
  static const MetaType meta = MetaType::Never;
  std::strong_ordering operator<=>(const TNever &) const;
  bool operator==(const TNever &) const;
};
struct TPrimitive {
  static const MetaType meta = MetaType::Primitive;
  Primitives primitive;
  std::strong_ordering operator<=>(const TPrimitive &) const;
  bool operator==(const TPrimitive &) const;
};
// Need forward declarations since these types are recursive
struct TPointer;
struct TArray;
struct TStruct;
template <typename T> using X = std::shared_ptr<T>;
using TypePtr = std::variant<X<TNever>, X<TPrimitive>, X<TPointer>, X<TArray>, X<TStruct>>;

struct TPointer {
  static const MetaType meta = MetaType::Pointer;
  quint8 pointer_size = 2;
  TypePtr to = X<TNever>{nullptr};
  std::strong_ordering operator<=>(const TPointer &) const;
  bool operator==(const TPointer &) const;
};

struct TArray {
  static const MetaType meta = MetaType::Array;
  quint8 pointer_size = 2;
  quint16 length = 2;
  TypePtr of = X<TNever>{nullptr};
  std::strong_ordering operator<=>(const TArray &) const;
  bool operator==(const TArray &) const;
};

struct TStruct {
  static const MetaType meta = MetaType::Struct;
  quint8 pointer_size = 2;
  // Map names to types + offsets
  std::vector<std::tuple<std::string, TypePtr, uint16_t>> members;
  std::strong_ordering operator<=>(const TStruct &) const;
  bool operator==(const TStruct &) const;
};

using Type = std::variant<TNever, TPrimitive, TPointer, TArray, TStruct>;

bool is_unsigned(const Type &type);
quint8 bitness(const Type &type);
QString to_string(const Type &type);
std::strong_ordering operator<=>(const Type &lhs, const Type &rhs);
std::strong_ordering operator<=>(const TypePtr &lhs, const TypePtr &rhs);
} // namespace pepp::debug::types
