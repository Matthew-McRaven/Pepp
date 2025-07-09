#pragma once
#include <QString>
#include <compare>
#include <cstdint>
#include <memory>

// Forward declare all below types so I can create the variant of shared ptrs.
namespace pepp::debug::types {
struct Never;
struct Primitive;
struct Pointer;
struct Array;
struct Struct;
using BoxedType = std::variant<std::shared_ptr<Never>, std::shared_ptr<Primitive>, std::shared_ptr<Pointer>,
                               std::shared_ptr<Array>, std::shared_ptr<Struct>>;
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
};
struct Primitive {
  static const MetaType meta = MetaType::Primitive;
  Primitives primitive;
  std::strong_ordering operator<=>(const Primitive &) const;
  bool operator==(const Primitive &) const;
};

struct Pointer {
  static const MetaType meta = MetaType::Pointer;
  quint8 pointer_size = 2;
  BoxedType to = std::shared_ptr<Never>{nullptr};
  std::strong_ordering operator<=>(const Pointer &) const;
  bool operator==(const Pointer &) const;
  inline uint64_t pad_bits(uint64_t bits) const { return bits & ~(static_cast<uint64_t>(pointer_size) * 8 - 1); }
};

struct Array {
  static const MetaType meta = MetaType::Array;
  quint8 pointer_size = 2;
  quint16 length = 2;
  BoxedType of = std::shared_ptr<Never>{nullptr};
  std::strong_ordering operator<=>(const Array &) const;
  bool operator==(const Array &) const;
  inline uint64_t pad_bits(uint64_t bits) const { return bits & ~(static_cast<uint64_t>(pointer_size) * 8 - 1); }
};

struct Struct {
  static const MetaType meta = MetaType::Struct;
  quint8 pointer_size = 2;
  // Map names to types + offsets
  std::vector<std::tuple<std::string, BoxedType, uint16_t>> members;
  std::strong_ordering operator<=>(const Struct &) const;
  bool operator==(const Struct &) const;
  inline uint64_t pad_bits(uint64_t bits) const { return bits & ~(static_cast<uint64_t>(pointer_size) * 8 - 1); }
};

using Type = std::variant<Never, Primitive, Pointer, Array, Struct>;

BoxedType box(Primitives type);
BoxedType box(const Type &type);
Type unbox(const BoxedType &type);
bool is_unsigned(const Type &type);
quint8 bitness(const Type &type);
QString to_string(const Type &type);
MetaType metatype(const Type &type);
std::strong_ordering operator<=>(const Type &lhs, const Type &rhs);
std::strong_ordering operator<=>(const BoxedType &lhs, const BoxedType &rhs);
std::strong_ordering operator<=>(const BoxedType &lhs, const Type &rhs);
std::strong_ordering operator<=>(const Type &lhs, const BoxedType &rhs);
bool operator==(const Type &lhs, const Type &rhs);
bool operator==(const BoxedType &lhs, const BoxedType &rhs);
bool operator==(const BoxedType &lhs, const Type &rhs);
bool operator==(const Type &lhs, const BoxedType &rhs);

} // namespace pepp::debug::types
