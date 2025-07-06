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

enum class MetaType : quint16 { Never = 0, Primitive = 1, Pointer = 2, Array = 3, Struct = 4 };

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
using BoxedType = std::variant<X<TNever>, X<TPrimitive>, X<TPointer>, X<TArray>, X<TStruct>>;

struct TPointer {
  static const MetaType meta = MetaType::Pointer;
  quint8 pointer_size = 2;
  BoxedType to = X<TNever>{nullptr};
  std::strong_ordering operator<=>(const TPointer &) const;
  bool operator==(const TPointer &) const;
};

struct TArray {
  static const MetaType meta = MetaType::Array;
  quint8 pointer_size = 2;
  quint16 length = 2;
  BoxedType of = X<TNever>{nullptr};
  std::strong_ordering operator<=>(const TArray &) const;
  bool operator==(const TArray &) const;
};

struct TStruct {
  static const MetaType meta = MetaType::Struct;
  quint8 pointer_size = 2;
  // Map names to types + offsets
  std::vector<std::tuple<std::string, BoxedType, uint16_t>> members;
  std::strong_ordering operator<=>(const TStruct &) const;
  bool operator==(const TStruct &) const;
};

using Type = std::variant<TNever, TPrimitive, TPointer, TArray, TStruct>;

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

class RuntimeTypeInfo {
public:
  class Handle {
  public:
    Handle();
    Handle(types::Primitives t);
    bool operator==(const Handle &rhs) const;
    std::strong_ordering operator<=>(const Handle &rhs) const;
    MetaType metatype() const;

  private:
    friend class RuntimeTypeInfo;
    Handle(MetaType, quint16);
    // Can cast to MetaType
    quint16 _metatype : 3;
    // Remaining 13 bits to distingush
    quint16 _type : 13;
  };
  Handle from(Type);
  BoxedType from(Handle) const;

private:
  struct Compare {
    using is_transparent = void;
    bool operator()(const BoxedType &lhs, const BoxedType &rhs) const { return lhs < rhs; }
    bool operator()(const Type &lhs, const BoxedType &rhs) const { return lhs < rhs; }
    bool operator()(const BoxedType &lhs, const Type &rhs) const { return lhs < rhs; }
    bool operator()(const Type &lhs, const Type &rhs) const { return lhs < rhs; }
  };
  mutable QMutex _mut;
  using ForwardTypeMap = std::map<BoxedType, Handle, Compare>;
  ForwardTypeMap _type_to_handle;
  std::vector<quint16> _type_next_free_handle = std::vector<quint16>(int(MetaType::Struct) + 1, 0);
  std::map<Handle, BoxedType> _handle_to_type;
};
} // namespace pepp::debug::types
