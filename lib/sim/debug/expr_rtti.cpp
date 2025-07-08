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
  return add_or_get_type(t).second;
}

pepp::debug::types::BoxedType pepp::debug::types::RuntimeTypeInfo::from(Handle handle) const {
  auto it = _handle_to_type.find(handle);
  if (it == _handle_to_type.end()) return {};
  return it->second;
}

pepp::debug::types::BoxedType pepp::debug::types::RuntimeTypeInfo::box(Type t) { return add_or_get_type(t).first; }

std::optional<pepp::debug::types::BoxedType> pepp::debug::types::RuntimeTypeInfo::box(Type t) const {
  QMutexLocker locker(&_mut);
  if (auto search = _type_to_handle.find(t); search == _type_to_handle.end()) return std::nullopt;
  else return search->first;
}

std::optional<pepp::debug::types::RuntimeTypeInfo::Handle> pepp::debug::types::RuntimeTypeInfo::from(Type t) const {
  QMutexLocker locker(&_mut);
  if (auto search = _type_to_handle.find(t); search == _type_to_handle.end()) return std::nullopt;
  else return search->second;
}

pepp::debug::types::RuntimeTypeInfo::Handle pepp::debug::types::RuntimeTypeInfo::from(types::Primitives t) {
  return from(types::Primitive{t});
}

std::pair<pepp::debug::types::BoxedType, pepp::debug::types::RuntimeTypeInfo::Handle>
pepp::debug::types::RuntimeTypeInfo::add_or_get_type(Type t) {
  QMutexLocker locker(&_mut);
  if (typename ForwardTypeMap::iterator search = _type_to_handle.find(t); search == _type_to_handle.end()) {
    auto meta = metatype(t);
    auto free_index = ++(_type_next_free_handle[(int)meta]);
    auto hnd = pepp::debug::types::RuntimeTypeInfo::Handle(meta, free_index);
    auto shared = pepp::debug::types::box(t);
    _type_to_handle[shared] = hnd;
    _handle_to_type[hnd] = shared;
    return {shared, hnd};
  } else return {search->first, search->second};
}

std::optional<pepp::debug::types::RuntimeTypeInfo::Handle>
pepp::debug::types::RuntimeTypeInfo::from(types::Primitives t) const {
  return from(types::Primitive{t});
}

pepp::debug::types::NamedTypeInfo::NamedTypeInfo(RuntimeTypeInfo &info) : _info(info), _name_to_handle(), _handles() {
  _handles.emplace_back(_info.box(types::Never{}));
}
std::pair<bool, pepp::debug::types::NamedTypeInfo::OpaqueHandle>
pepp::debug::types::NamedTypeInfo::register_name(const QString &name) {
  if (const auto it = _name_to_handle.find(name); it != _name_to_handle.end()) return {false, it->second};
  else {
    OpaqueHandle hnd((quint16)_handles.size());
    auto boxed_type = _info.box(types::Never{});
    _handles.emplace_back(Versioned<OptType>{boxed_type});
    _name_to_handle[name] = hnd;
    return {true, hnd};
  }
}

std::optional<pepp::debug::types::NamedTypeInfo::OpaqueHandle>
pepp::debug::types::NamedTypeInfo::handle(const QString &name) const {
  if (const auto it = _name_to_handle.find(name); it == _name_to_handle.end()) return std::nullopt;
  else return it->second;
}

void pepp::debug::types::NamedTypeInfo::set_type(const OpaqueHandle &hnd, const BoxedType &type) {
  if (hnd._index < 1 || hnd._index >= _handles.size()) throw std::out_of_range("Invalid handle index");
}

void pepp::debug::types::NamedTypeInfo::set_type(const QString &name, const BoxedType &type) {
  auto hnd = handle(name);
  if (!hnd) throw std::invalid_argument("No registered type for name: " + name.toStdString());
  return set_type(*hnd, type);
}

std::pair<uint32_t, pepp::debug::types::BoxedType>

pepp::debug::types::NamedTypeInfo::type(const OpaqueHandle &hnd) const {
  if (hnd._index < 0 || hnd._index >= _handles.size()) throw std::out_of_range("Invalid handle index");
  auto item = _handles[hnd._index];
  return {item.version, item.type};
}
