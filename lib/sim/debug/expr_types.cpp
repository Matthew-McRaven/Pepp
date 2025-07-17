#include "expr_types.hpp"
#include <QList>
#include <stdexcept>

uint32_t pepp::debug::types::bitness(Primitives t) {
  using enum pepp::debug::types::Primitives;
  switch (t) {
  case i8: [[fallthrough]];
  case u8: return 8;
  case i16: [[fallthrough]];
  case u16: return 16;
  case i32: [[fallthrough]];
  case u32: return 32;
  }
  throw std::logic_error("Unreachable");
}

bool pepp::debug::types::is_unsigned(Primitives t) {
  using enum pepp::debug::types::Primitives;
  switch (t) {
  case u8: [[fallthrough]];
  case u16: [[fallthrough]];
  case u32: return true;
  case i8: [[fallthrough]];
  case i16: [[fallthrough]];
  case i32: return false;
  }
  throw std::logic_error("Unreachable");
}

pepp::debug::types::Primitives pepp::debug::types::make_unsigned(Primitives t) {
  using enum pepp::debug::types::Primitives;
  switch (t) {
  case i8: [[fallthrough]];
  case u8: return u8;
  case i16: [[fallthrough]];
  case u16: return u16;
  case i32: [[fallthrough]];
  case u32: return u32;
  }
  throw std::logic_error("Unreachable");
}
pepp::debug::types::Primitives pepp::debug::types::common_type(Primitives lhs, Primitives rhs) {
  if (lhs == rhs) return lhs;
  auto lhs_bitness = bitness(lhs), rhs_bitness = bitness(rhs);
  auto lhs_unsigned = is_unsigned(lhs), rhs_unsigned = is_unsigned(rhs);

  // Same signedness, prefer the larger type.
  if (lhs_unsigned == rhs_unsigned) return (lhs_bitness >= rhs_bitness) ? lhs : rhs;
  // If only one is signed, prefer the unsigned type unless the signed type is bigger.
  else if (!lhs_unsigned && lhs_bitness > rhs_bitness) return lhs;
  else if (!rhs_unsigned && rhs_bitness > lhs_bitness) return rhs;
  // Promote larger type to unsigned.
  return make_unsigned((lhs_bitness >= rhs_bitness) ? lhs : rhs);
}

QString pepp::debug::types::to_string(Primitives t) {
  using enum pepp::debug::types::Primitives;
  switch (t) {
  case i8: return "i8";
  case u8: return "u8";
  case i16: return "i16";
  case u16: return "u16";
  case i32: return "i32";
  case u32: return "u32";
  }
  throw std::logic_error("Unreachable");
}

std::strong_ordering pepp::debug::types::Never::operator<=>(const Never &) const {
  return std::strong_ordering::equivalent;
}

bool pepp::debug::types::Never::operator==(const Never &other) const { return true; }

std::strong_ordering pepp::debug::types::Primitive::operator<=>(const Primitive &other) const {
  return primitive <=> other.primitive;
}

bool pepp::debug::types::Primitive::operator==(const Primitive &other) const {
  return (*this <=> other) == std::strong_ordering::equal;
}

std::strong_ordering pepp::debug::types::Pointer::operator<=>(const Pointer &other) const {
  if (auto r = to <=> other.to; r != 0) return r;
  return pointer_size <=> other.pointer_size;
}

bool pepp::debug::types::Pointer::operator==(const Pointer &other) const {
  return (*this <=> other) == std::strong_ordering::equal;
}

std::strong_ordering pepp::debug::types::Array::operator<=>(const Array &other) const {
  if (auto r = of <=> other.of; r != 0) return r;
  else if (r = length <=> other.length; r != 0) return r;
  return pointer_size <=> other.pointer_size;
}

bool pepp::debug::types::Array::operator==(const Array &other) const {
  return (*this <=> other) == std::strong_ordering::equal;
}

