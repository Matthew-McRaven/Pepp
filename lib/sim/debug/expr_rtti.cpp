#include "./expr_rtti.hpp"
#include <stdexcept>

pepp::debug::types::TypeInfo::TypeInfo() {
  _indirectTypes.emplace_back(box(types::Never{}));
  auto register_primitive = [this](types::Primitives t) {
    auto hnd = DirectHandle(t);
    auto shared = pepp::debug::types::box(t);
    _directTypes[shared] = hnd;
  };
  // Ensure that all primitives are always registered by default.
  // This allows you to create DirectHandles to primitives w/o a TypeInfo instance.
  // If you add a new primitive, you need to update this list and the DirectHandle(Primitive) CTOR.
  register_primitive(Primitives::i8);
  register_primitive(Primitives::u8);
  register_primitive(Primitives::i16);
  register_primitive(Primitives::u16);
  register_primitive(Primitives::i32);
  register_primitive(Primitives::u32);
}

pepp::debug::types::TypeInfo::DirectHandle::DirectHandle() : _metatype((int)types::MetaType::Never), _type(0) {}

pepp::debug::types::MetaType pepp::debug::types::TypeInfo::DirectHandle::metatype() const {
  return (MetaType)_metatype;
}

pepp::debug::types::TypeInfo::DirectHandle::DirectHandle(Primitives t)
    : _metatype((int)types::MetaType::Primitive), _type((int)t) {
  // Special CTOR for primitives uses the enum value as the type.
  switch (t) {
  case Primitives::i8: break;
  case Primitives::u8: break;
  case Primitives::i16: break;
  case Primitives::u16: break;
  case Primitives::i32: break;
  case Primitives::u32: break;
  default:
    // If you hit this, you need to update TypeInfo CTOR to register the primitive and then add a case
    throw std::logic_error("Unreachable cae in DirectHandle CTOR");
  }
}

pepp::debug::types::TypeInfo::DirectHandle::DirectHandle(MetaType meta, quint16 type)
    : _metatype((int)meta), _type(type) {
  // Requires TypeInfo give us a unique (metatype, type) pair.
}

bool pepp::debug::types::TypeInfo::DirectHandle::operator==(const DirectHandle &rhs) const {
  return _metatype == rhs._metatype && _type == rhs._type;
}

std::strong_ordering pepp::debug::types::TypeInfo::DirectHandle::operator<=>(const DirectHandle &rhs) const {
  if (auto cmp = _metatype <=> rhs._metatype; cmp != 0) return cmp;
  return _type <=> rhs._type;
}

pepp::debug::types::TypeInfo::DirectHandle pepp::debug::types::TypeInfo::register_direct(Type t) {
  QMutexLocker locker(&_mut);
  return add_or_get_direct(t).second;
}

pepp::debug::types::TypeInfo::DirectHandle pepp::debug::types::TypeInfo::register_direct(Primitives t) {
  return register_direct(types::Primitive{t});
}

std::optional<pepp::debug::types::TypeInfo::DirectHandle> pepp::debug::types::TypeInfo::get_direct(Type t) const {
  QMutexLocker locker(&_mut);
  if (auto search = _directTypes.find(t); search == _directTypes.end()) return std::nullopt;
  else return search->second;
}

std::optional<pepp::debug::types::TypeInfo::DirectHandle> pepp::debug::types::TypeInfo::get_direct(Primitives t) const {
  return get_direct(types::Primitive{t});
}

pepp::debug::types::BoxedType pepp::debug::types::TypeInfo::box(Type t) {
  QMutexLocker locker(&_mut);
  return add_or_get_direct(t).first;
}

std::optional<pepp::debug::types::BoxedType> pepp::debug::types::TypeInfo::box(Type t) const {
  QMutexLocker locker(&_mut);
  if (const auto &search = _directTypes.find(t); search == _directTypes.end()) return std::nullopt;
  else return search->first;
}

bool pepp::debug::types::TypeInfo::IndirectHandle::operator==(const IndirectHandle &rhs) const {
  return _index == rhs._index;
}

