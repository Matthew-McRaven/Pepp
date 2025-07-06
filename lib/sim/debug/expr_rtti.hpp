#pragma once
#include <QtCore>
#include <map>
#include "expr_types.hpp"

namespace pepp::debug::types {
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