std::strong_ordering pepp::debug::types::Struct::operator<=>(const Struct &other) const {
  if (members.size() != other.members.size()) return members.size() <=> other.members.size();
  else if (pointer_size != other.pointer_size) return pointer_size <=> other.pointer_size;
  auto lhs = std::next(members.cbegin(), 0);
  auto rhs = std::next(other.members.cbegin(), 0);
  while (lhs != members.cend() && rhs != other.members.cend()) {
    if (auto str_cmp = std::get<0>(*lhs).compare(std::get<0>(*rhs)); str_cmp < 0) return std::strong_ordering::less;
    else if (str_cmp > 0) return std::strong_ordering::greater;
    else if (auto r = std::get<1>(*lhs) <=> std::get<1>(*rhs); r != 0) return r;
    else if (r = std::get<2>(*lhs) <=> std::get<2>(*rhs); r != 0) return r;
    ++lhs, ++rhs;
  }
  return std::strong_ordering::equivalent;
}

bool pepp::debug::types::Struct::operator==(const Struct &other) const {
  return (*this <=> other) == std::strong_ordering::equal;
}

std::optional<std::pair<pepp::debug::types::BoxedType, uint16_t>>
pepp::debug::types::Struct::find(const QString &member) {
  for (const auto &[name, boxed_type, offset] : members)
    if (member == name) return {{boxed_type, offset}};
  return std::nullopt;
}

namespace detail {
using namespace pepp::debug::types;
struct BoxVisitor {
  BoxedType operator()(const Never &v) const { return std::make_shared<Never>(); }
  BoxedType operator()(const Primitive &v) { return std::make_shared<Primitive>(v); }
  BoxedType operator()(const Pointer &v) { return std::make_shared<Pointer>(v); }
  BoxedType operator()(const Array &v) { return std::make_shared<Array>(v); }
  BoxedType operator()(const Struct &v) { return std::make_shared<Struct>(v); }
};
struct UnboxVisitor {
  Type operator()(const std::shared_ptr<Never> &v) const { return *v; }
  Type operator()(const std::shared_ptr<Primitive> &v) { return *v; }
  Type operator()(const std::shared_ptr<Pointer> &v) { return *v; }
  Type operator()(const std::shared_ptr<Array> &v) { return *v; }
  Type operator()(const std::shared_ptr<Struct> &v) { return *v; }
};

struct IsUnsignedVisitor {
  bool operator()(const Never &v) const { return false; }
  bool operator()(const Primitive &v) { return is_unsigned(v.primitive); }
  bool operator()(const Pointer &v) { return true; }
  bool operator()(const Array &v) { return true; }
  bool operator()(const Struct &v) { return true; }
};

struct BitnessVisitor {
  quint8 operator()(const Never &v) const { return 0; }
  quint8 operator()(const Primitive &v) { return bitness(v.primitive); }
  quint8 operator()(const Pointer &v) { return 8 * v.pointer_size; }
  quint8 operator()(const Array &v) { return 8 * v.pointer_size; }
  quint8 operator()(const Struct &v) { return 8 * v.pointer_size; }
};

struct QStringVisitor {
  QString operator()(const Never &v) const { return "<Never>"; }
  QString operator()(const Primitive &v) const {
    using enum Primitives;
    switch (v.primitive) {
    case i8: return "i8";
    case u8: return "u8";
    case i16: return "i16";
    case u16: return "u16";
    case i32: return "i32";
    case u32: return "u32";
    }
    throw std::logic_error("Unreachable");
  }

  QString operator()(const Pointer &v) const {
    using namespace Qt::StringLiterals;
    return u"%1*"_s.arg(std::visit(*this, v.to));
  }
  QString operator()(const Array &v) const {
    using namespace Qt::StringLiterals;
    return u"%1[%2]"_s.arg(std::visit(*this, v.of)).arg(v.length);
  }
  QString operator()(const Struct &v) const {
    using namespace Qt::StringLiterals;
    QStringList members;
    for (const auto &[name, type, offset] : v.members) members.append(u"%1 %2;"_s.arg(std::visit(*this, type), name));
    return u"struct {\n%1\n}"_s.arg(members.join(u"\n "_s));
  }
  template <typename T> QString operator()(const std::shared_ptr<T> &v) const { return (*this)(*v); }
};

struct MetatypeVisitor {
  template <typename T> MetaType operator()(const T &) const { return T::meta; }
};

template <typename T>
concept NotSharedPtr = !requires { typename T::element_type; };

struct OrderingVisitor {
  template <typename T>
    requires(NotSharedPtr<T>)
  std::strong_ordering operator()(const T &lhs, const T &rhs) {
    return lhs <=> rhs;
  }
  template <typename T> std::strong_ordering operator()(const std::shared_ptr<T> &lhs, const std::shared_ptr<T> &rhs) {
    return *lhs <=> *rhs;
  }
  template <typename T> std::strong_ordering operator()(const std::shared_ptr<T> &lhs, const T &rhs) const {
    return (*lhs) <=> rhs;
  }

