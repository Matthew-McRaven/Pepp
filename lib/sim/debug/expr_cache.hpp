#pragma once
#include <QtCore/qmutex.h>
#include <memory>
#include <set>

namespace pepp::debug {
namespace detail {
template <typename T>
concept has_link = requires(T *t) {
  { t->link() };
};
} // namespace detail
template <typename Item> struct Cache {
  struct Compare {
    using is_transparent = void;
    bool operator()(const std::shared_ptr<Item> &lhs, const std::shared_ptr<Item> &rhs) const { return *lhs < *rhs; }
    bool operator()(const Item &lhs, const std::shared_ptr<Item> &rhs) const { return lhs < *rhs; }
    bool operator()(const std::shared_ptr<Item> &lhs, const Item &rhs) const { return *lhs < rhs; }
    bool operator()(const Item &lhs, const Item &rhs) const { return lhs < rhs; }
  };
  using Set = std::set<std::shared_ptr<Item>, Compare>;
  template <typename Derived> std::shared_ptr<Derived> add_or_return(std::shared_ptr<Derived> item) {
    QMutexLocker locker(&_mut);
    if (typename Set::iterator search = _set.find(item); search == _set.end()) {
      if constexpr (detail::has_link<Derived>) item->link();
      _set.insert(item);
      return item;
    } else return std::dynamic_pointer_cast<Derived>(*search);
  }
  template <typename Derived, typename Base = std::remove_cv_t<std::remove_reference_t<Derived>>>
  std::shared_ptr<Base> add_or_return(Derived &&item) {
    // Don't want to make a pointer, and it must be mutable. remove cv and ref.
    using DerivedType = std::remove_cv_t<std::remove_reference_t<Derived>>;
    QMutexLocker locker(&_mut);
    if (typename Set::iterator search = _set.find(item); search == _set.end()) {
      std::shared_ptr<DerivedType> ret;
      // Can't forward lvalue ref?
      if constexpr (std::is_lvalue_reference_v<Derived>) ret = std::make_shared<DerivedType>(item);
      else ret = std::make_shared<DerivedType>(std::forward<Derived>(item));
      // Set up dependent tracking on creation. Only called if it exists.
      if constexpr (detail::has_link<Derived>) ret->link();
      _set.insert(ret);
      return ret;
    } else return std::dynamic_pointer_cast<Base>(*search);
  }
  // While the (code) complexity of this  algorithm is low, runtime complexity may be high.
  // For a cache of size N, it may need to recurse up to N times if there is only one long expression with no repeated
  // portions. With erase_if having linear complexity, this could make garbage collection O(N^2). That being said, I
  // expect N to be small (~30) and for there to be repeated subexpressions.
  void collect_garbage() {
    std::size_t old_size = -1, current_size = 0;
    { // Limit scope of locker so that we do not need to enable recursion in the mutex/locker
      QMutexLocker locker(&_mut);
      old_size = _set.size();
      // Remove all pointers whose only reference is the cache itself.
      std::erase_if(_set, [](const std::shared_ptr<Item> &ptr) { return ptr.use_count() == 1; });
      current_size = _set.size();
    }
    if (old_size != current_size) return collect_garbage();
  }
  std::size_t count() const {
    QMutexLocker locker(&_mut);
    return _set.size();
  };

private:
  mutable QMutex _mut;
  Set _set{};
};

} // namespace pepp::debug
