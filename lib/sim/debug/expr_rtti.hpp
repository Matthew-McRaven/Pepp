#pragma once
#include <QtCore/qmutex.h>
#include <map>
#include "expr_cache.hpp"
#include "expr_types.hpp"

namespace pepp::debug::types {
class RuntimeTypeInfo {
public:
  class Handle {
  public:
    explicit Handle();
    explicit Handle(types::Primitives t);
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
  std::optional<Handle> from(Type) const;
  BoxedType from(Handle) const;
  BoxedType box(Type);
  std::optional<BoxedType> box(Type) const;
  // Convenience overloads for common types
  Handle from(types::Primitives);
  std::optional<Handle> from(types::Primitives) const;

private:
  struct Compare {
    using is_transparent = void;
    bool operator()(const BoxedType &lhs, const BoxedType &rhs) const { return lhs < rhs; }
    bool operator()(const Type &lhs, const BoxedType &rhs) const { return lhs < rhs; }
    bool operator()(const BoxedType &lhs, const Type &rhs) const { return lhs < rhs; }
    bool operator()(const Type &lhs, const Type &rhs) const { return lhs < rhs; }
  };
  // Unifies from/box implemementations.
  std::pair<BoxedType, Handle> add_or_get_type(Type t);

  mutable QMutex _mut;
  using ForwardTypeMap = std::map<BoxedType, Handle, Compare>;
  ForwardTypeMap _type_to_handle;
  std::vector<quint16> _type_next_free_handle = std::vector<quint16>(int(MetaType::Struct) + 1, 0);
  std::map<Handle, BoxedType> _handle_to_type;
};

// Combined with structs from expr_cache to provide a versioned type.
struct OptType {
  OptType() = default;
  explicit OptType(BoxedType type) : type(type) {}
  OptType(const OptType &other) = default;
  OptType &operator=(const OptType &other) = default;
  OptType(OptType &&other) = default;
  OptType &operator=(OptType &&other) = default;
  BoxedType type;
};

class NamedTypeInfo {
public:
  // A "token" you can give back to this class to get a type in the future.
  // Secretly an index into the _handles vector.
  struct OpaqueHandle {
    OpaqueHandle() = default;
    OpaqueHandle(const OpaqueHandle &) = default;
    OpaqueHandle &operator=(const OpaqueHandle &) = default;
    OpaqueHandle(OpaqueHandle &&) = default;
    OpaqueHandle &operator=(OpaqueHandle &&) = default;
    bool operator==(const OpaqueHandle &rhs) const { return _index == rhs._index; }
    std::strong_ordering operator<=>(const OpaqueHandle &rhs) const { return _index <=> rhs._index; }

  private:
    OpaqueHandle(quint16 index) : _index(index) {}
    friend class NamedTypeInfo;
    quint16 _index = 0;
  };
  explicit NamedTypeInfo(types::RuntimeTypeInfo &info);
  // [0] is true if the name was not yet registered.
  // [1] unconditionally contains the handle for that name, regardless of registration status.
  std::pair<bool, OpaqueHandle> register_name(const QString &);
  // Return handle for a name if it exists, else nullopt.
  std::optional<OpaqueHandle> handle(const QString &) const;
  void set_type(const OpaqueHandle &, const BoxedType &);
  void set_type(const QString &, const BoxedType &);
  std::pair<uint32_t, BoxedType> type(const OpaqueHandle &) const;
  Versioned<OptType> versioned_type(const OpaqueHandle &) const;
  inline RuntimeTypeInfo &info() { return _info; }

private:
  types::RuntimeTypeInfo &_info;
  std::map<QString, OpaqueHandle> _name_to_handle;
  // [0] must always be Never, because I have trust issues with null objects.
  std::vector<Versioned<OptType>> _handles;
};
} // namespace pepp::debug::types