  template <typename T> std::strong_ordering operator()(const T &lhs, const std::shared_ptr<T> &rhs) const {
    return lhs <=> (*rhs);
  }

  template <typename T, typename U>
    requires(!std::is_same_v<T, U>)
  std::strong_ordering operator()(const std::shared_ptr<T> &, const std::shared_ptr<U> &) {
    return T::meta <=> U::meta;
  }
  template <typename T, typename U>
    requires(!std::is_same_v<T, U> && NotSharedPtr<U>)
  std::strong_ordering operator()(const std::shared_ptr<T> &, const U &) {
    return T::meta <=> U::meta;
  }
  template <typename T, typename U>
    requires(!std::is_same_v<T, U> && NotSharedPtr<T>)
  std::strong_ordering operator()(const T &, const std::shared_ptr<U> &) {
    return T::meta <=> U::meta;
  }
  template <typename T, typename U>
    requires(!std::is_same_v<T, U> && NotSharedPtr<T> && NotSharedPtr<U>)
  std::strong_ordering operator()(const T &, const U &) {
    return T::meta <=> U::meta;
  }
};

} // namespace detail

pepp::debug::types::BoxedType pepp::debug::types::box(Primitives type) { return box(types::Primitive{type}); }

pepp::debug::types::BoxedType pepp::debug::types::box(const Type &type) {
  return std::visit(::detail::BoxVisitor{}, type);
}

pepp::debug::types::Type pepp::debug::types::unbox(const pepp::debug::types::BoxedType &type) {
  return std::visit(::detail::UnboxVisitor{}, type);
}

bool pepp::debug::types::is_unsigned(const Type &type) { return std::visit(::detail::IsUnsignedVisitor{}, type); }

quint8 pepp::debug::types::bitness(const Type &type) { return std::visit(::detail::BitnessVisitor{}, type); }

QString pepp::debug::types::to_string(const Type &type) { return std::visit(::detail::QStringVisitor{}, type); }

pepp::debug::types::MetaType pepp::debug::types::metatype(const Type &type) {
  return std::visit(::detail::MetatypeVisitor{}, type);
}

std::strong_ordering pepp::debug::types::operator<=>(const pepp::debug::types::Type &lhs,
                                                     const pepp::debug::types::Type &rhs) {
  return std::visit(::detail::OrderingVisitor{}, lhs, rhs);
}
std::strong_ordering pepp::debug::types::operator<=>(const pepp::debug::types::BoxedType &lhs,
                                                     const pepp::debug::types::BoxedType &rhs) {
  return std::visit(::detail::OrderingVisitor{}, lhs, rhs);
}

std::strong_ordering pepp::debug::types::operator<=>(const pepp::debug::types::BoxedType &lhs,
                                                     const pepp::debug::types::Type &rhs) {
  return std::visit(::detail::OrderingVisitor{}, lhs, rhs);
}

std::strong_ordering pepp::debug::types::operator<=>(const pepp::debug::types::Type &lhs,
                                                     const pepp::debug::types::BoxedType &rhs) {
  return std::visit(::detail::OrderingVisitor{}, lhs, rhs);
}

bool pepp::debug::types::operator==(const Type &lhs, const Type &rhs) {
  return (lhs <=> rhs) == std::strong_ordering::equal;
}

bool pepp::debug::types::operator==(const BoxedType &lhs, const Type &rhs) {
  return (lhs <=> rhs) == std::strong_ordering::equal;
}

bool pepp::debug::types::operator==(const BoxedType &lhs, const BoxedType &rhs) {
  return (lhs <=> rhs) == std::strong_ordering::equal;
}

bool pepp::debug::types::operator==(const Type &lhs, const BoxedType &rhs) {
  return (lhs <=> rhs) == std::strong_ordering::equal;
}

quint32 pepp::debug::types::SerializationHelper::index_for_string(const QString &str) { return _strs.add(str); }

QString pepp::debug::types::SerializationHelper::string_for_index(quint32 index) { return _strs.at(index); }
