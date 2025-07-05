#include "expr_types.hpp"
#include <QtCore>
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
}
bool pepp::debug::types::is_unsigned(Primitives t) {
  using enum pepp::debug::types::Primitives;
  switch (t) {
  case i8: [[fallthrough]];
  case u8: return 8;
  case i16: [[fallthrough]];
  case u16: return 16;
  case i32: [[fallthrough]];
  case u32: return 32;
  }
}

pepp::debug::types::Primitives pepp::debug::types::common_type(Primitives lhs, Primitives rhs) {
  if (lhs == rhs) return lhs;
  auto lhs_bitness = bitness(lhs), rhs_bitness = bitness(rhs);
  auto lhs_unsigned = is_unsigned(lhs), rhs_unsigned = is_unsigned(rhs);
  // TODO: I think these rules can be simplified to reduce the amount of branching.
  // If both share a sign, pick the larger of the two types.
  if (lhs_unsigned == rhs_unsigned) {
    if (lhs_bitness > rhs_bitness) return lhs;
    return rhs;
  }
  // If only one is signed, prefer the unsigned type unless the signed type is bigger.
  else if (lhs_unsigned) {
    if (lhs_bitness >= rhs_bitness) return lhs;
    return rhs;
  } else {
    if (rhs_bitness >= lhs_bitness) return rhs;
    return lhs;
  }
}

std::strong_ordering pepp::debug::types::TNever::operator<=>(const TNever &) const {
  return std::strong_ordering::equivalent;
}

bool pepp::debug::types::TNever::operator==(const TNever &other) const { return true; }

std::strong_ordering pepp::debug::types::TPrimitive::operator<=>(const TPrimitive &other) const {
  return primitive <=> other.primitive;
}

bool pepp::debug::types::TPrimitive::operator==(const TPrimitive &other) const {
  return (*this <=> other) == std::strong_ordering::equal;
}

std::strong_ordering pepp::debug::types::TPointer::operator<=>(const TPointer &other) const {
  if (auto r = to <=> other.to; r != 0) return r;
  return pointer_size <=> other.pointer_size;
}

bool pepp::debug::types::TPointer::operator==(const TPointer &other) const {
  return (*this <=> other) == std::strong_ordering::equal;
}

std::strong_ordering pepp::debug::types::TArray::operator<=>(const TArray &other) const {
  if (auto r = of <=> other.of; r != 0) return r;
  else if (r = length <=> other.length; r != 0) return r;
  return pointer_size <=> other.pointer_size;
}

bool pepp::debug::types::TArray::operator==(const TArray &other) const {
  return (*this <=> other) == std::strong_ordering::equal;
}

std::strong_ordering pepp::debug::types::TStruct::operator<=>(const TStruct &other) const {
  if (members.size() != other.members.size()) return members.size() <=> other.members.size();
  else if (pointer_size != other.pointer_size) return pointer_size <=> other.pointer_size;
  auto lhs = std::next(members.cbegin(), 0);
  auto rhs = std::next(other.members.cbegin(), 0);
  while (lhs != members.cend() && rhs != other.members.cend()) {
    if (auto r = std::get<0>(*lhs) <=> std::get<0>(*rhs); r != 0) return r;
    else if (r = std::get<1>(*lhs) <=> std::get<1>(*rhs); r != 0) return r;
    else if (r = std::get<2>(*lhs) <=> std::get<2>(*rhs); r != 0) return r;
    ++lhs, ++rhs;
  }
  return std::strong_ordering::equivalent;
}

bool pepp::debug::types::TStruct::operator==(const TStruct &other) const {
  return (*this <=> other) == std::strong_ordering::equal;
}

struct IsUnsignedVisitor {
  bool operator()(const pepp::debug::types::TNever &v) const { return false; }
  bool operator()(const pepp::debug::types::TPrimitive &v) { return is_unsigned(v.primitive); }
  bool operator()(const pepp::debug::types::TPointer &v) { return true; }
  bool operator()(const pepp::debug::types::TArray &v) { return true; }
  bool operator()(const pepp::debug::types::TStruct &v) { return true; }
};

bool pepp::debug::types::is_unsigned(const Type &type) { return std::visit(IsUnsignedVisitor{}, type); }

struct BitnessVisitor {
  quint8 operator()(const pepp::debug::types::TNever &v) const { return 0; }
  quint8 operator()(const pepp::debug::types::TPrimitive &v) { return pepp::debug::types::bitness(v.primitive); }
  quint8 operator()(const pepp::debug::types::TPointer &v) { return 8 * v.pointer_size; }
  quint8 operator()(const pepp::debug::types::TArray &v) { return 8 * v.pointer_size; }
  quint8 operator()(const pepp::debug::types::TStruct &v) { return 8 * v.pointer_size; }
};

quint8 pepp::debug::types::bitness(const Type &type) { return std::visit(BitnessVisitor{}, type); }

struct QStringVisitor {
  QString operator()(const pepp::debug::types::TNever &v) const { return "<never>"; }
  QString operator()(const pepp::debug::types::TPrimitive &v) const {
    using enum pepp::debug::types::Primitives;
    switch (v.primitive) {
    case i8: return "i8";
    case u8: return "u8";
    case i16: return "i16";
    case u16: return "u16";
    case i32: return "i32";
    case u32: return "u32";
    }
  }

  QString operator()(const pepp::debug::types::TPointer &v) const {
    using namespace Qt::StringLiterals;
    return u"*%1"_s.arg(std::visit(*this, v.to));
  }
  QString operator()(const pepp::debug::types::TArray &v) const {
    using namespace Qt::StringLiterals;
    return u"%1[%2]"_s.arg(std::visit(*this, v.of)).arg(v.length);
  }
  QString operator()(const pepp::debug::types::TStruct &v) const {
    using namespace Qt::StringLiterals;
    QStringList members;
    for (const auto &[name, type, offset] : v.members)
      members.append(u"%1 %2;"_s.arg(std::visit(*this, type), QString::fromStdString(name)));
    return u"struct {\n%1\n}"_s.arg(members.join(u"\n "_s));
  }
  template <typename T> QString operator()(const std::shared_ptr<T> &v) const { return (*this)(*v); }
};

QString pepp::debug::types::to_string(const Type &type) { return std::visit(QStringVisitor{}, type); }

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

std::strong_ordering pepp::debug::types::operator<=>(const pepp::debug::types::Type &lhs,
                                                     const pepp::debug::types::Type &rhs) {
  return std::visit(OrderingVisitor{}, lhs, rhs);
}
std::strong_ordering pepp::debug::types::operator<=>(const pepp::debug::types::TypePtr &lhs,
                                                     const pepp::debug::types::TypePtr &rhs) {
  return std::visit(OrderingVisitor{}, lhs, rhs);
}