std::strong_ordering pepp::debug::types::TypeInfo::IndirectHandle::operator<=>(const IndirectHandle &rhs) const {
  return _index <=> rhs._index;
}

pepp::debug::types::TypeInfo::IndirectHandle::IndirectHandle(quint16 index) : _index(index) {}

std::pair<bool, pepp::debug::types::TypeInfo::IndirectHandle>
pepp::debug::types::TypeInfo::register_indirect(const QString &name) {
  QMutexLocker locker(&_mut);
  if (const auto it = _nameToIndirect.find(name); it != _nameToIndirect.end()) return {false, it->second};
  else {
    IndirectHandle hnd((quint16)_indirectTypes.size());
    auto [boxed_type, _] = add_or_get_direct(types::Never{});
    _indirectTypes.emplace_back(Versioned<OptType>{boxed_type});
    _nameToIndirect[name] = hnd;
    return {true, hnd};
  }
}

std::optional<pepp::debug::types::TypeInfo::IndirectHandle>
pepp::debug::types::TypeInfo::get_indirect(const QString &name) const {
  QMutexLocker locker(&_mut);
  if (const auto it = _nameToIndirect.find(name); it == _nameToIndirect.end()) return std::nullopt;
  else return it->second;
}

void pepp::debug::types::TypeInfo::set_indirect_type(const IndirectHandle &indirect, const DirectHandle &direct) {
  auto type = type_from(direct); // Locks a mutex so it must occur before our critical section
  QMutexLocker locker(&_mut);
  if (indirect._index < 1 || indirect._index >= _indirectTypes.size())
    throw std::out_of_range("Invalid indirect handle");
  else if (auto &item = _indirectTypes[indirect._index]; item.type != type) item.type = type, item.version++;
}

void pepp::debug::types::TypeInfo::set_indirect_type(const QString &name, const DirectHandle &direct) {
  // Since we can't unregister a name, not worried about a data race.
  if (auto hnd = get_indirect(name); !hnd)
    throw std::invalid_argument("No registered type for name: " + name.toStdString());
  else return set_indirect_type(*hnd, direct);
}

void pepp::debug::types::TypeInfo::clear_indirect_types() {
  QMutexLocker locker(&_mut);
  auto never = box(types::Never{}); // Reset all indirect types to Never.
  std::fill(_indirectTypes.begin(), _indirectTypes.end(), Versioned<OptType>{never});
}

pepp::debug::types::BoxedType pepp::debug::types::TypeInfo::type_from(IndirectHandle handle) const {
  QMutexLocker locker(&_mut);
  if (handle._index < 0 || handle._index >= _indirectTypes.size()) throw std::out_of_range("Invalid handle index");
  else return _indirectTypes[handle._index].type;
}

pepp::debug::types::BoxedType pepp::debug::types::TypeInfo::type_from(DirectHandle handle) const {
  QMutexLocker locker(&_mut);
  for (const auto &[key, value] : _directTypes)
    if (value == handle) return key;
  return {};
}

pepp::debug::Versioned<pepp::debug::types::OptType>
pepp::debug::types::TypeInfo::versioned_from(IndirectHandle handle) const {
  QMutexLocker locker(&_mut);
  if (handle._index < 0 || handle._index >= _indirectTypes.size()) throw std::out_of_range("Invalid handle index");
  else return _indirectTypes[handle._index];
}

uint32_t pepp::debug::types::TypeInfo::version_of(IndirectHandle) const {
  return versioned_from(IndirectHandle{}).version;
}

std::pair<pepp::debug::types::BoxedType, pepp::debug::types::TypeInfo::DirectHandle>
pepp::debug::types::TypeInfo::add_or_get_direct(Type t) {
  // Per function precondition, it is assumed you already hold _mut.
  if (typename DirectTypeMap::iterator search = _directTypes.find(t); search == _directTypes.end()) {
    auto meta = metatype(t);
    auto free_index = ++(_nextDirectHandle[(int)meta]);
    auto hnd = DirectHandle(meta, free_index);
    auto shared = pepp::debug::types::box(t);
    _directTypes[shared] = hnd;
    return {shared, hnd};
  } else return {search->first, search->second};
}
