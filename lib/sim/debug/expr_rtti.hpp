#pragma once
#include <QtCore/qmutex.h>
#include <map>
#include <zpp_bits.h>
#include "expr_cache.hpp"
#include "expr_types.hpp"

namespace pepp::debug::types {
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

class TypeInfo {
public:
  explicit TypeInfo();
  // A token you can give back to this class to get a type in the future.
  // Used to compress the type information in pointers into 16-bits instead of 64+ bits.
  // After creation, the type for a direct handle will never change.
  class DirectHandle {
  public:
    explicit DirectHandle();
    explicit DirectHandle(types::Primitives t);
    DirectHandle(const DirectHandle &) = default;
    DirectHandle &operator=(const DirectHandle &) = default;
    DirectHandle(DirectHandle &&) = default;
    DirectHandle &operator=(DirectHandle &&) = default;
    bool operator==(const DirectHandle &rhs) const;
    std::strong_ordering operator<=>(const DirectHandle &rhs) const;
    MetaType metatype() const;

  private:
    friend class TypeInfo;
    DirectHandle(MetaType, quint16);
    // Must be allowed to cast to MetaType
    quint16 _metatype : 3;
    // Remaining 13 bits to distingush within the metatype.
    // Each metatype can figure out what to do with these bits individually.
    quint16 _type : 13;
  };
  DirectHandle register_direct(Type);
  DirectHandle register_direct(types::Primitives);
  std::optional<DirectHandle> get_direct(Type) const;
  std::optional<DirectHandle> get_direct(types::Primitives) const;
  // Helper to avoid pattern: type_for_handle(register_direct_type(...))
  BoxedType box(Type);                      // this variant will register a type if it does not exist yet.
  std::optional<BoxedType> box(Type) const; // while this variant will return nullopt.

  // A "token" you can give back to this class to get a type in the future.
  // Secretly an index into the _handles vector.
  // The underlying type for the indirect handle can be changed at any time.
  // Essentially enables forward declared types.
  struct IndirectHandle {
    IndirectHandle() = default;
    IndirectHandle(const IndirectHandle &) = default;
    IndirectHandle &operator=(const IndirectHandle &) = default;
    IndirectHandle(IndirectHandle &&) = default;
    IndirectHandle &operator=(IndirectHandle &&) = default;
    bool operator==(const IndirectHandle &rhs) const;
    std::strong_ordering operator<=>(const IndirectHandle &rhs) const;

  private:
    IndirectHandle(quint16 index);
    friend class TypeInfo;
    quint16 _index = 0;
  };
  // [0] is true if the name was not yet registered.
  // [1] unconditionally contains the handle for that name, regardless of registration status.
  std::pair<bool, IndirectHandle> register_indirect(const QString &);
  // Return handle for a name if it exists, else nullopt.
  std::optional<IndirectHandle> get_indirect(const QString &) const;
  // While DirectHandle will be converted to a BoxedType internally, it prevents you from passing any old pointer in.
  void set_indirect_type(const IndirectHandle &, const DirectHandle &);
  void set_indirect_type(const QString &, const DirectHandle &);
  // Set all indirect types to Never.
  void clear_indirect_types();

  // Helpers to extract types from handles.
  // If (somehow) the handle is invalid, these will return Never.
  BoxedType type_from(DirectHandle) const;
  BoxedType type_from(IndirectHandle) const;
  Versioned<OptType> versioned_from(IndirectHandle) const;
  uint32_t version_of(IndirectHandle) const;

private:
  struct CompareType {
    using is_transparent = void;
    bool operator()(const BoxedType &lhs, const BoxedType &rhs) const { return lhs < rhs; }
    bool operator()(const Type &lhs, const BoxedType &rhs) const { return lhs < rhs; }
    bool operator()(const BoxedType &lhs, const Type &rhs) const { return lhs < rhs; }
    bool operator()(const Type &lhs, const Type &rhs) const { return lhs < rhs; }
  };
  // Unifies direct accessor implemementations. Assumes you already hold !!_mut!!!
  std::pair<BoxedType, DirectHandle> add_or_get_direct(Type t);

  mutable QMutex _mut;

  // Members for direct types
  using DirectTypeMap = std::map<BoxedType, DirectHandle, CompareType>;
  DirectTypeMap _directTypes;
  std::vector<quint16> _nextDirectHandle = std::vector<quint16>(int(MetaType::Struct) + 1, 0);

  // Members for indirect types
  std::map<QString, IndirectHandle> _nameToIndirect;
  // [0] must always be Never, because I have trust issues with null objects.
  std::vector<Versioned<OptType>> _indirectTypes;
};
} // namespace pepp::debug::types
