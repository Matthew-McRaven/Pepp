#include "./expr_rtti.hpp"

pepp::debug::types::RuntimeTypeInfo::Handle::Handle() : _metatype((int)types::MetaType::Never), _type(0) {}

pepp::debug::types::RuntimeTypeInfo::Handle::Handle(Primitives t)
    : _metatype((int)types::MetaType::Primitive), _type((int)t) {}

bool pepp::debug::types::RuntimeTypeInfo::Handle::operator==(const Handle &rhs) const {
  return _metatype == rhs._metatype && _type == rhs._type;
}

std::strong_ordering pepp::debug::types::RuntimeTypeInfo::Handle::operator<=>(const Handle &rhs) const {
  if (auto cmp = _metatype <=> rhs._metatype; cmp != 0) return cmp;
  return _type <=> rhs._type;
}

pepp::debug::types::RuntimeTypeInfo::Handle::Handle(MetaType meta, quint16 type) : _metatype((int)meta), _type(type) {}

pepp::debug::types::MetaType pepp::debug::types::RuntimeTypeInfo::Handle::metatype() const {
  return (MetaType)_metatype;
}

pepp::debug::types::RuntimeTypeInfo::Handle pepp::debug::types::RuntimeTypeInfo::from(Type t) {
  QMutexLocker locker(&_mut);
  if (typename ForwardTypeMap::iterator search = _type_to_handle.find(t); search == _type_to_handle.end()) {
    auto meta = metatype(t);
    auto free_index = ++(_type_next_free_handle[(int)meta]);
    auto hnd = pepp::debug::types::RuntimeTypeInfo::Handle(meta, free_index);
    auto shared = box(t);
    _type_to_handle[shared] = hnd;
    _handle_to_type[hnd] = shared;
    return hnd;
  } else return search->second;
}

pepp::debug::types::BoxedType pepp::debug::types::RuntimeTypeInfo::from(Handle handle) const {
  auto it = _handle_to_type.find(handle);
  if (it == _handle_to_type.end()) return {};
  return it->second;
}

std::optional<pepp::debug::types::RuntimeTypeInfo::Handle> pepp::debug::types::RuntimeTypeInfo::from(Type t) const {
  QMutexLocker locker(&_mut);
  if (auto search = _type_to_handle.find(t); search == _type_to_handle.end()) return std::nullopt;
  else return search->second;
}
