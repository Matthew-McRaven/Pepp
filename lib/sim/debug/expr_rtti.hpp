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
  std::strong_ordering operator<=>(const OptType &rhs) const;
  bool operator==(const OptType &rhs) const;
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
  DirectHandle register_direct(Type &);
  DirectHandle register_direct(types::Primitives);
  std::optional<DirectHandle> get_direct(Type &) const;
  std::optional<DirectHandle> get_direct(types::Primitives) const;
  // Helper to avoid pattern: type_for_handle(register_direct_type(...))
  BoxedType box(Type &);                      // this variant will register a type if it does not exist yet.
  std::optional<BoxedType> box(Type &) const; // while this variant will return nullopt.
  BoxedType box(types::Primitives);           // Primitive types should be registered by default in CTOR.

  // A token you can give back to this class to get a type in the future.
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
    explicit IndirectHandle(quint16 index);
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
  std::pair<BoxedType, DirectHandle> add_or_get_direct(Type &t);
  // Helper that ensures dependent types inside t are registered before t.
  // Assume you already hold _mut, and that t does not depend on t.
  void register_dependents(Type t);

  mutable QMutex _mut;

  // Members for direct types
  // Second member in pair is the topological sort index. When serializing, sort items by this value before writing out.
  using DirectTypeMap = std::map<BoxedType, std::pair<DirectHandle, uint16_t>, CompareType>;
  DirectTypeMap _directTypes;
  std::vector<quint16> _nextDirectHandle = std::vector<quint16>(int(MetaType::Struct) + 1, 0);

  // Members for indirect types
  std::map<QString, IndirectHandle> _nameToIndirect;
  // [0] must always be Never, because I have trust issues with null objects.
  std::vector<Versioned<OptType>> _indirectTypes;
  void extract_strings(StringInternPool &f) const;

public:
  // Only order over _directTypes and _nameToIndirect. Mostly used to check the deserialize(serialize(...)) is
  // idempotent. _indirectTypes are ignored because they are tied to simulator state and not the set of declared types.
  std::weak_ordering operator<=>(const TypeInfo &rhs) const;
  bool operator==(const TypeInfo &rhs) const;

  static zpp::bits::errc serialize(auto &archive, auto &self) {
    using archive_type = std::remove_cvref_t<decltype(archive)>;
    if constexpr (archive_type::kind() == zpp::bits::kind::out) {
      SerializationHelper h;
      // Pool & intern strings before serializing.
      self.extract_strings(h._strs);
      auto s = std::span(h._strs.data(), h._strs.size());
      if (auto errc = archive(s); errc.code != std::errc()) return errc;

      // Extract toplological sorting info into SerializationHelper
      for (const auto &[key, value] : self._directTypes) h._type_to_index[key] = value.second;
      // Extract & sort boxed types by their topological index.
      std::vector<BoxedType> b;
      std::transform(self._directTypes.begin(), self._directTypes.end(), std::back_inserter(b),
                     [](auto &i) { return i.first; });
      std::sort(b.begin(), b.end(), [&](const auto &lhs, const auto &rhs) {
        return self._directTypes[lhs].second < self._directTypes[rhs].second;
      });

      // Hand-serialize the vector of boxed types. First the length, then all members.
      if (auto errc = archive((quint16)b.size()); errc.code != std::errc()) return errc;
      for (const auto &item : b) {
        auto t = unbox(item); // Unbox first because otherwise we can't take a ref.
        if (auto errc = types::serialize(archive, t, &h); errc.code != std::errc()) return errc;
      }

      // Serialize indirect types registrations. Don't serialize the types because these are tied to simulator state.
      if (auto errc = archive((quint16)self._nameToIndirect.size()); errc.code != std::errc()) return errc;
      for (const auto &[name, hnd] : self._nameToIndirect) {
        if (auto errc = archive(h.index_for_string(name)); errc.code != std::errc()) return errc;
        if (auto errc = archive(hnd._index); errc.code != std::errc()) return errc;
      }
      return std::errc();
    } else if constexpr (archive_type::kind() == zpp::bits::kind::in && !std::is_const<decltype(self)>()) {
      SerializationHelper h;
      quint16 type_count = 0;
      // Load string pool.
      if (auto errc = archive(h._strs.container()); errc.code != std::errc()) return errc;

      // Load type count, then load each type.
      else if (auto errc = archive(type_count); errc.code != std::errc()) return errc;
      for (int it = 0; it < type_count; ++it) {
        Type t;
        if (auto errc = types::serialize(archive, t, &h); errc.code != std::errc()) return errc;
        h._type_to_index[self.box(t)] = it;
      }

      // Load indirect types.
      quint16 indirect_handle_count = 0;
      if (auto errc = archive(indirect_handle_count); errc.code != std::errc()) return errc;
      self._indirectTypes.resize(indirect_handle_count);
      for (int it = 0; it < indirect_handle_count; ++it) {
        quint32 string_idx = 0;
        if (auto errc = archive(string_idx); errc.code != std::errc()) return errc;
        QString name = h.string_for_index(string_idx);

        quint16 index = 0;
        if (auto errc = archive(index); errc.code != std::errc()) return errc;

        self._nameToIndirect[name] = IndirectHandle{index};
      }
      return std::errc{};
    } else if constexpr (archive_type::kind() == zpp::bits::kind::in) throw std::logic_error("Can't read into const");
    throw std::logic_error("Unreachable");
  }
};
} // namespace pepp::debug::types
