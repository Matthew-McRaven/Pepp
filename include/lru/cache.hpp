/// The MIT License (MIT)
/// Copyright (c) 2016 Peter Goldsborough
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to
/// deal in the Software without restriction, including without limitation the
/// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
/// sell copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
/// IN THE SOFTWARE.

#ifndef LRU_CACHE_HPP
#define LRU_CACHE_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <list>
#include <lru/insertion-result.hpp>
#include <lru/internal/callback-manager.hpp>
#include <lru/internal/definitions.hpp>
#include <lru/internal/information.hpp>
#include <lru/internal/last-accessed.hpp>
#include <lru/internal/optional.hpp>
#include <lru/internal/statistics-mutator.hpp>
#include <lru/internal/utility.hpp>
#include <lru/statistics.hpp>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <utility>


namespace LRU {

/// A basic LRU cache implementation.
///
/// An LRU cache is a fixed-size cache that remembers the order in which
/// elements were inserted into it. When the size of the cache exceeds its
/// capacity, the "least-recently-used" (LRU) element is erased. In our
/// implementation, usage is defined as insertion, but not lookup. That is,
/// looking up an element does not move it to the "front" of the cache (making
/// the operation faster). Only insertions (and erasures) can change the order
/// of elements. The capacity of the cache can be modified at any time.
///
/// \see LRU::TimedCache
template <typename Key,
          typename Value,
          typename HashFunction = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
class Cache {
 protected:
  using Information = Internal::Information<Key, Value>;
  using Queue = Internal::Queue<const Key>;
  using QueueIterator = typename Queue::const_iterator;


  using Map = Internal::Map<Key, Information, HashFunction, KeyEqual>;
  using MapIterator = typename Map::iterator;
  using MapConstIterator = typename Map::const_iterator;

  using CallbackManagerType = Internal::CallbackManager<Key, Value>;
  using HitCallback = typename CallbackManagerType::HitCallback;
  using MissCallback = typename CallbackManagerType::MissCallback;
  using AccessCallback = typename CallbackManagerType::AccessCallback;
  using HitCallbackContainer =
      typename CallbackManagerType::HitCallbackContainer;
  using MissCallbackContainer =
      typename CallbackManagerType::MissCallbackContainer;
  using AccessCallbackContainer =
      typename CallbackManagerType::AccessCallbackContainer;

 public:
  using Key_t = Key;
  using Value_t = Value;
  using InitializerList = std::initializer_list<std::pair<Key, Value>>;
  using StatisticsPointer = std::shared_ptr<Statistics<Key>>;
  using size_t = std::size_t;

  using iterator = MapIterator;
  using const_iterator = MapConstIterator;
  using InsertionResultType = InsertionResult<MapConstIterator>;

  /////////////////////////////////////////////////////////////////////////////
  // SPECIAL MEMBER FUNCTIONS
  /////////////////////////////////////////////////////////////////////////////


  /// Constructor.
  ///
  /// \param capacity The capacity of the cache.
  /// \param hash The hash function to use for the internal map.
  /// \param key_equal The key equality function to use for the internal map.
  explicit Cache(size_t capacity = Internal::DEFAULT_CAPACITY,
                 const HashFunction& hash = HashFunction(),
                 const KeyEqual& equal = KeyEqual())
  : _map(0, hash, equal), _capacity(capacity), _last_accessed(equal) {
  }

  /// Constructor.
  ///
  /// \param capacity The capacity of the cache.
  /// \param begin The start of a range to construct the cache with.
  /// \param end The end of a range to construct the cache with.
  /// \param hash The hash function to use for the internal map.
  /// \param key_equal The key equality function to use for the internal map.
  template <typename Iterator>
  Cache(size_t capacity,
        Iterator begin,
        Iterator end,
        const HashFunction& hash = HashFunction(),
        const KeyEqual& equal = KeyEqual())
  : Cache(capacity, hash, equal) {
    insert(begin, end);
  }

  /// Constructor.
  ///
  /// The capacity is inferred from the distance between the two iterators and
  /// lower-bounded by an internal constant $c_0$, usually 128 (i.e. the actual
  /// capacity will be $\max(\text{distance}, c_0)$).
  /// This may be expensive for iterators that are not random-access.
  ///
  /// \param begin The start of a range to construct the cache with.
  /// \param end The end of a range to construct the cache with.
  /// \param hash The hash function to use for the internal map.
  /// \param key_equal The key equality function to use for the internal map.
  template <typename Iterator>
  Cache(Iterator begin,
        Iterator end,
        const HashFunction& hash = HashFunction(),
        const KeyEqual& equal = KeyEqual())
  : Cache(
        std::max<size_t>(std::distance(begin, end), Internal::DEFAULT_CAPACITY),
        begin,
        end,
        hash,
        equal) {
  }

  /// Constructor.
  ///
  /// \param capacity The capacity of the cache.
  /// \param range A range to construct the cache with.
  /// \param hash The hash function to use for the internal map.
  /// \param key_equal The key equality function to use for the internal map.
  template <typename Range, typename = Internal::enable_if_range<Range>>
  Cache(size_t capacity,
        Range&& range,
        const HashFunction& hash = HashFunction(),
        const KeyEqual& equal = KeyEqual())
  : Cache(capacity, hash, equal) {
    insert(range);
  }

  /// Constructor.
  ///
  /// The capacity is inferred from the distance between the beginning and end
  /// of the range. This may be expensive for iterators that are not
  /// random-access.
  ///
  /// \param range A range to construct the cache with.
  /// \param hash The hash function to use for the internal map.
  /// \param key_equal The key equality function to use for the internal map.
  template <typename Range, typename = Internal::enable_if_range<Range>>
  explicit Cache(Range&& range,
                 const HashFunction& hash = HashFunction(),
                 const KeyEqual& equal = KeyEqual())
  : Cache(std::distance(std::begin(range), std::end(range)),
          std::move(range),
          hash,
          equal) {
  }

  /// Constructor.
  ///
  /// \param capacity The capacity of the cache.
  /// \param list The initializer list to construct the cache with.
  /// \param hash The hash function to use for the internal map.
  /// \param key_equal The key equality function to use for the internal map.
  Cache(InitializerList list,
        const HashFunction& hash = HashFunction(),
        const KeyEqual& equal = KeyEqual())  // NOLINT(runtime/explicit)
  : Cache(list.size(), list.begin(), list.end(), hash, equal) {
  }

  /// Constructor.
  ///
  /// \param list The initializer list to construct the cache with.
  /// \param hash The hash function to use for the internal map.
  /// \param key_equal The key equality function to use for the internal map.
  Cache(size_t capacity,
        InitializerList list,
        const HashFunction& hash = HashFunction(),
        const KeyEqual& equal = KeyEqual())  // NOLINT(runtime/explicit)
  : Cache(capacity, list.begin(), list.end(), hash, equal) {
  }

  /// Copy constructor.
  Cache(const Cache& other)
  : _map(other._map)
  , _order(other._order)
  , _stats(other._stats)
  , _last_accessed(other._last_accessed)
  , _callback_manager(other._callback_manager)
  , _capacity(other._capacity) {
    _reassign_references();
  }

  /// Move constructor.
  Cache(Cache&& other) {
    // Following the copy-swap idiom.
    swap(other);
  }

  /// Copy assignment operator.
  Cache& operator=(const Cache& other) noexcept {
    if (this != &other) {
      _map = other._map;
      _order = other._order;
      _stats = other._stats;
      _last_accessed = other._last_accessed;
      _callback_manager = other._callback_manager;
      _capacity = other._capacity;
      _reassign_references();
    }

    return *this;
  }

  /// Move assignment operator.
  Cache& operator=(Cache&& other) noexcept {
    // Following the copy-swap idiom.
    swap(other);
    return *this;
  }

  /// Destructor.
  virtual ~Cache() = default;

  /// Sets the contents of the cache to a range.
  ///
  /// If the size of the range is greater than the current capacity,
  /// the capacity is increased to match the range's size. If the size of
  /// the range is less than the current capacity, the cache's capacity is *not*
  /// changed.
  ///
  /// \param range A range of pairs to assign to the cache.
  /// \returns The cache instance.
  template <typename Range, typename = Internal::enable_if_range<Range>>
  Cache& operator=(const Range& range) {
    _clear_and_increase_capacity(range);
    insert(range);
    return *this;
  }

  /// Sets the contents of the cache to an rvalue range.
  ///
  /// Pairs of the range are moved into the cache.
  ///
  /// \param range A range of pairs to assign to the cache.
  /// \returns The cache instance.
  template <typename Range, typename = Internal::enable_if_range<Range>>
  Cache& operator=(Range&& range) {
    _clear_and_increase_capacity(range);
    insert(std::move(range));
    return *this;
  }

  /// Sets the contents of the cache to pairs from a list.
  ///
  /// \param list The list to assign to the cache.
  /// \returns The cache instance.
  Cache& operator=(InitializerList list) {
    return operator= <InitializerList>(list);
  }

  /// Swaps the contents of the cache with another cache.
  ///
  /// \param other The other cache to swap with.
  void swap(Cache& other) noexcept {
    using std::swap;

    swap(_order, other._order);
    swap(_map, other._map);
    swap(_last_accessed, other._last_accessed);
    swap(_capacity, other._capacity);
  }

  /// Swaps the contents of one cache with another cache.
  ///
  /// \param first The first cache to swap.
  /// \param second The second cache to swap.
  friend void swap(Cache& first, Cache& second) noexcept {
    first.swap(second);
  }

  /// Compares the cache for equality with another cache.
  ///
  /// \complexity O(N)
  /// \param other The other cache to compare with.
  /// \returns True if the keys __and values__ of the cache are identical to the
  ///         other, else false.
  bool operator==(const Cache& other) const noexcept {
    if (this == &other) return true;
    if (this->_map != other._map) return false;
    // clang-format off
    return std::equal(
      this->_order.begin(),
      this->_order.end(),
      other._order.begin(),
      other._order.end(),
      [](const auto& first, const auto& second) {
        return first.get() == second.get();
    });
    // clang-format on
  }

  /// Compares the cache for inequality with another cache.
  ///
  /// \complexity O(N)
  /// \param other The other cache to compare with.
  /// \returns True if there is any mismatch in keys __or their values__
  /// betweent
  /// the two caches, else false.
  bool operator!=(const Cache& other) const noexcept {
    return !(*this == other);
  }
  /////////////////////////////////////////////////////////////////////////////
  // ITERATOR INTERFACE
  /////////////////////////////////////////////////////////////////////////////
  /// \returns An unordered iterator to the beginning of the cache (this need
  /// not be the first key inserted).
  auto begin() noexcept {
    return _map.begin();
  }

  /// \returns A const unordered iterator to the beginning of the cache (this
  /// need not be the key least recently inserted).
  auto begin() const noexcept {
    return _map.begin();
  }

  /// \returns A const unordered iterator to the beginning of the cache (this
  /// need not be the key least recently inserted).
  auto cbegin() const noexcept {
    return _map.cbegin();
  }

  /// \returns An unordered iterator to the end of the cache (this
  /// need not be one past the key most recently inserted).
  auto end() noexcept {
    return _map.end();
  }

  /// \returns A const unordered iterator to the end of the cache (this
  /// need not be one past the key most recently inserted).
  auto end() const noexcept {
    return _map.end();
  }

  /// \returns A const unordered iterator to the end of the cache (this
  /// need not be one past the key most recently inserted).
  auto cend() const noexcept {
    return _map.end();
  }

  /// \returns True if the given iterator may be safely dereferenced, else
  /// false.
  /// \details Behavior is undefined if the iterator does not point into this
  /// cache.
  /// \param unordered_iterator The iterator to check.
  bool is_valid(MapIterator iterator) const noexcept {
    return iterator != end();
  }

  /// \returns True if the given iterator may be safely dereferenced, else
  /// false.
  /// \details Behavior is undefined if the iterator does not point into this
  /// cache.
  /// \param ordered_iterator The iterator to check.
  bool is_valid(MapConstIterator iterator) const noexcept {
    return iterator != cend();
  }

  /// Checks if the given iterator may be dereferencend and throws an exception
  /// if not.
  ///
  /// The exception thrown, if any, depends on the state of the iterator.
  ///
  /// \param unordered_iterator The iterator to check.
  /// \throws LRU::Error::InvalidIterator if the iterator is the end iterator.
  void throw_if_invalid(MapConstIterator iterator) const {
    if (iterator == cend()) {
      throw LRU::Error::InvalidIterator();
    }
  }
  QueueIterator ordered_cbegin() const noexcept {
    return _order.cbegin();
  }
  QueueIterator ordered_cend() const noexcept {
    return _order.cend();
  }
  /////////////////////////////////////////////////////////////////////////////
  // CACHE INTERFACE
  /////////////////////////////////////////////////////////////////////////////
  ///
  /// Tests if the given key is contained in the cache.
  ///
  /// This function may return false even if the key is actually currently
  /// stored in the cache, but the concrete cache class places some additional
  /// constraint as to when a key may be accessed (such as a time limit).
  ///
  /// \complexity O(1) expected and amortized.
  /// \param key The key to check for.
  /// \returns True if the key's value may be accessed via `lookup()` without an
  /// error, else false.
  bool contains(const Key& key) const {
    if (key == _last_accessed) {
      if (_last_accessed_is_ok(key)) {
        _register_hit(key, _last_accessed.value());
        // If this is the last accessed key, it's at the front anyway
        return true;
      } else {
        return false;
      }
    }
    return find(key) != end();
  }

  /// Looks up the value for the given key.
  ///
  /// If the key is found in the cache, it is moved to the front. Any iterators
  /// pointing to that key are still valid, but the subsequent order of
  /// iteration may be different from what it was before.
  ///
  /// \complexity O(1) expected and amortized.
  /// \param key The key whose value to look for.
  /// \throws LRU::Error::KeyNotFound if the key's value may not be accessed.
  /// \returns The value stored in the cache for the given key.
  /// \see contains()
  virtual Value& lookup(const Key& key) {
    if (key == _last_accessed) {
      auto& value = _value_for_last_accessed();
      _register_hit(key, value);
      // If this is the last accessed key, it's at the front anyway
      return value;
    }

    auto iterator = find(key);
    if (iterator == end()) throw LRU::Error::KeyNotFound();
    return iterator->second.value;
  }

  /// Attempts to return an iterator to the given key in the cache.
  ///
  /// If the key is found in the cache, it is moved to the front. Any iterators
  /// pointing to that key are still valid, but the subsequent order of
  /// iteration may be different from what it was before.
  ///
  /// \complexity O(1) expected and amortized.
  /// \param key The key whose value to look for.
  /// \returns An iterator pointing to the entry with the given key, if one
  /// exists, else the end iterator.
  MapIterator find(const Key& key) {
    auto iterator = _map.find(key);
    if (iterator != _map.end()) {
      _register_hit(key, iterator->second.value);
      _move_to_front(iterator->second.order);
      _last_accessed = iterator;
    } else
      _register_miss(key);


    return iterator;
  }

  /// Attempts to return a const iterator to the given key in the cache.
  ///
  /// If the key is found in the cache, it is moved to the front. Any iterators
  /// pointing to that key are still valid, but the subsequent order of
  /// iteration may be different from what it was before.
  ///
  /// \complexity O(1) expected and amortized.
  /// \param key The key whose value to look for.
  /// \returns A const iterator pointing to the entry with the given key, if one
  /// exists, else the end iterator.
  MapConstIterator find(const Key& key) const {
    auto iterator = _map.find(key);
    if (iterator != _map.end()) {
      _register_hit(key, iterator->second.value);
      _move_to_front(iterator->second.order);
      _last_accessed = iterator;
    } else {
      _register_miss(key);
    }

    return iterator;
  }

  MapConstIterator const_find(const Key& key) const {
    return _map.find(key);
  }

  /// \copydoc lookup(const Key&)
  virtual Value& operator[](const Key& key) {
    return lookup(key);
  }

  /// \copydoc lookup(const Key&) const
  const Value& operator[](const Key& key) const {
    return lookup(key);
  }

  /// Inserts the given `(key, value)` pair into the cache.
  ///
  /// If the cache's capacity is reached, the most recently used element will be
  /// evicted. Any iterators pointing to that element will be invalidated.
  /// Iterators pointing to other elements are not affected.
  ///
  /// \complexity O(1) expected and amortized.
  /// \param key The key to insert.
  /// \param value The value to insert with the key.
  /// \returns An `InsertionResult`, holding a boolean indicating whether the
  /// key was newly inserted (true) or only updated (false) as well as an
  /// iterator pointing to the entry for the key.
  InsertionResultType insert(const Key& key, const Value& value) {
    if (_capacity == 0) return {false, end()};

    auto iterator = _map.find(key);

    // To insert, we first check if the key is already present in the cache
    // and if so, update its value and move its order iterator to the front
    // of the queue. Else, we insert the key at the end of the queue and
    // possibly pop the front if the cache has reached its capacity.

    if (iterator == _map.end()) {
      auto result = _map.emplace(key, Information(value));
      assert(result.second);
      auto order = _insert_new_key(result.first->first);
      result.first->second.order = order;

      _last_accessed = result.first;
      return {true, result.first};
    } else {
      _move_to_front(iterator, value);
      _last_accessed = iterator;
      return {false, iterator};
    }
  }

  /// Inserts a range of `(key, value)` pairs.
  ///
  /// If, at any point, the cache's capacity is reached, the most recently used
  /// element will be evicted. Any iterators pointing to that element will
  /// be invalidated. Iterators pointing to other elements are not affected.
  ///
  /// Note: This operation has no performance benefits over
  /// element-wise insertion via `insert()`.
  ///
  /// \param begin An iterator for the start of the range to insert.
  /// \param end An iterator for the end of the range to insert.
  /// \returns The number of elements newly inserted (as opposed to only
  /// updated).
  template <typename Iterator,
            typename = Internal::enable_if_iterator_over_pair<Iterator>>
  size_t insert(Iterator begin, Iterator end) {
    size_t newly_inserted = 0;
    for (; begin != end; ++begin) {
      const auto result = insert(begin->first, begin->second);
      newly_inserted += result.was_inserted();
    }

    return newly_inserted;
  }

  /// Inserts a range of `(key, value)` pairs.
  ///
  /// If, at any point, the cache's capacity is reached, the most recently used
  /// element will be evicted. Any iterators pointing to that element will
  /// be invalidated. Iterators pointing to other elements are not affected.
  ///
  /// This operation has no performance benefits over
  /// element-wise insertion via `insert()`.
  ///
  /// \param range The range of `(key, value)` pairs to insert.
  /// \returns The number of elements newly inserted (as opposed to only
  /// updated).
  template <typename Range, typename = Internal::enable_if_range<Range>>
  size_t insert(Range& range) {
    using std::begin;
    using std::end;

    return insert(begin(range), end(range));
  }

  /// Moves the elements of the range into the cache.
  ///
  /// If, at any point, the cache's capacity is reached, the most recently used
  /// element will be evicted. Any iterators pointing to that element will
  /// be invalidated. Iterators pointing to other elements are not affected.
  ///
  /// \param range The range of `(key, value)` pairs to move into the cache.
  /// \returns The number of elements newly inserted (as opposed to only
  /// updated).
  template <typename Range, typename = Internal::enable_if_range<Range>>
  size_t insert(Range&& range) {
    size_t newly_inserted = 0;
    for (auto& pair : range) {
      const auto result =
          emplace(std::move(pair.first), std::move(pair.second));
      newly_inserted += result.was_inserted();
    }

    return newly_inserted;
  }

  /// Inserts a list `(key, value)` pairs.
  ///
  /// If the cache's capacity is reached, the most recently used element will be
  /// evicted (one or more times). Any iterators pointing to that element will
  /// be invalidated. Iterators pointing to other elements are not affected.
  ///
  /// This operation has no performance benefits over
  /// element-wise insertion via `insert()`.
  ///
  /// \param list The list of `(key, value)` pairs to insert.
  /// \returns The number of elements newly inserted (as opposed to only
  /// updated).
  virtual size_t insert(InitializerList list) {
    return insert(list.begin(), list.end());
  }

  /// Emplaces a new `(key, value)` pair into the cache.
  ///
  /// This emplacement function allows perfectly forwarding an arbitrary number
  /// of arguments to the constructor of both the key and value type, via
  /// appropriate tuples. The intended usage is with `std::forward_as_tuple`,
  /// for example:
  /// \code{.cpp}
  /// struct A { A(int, const std::string&) { } };
  /// struct B { B(double) {} };
  ///
  /// LRU::Cache<A> cache;
  ///
  /// cache.emplace(
  ///   std::piecewise_construct,
  ///   std::forward_as_tuple(1, "hello"),
  ///   std::forward_as_tuple(5.0),
  ///  );
  /// \endcode
  ///
  /// There is a convenience overload that requires much less overhead, if both
  /// constructors expect only a single argument.
  ///
  /// If the cache's capacity is reached, the most recently used element will be
  /// evicted. Any iterators pointing to that element will be invalidated.
  /// Iterators pointing to other elements are not affected.
  ///
  /// \complexity O(1) expected and amortized.
  /// \param _ A dummy parameter to work around overload resolution.
  /// \param key_arguments A tuple of arguments to construct a key object with.
  /// \param value_arguments A tuple of arguments to construct a value object
  ///                        with.
  /// \returns An `InsertionResult`, holding a boolean indicating whether the
  /// key was newly inserted (true) or only updated (false) as well as an
  /// iterator pointing to the entry for the key.
  template <typename... Ks, typename... Vs>
  InsertionResultType emplace(std::piecewise_construct_t _,
                              const std::tuple<Ks...>& key_arguments,
                              const std::tuple<Vs...>& value_arguments) {
    if (_capacity == 0) return {false, end()};

    auto key = Internal::construct_from_tuple<Key>(key_arguments);
    auto iterator = _map.find(key);

    if (iterator == _map.end()) {
      auto result = _map.emplace(std::move(key), Information(value_arguments));
      auto order = _insert_new_key(result.first->first);
      result.first->second.order = order;
      assert(result.second);

      _last_accessed = result.first;
      return {true, result.first};
    } else {
      auto value = Internal::construct_from_tuple<Value>(value_arguments);
      _move_to_front(iterator, value);
      _last_accessed = iterator;
      return {false, iterator};
    }
  }

  /// Emplaces a `(key, value)` pair.
  ///
  /// This is a convenience overload removing the necessity for
  /// `std::piecewise_construct` and `std::forward_as_tuple` that may be used in
  /// the case that both the key and value have constructors expecting only a
  /// single argument.
  ///
  /// If the cache's capacity is reached, the most recently used element will be
  /// evicted. Any iterators pointing to that element will be invalidated.
  /// Iterators pointing to other elements are not affected.
  ///
  /// \param key_argument The argument to construct a key object with.
  /// \param value_argument The argument to construct a value object with.
  /// \returns An `InsertionResult`, holding a boolean indicating whether the
  /// key was newly inserted (true) or only updated (false) as well as an
  /// iterator pointing to the entry for the key.
  template <typename K, typename V>
  InsertionResultType emplace(K&& key_argument, V&& value_argument) {
    auto key_tuple = std::forward_as_tuple(std::forward<K>(key_argument));
    auto value_tuple = std::forward_as_tuple(std::forward<V>(value_argument));
    return emplace(std::piecewise_construct, key_tuple, value_tuple);
  }

  /// Erases the given key from the cache, if it is present.
  ///
  /// If the key is not present in the cache, this is a no-op.
  /// All iterators pointing to the given key are invalidated.
  /// Other iterators are not affected.
  ///
  /// \param key The key to erase.
  /// \returns True if the key was erased, else false.
  virtual bool erase(const Key& key) {
    // No need to use _last_accessed_is_ok here, because even
    // if it has expired, it's no problem to erase it anyway
    if (_last_accessed == key) {
      _erase(_last_accessed.key(), _last_accessed.information());
      return true;
    }

    auto iterator = _map.find(key);
    if (iterator != _map.end()) {
      _erase(iterator);
      return true;
    }

    return false;
  }

  /// Erases the key pointed to by the given iterator.
  ///
  /// \param iterator The iterator whose key to erase.
  /// \throws LRU::Error::InvalidIterator if the iterator is the end iterator.
  virtual void erase(MapConstIterator iterator) {
    /// We have this overload to avoid the extra conversion-construction from
    /// unordered to ordered iterator (and renewed hash lookup)
    if (iterator == cend()) {
      throw LRU::Error::InvalidIterator();
    } else {
      _erase(iterator);
    }
  }

  /// Clears the cache entirely.
  virtual void clear() {
    _map.clear();
    _order.clear();
    _last_accessed.invalidate();
  }

  /// Requests shrinkage of the cache to the given size.
  ///
  /// If the passed size is 0, this operation is equivalent to `clear()`. If the
  /// size is greater than the current size, it is a no-op. Otherwise, the size
  /// of the cache is reduzed to the given size by repeatedly removing the least
  /// recent element.
  ///
  /// \param new_size The size to (maybe) shrink to.
  virtual void shrink(size_t new_size) {
    if (new_size >= size()) return;
    if (new_size == 0) {
      clear();
      return;
    }

    while (size() > new_size) {
      _erase_lru();
    }
  }

  /// \returns The most-recently inserted element.
  const Key& front() const {
    if (is_empty()) {
      throw LRU::Error::EmptyCache("front");
    } else {
      // The queue is reversed for natural order of iteration.
      return _order.back();
    }
  }


  /// \returns The least-recently inserted element.
  const Key& back() const {
    if (is_empty()) {
      throw LRU::Error::EmptyCache("back");
    } else {
      // The queue is reversed for natural order of iteration.
      return _order.front();
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  // SIZE AND CAPACITY INTERFACE
  /////////////////////////////////////////////////////////////////////////////
  /// \returns The number of keys present in the cache.
  virtual size_t size() const noexcept {
    return _map.size();
  }

  /// Sets the capacity of the cache to the given value.
  ///
  /// If the given capacity is less than the current capacity of the cache,
  /// the least-recently inserted element is removed repeatedly until the
  /// capacity is equal to the given value.
  ///
  /// \param new_capacity The capacity to shrink or grow to.
  virtual void capacity(size_t new_capacity) {
    // Pop the front of the cache if we have to resize
    while (size() > new_capacity) {
      _erase_lru();
    }
    _capacity = new_capacity;
  }

  /// Returns the current capacity of the cache.
  virtual size_t capacity() const noexcept {
    return _capacity;
  }

  /// \returns the number of slots left in the cache.
  ///
  /// \details After this number of elements have been inserted, the next one
  /// insertion is preceded by an erasure of the least-recently inserted
  /// element.
  virtual size_t space_left() const noexcept {
    return _capacity - size();
  }

  /// \returns True if the cache contains no elements, else false.
  virtual bool is_empty() const noexcept {
    return size() == 0;
  }

  /// \returns True if the cache's size equals its capacity, else false.
  ///
  /// \details If `is_full()` returns `true`, the next insertion is preceded by
  /// an erasure of the least-recently inserted element.
  virtual bool is_full() const noexcept {
    return size() == _capacity;
  }

  /// \returns The function used to hash keys.
  virtual HashFunction hash_function() const {
    return _map.hash_function();
  }

  /// \returns The function used to compare keys.
  virtual KeyEqual key_equal() const {
    return _map.key_eq();
  }

  /////////////////////////////////////////////////////////////////////////////
  // STATISTICS INTERFACE
  /////////////////////////////////////////////////////////////////////////////

  /// Registers the given statistics object for monitoring.
  ///
  /// This method is useful if the statistics object is to
  /// be shared between caches.
  ///
  /// Ownership of the statistics object remains with the user and __not__ with
  /// the cache object. Also, behavior is undefined if the lifetime of the cache
  /// exceeds that of the registered statistics object.
  ///
  /// \param statistics The statistics object to register.
  virtual void monitor(const StatisticsPointer& statistics) {
    _stats = statistics;
  }

  /// Registers the given statistics object for monitoring.
  ///
  /// Ownership of the statistics object is transferred to the cache.
  ///
  /// \param statistics The statistics object to register.
  virtual void monitor(StatisticsPointer&& statistics) {
    _stats = std::move(statistics);
  }

  /// Constructs a new statistics in-place in the cache.
  ///
  /// This method is useful if the cache is to have exclusive ownership of the
  /// statistics and out-of-place construction and move is inconvenient.
  ///
  /// \param args Arguments to be forwarded to the constructor of the statistics
  ///             object.
  template <typename... Args,
            typename = std::enable_if_t<
                Internal::none_of_type<StatisticsPointer, Args...>>>
  void monitor(Args&&... args) {
    _stats = std::make_shared<Statistics<Key>>(std::forward<Args>(args)...);
  }

  /// Stops any monitoring being performed with a statistics object.
  ///
  /// If the cache is not currently monitoring at all, this is a no-op.
  virtual void stop_monitoring() {
    _stats.reset();
  }

  /// \returns True if the cache is currently monitoring statistics, else
  /// false.
  bool is_monitoring() const noexcept {
    return _stats.has_stats();
  }

  /// \returns The statistics object currently in use by the cache.
  /// \throws LRU::Error::NotMonitoring if the cache is currently not
  /// monitoring.
  virtual Statistics<Key>& stats() {
    if (!is_monitoring()) {
      throw LRU::Error::NotMonitoring();
    }
    return _stats.get();
  }

  /// \returns The statistics object currently in use by the cache.
  /// \throws LRU::Error::NotMonitoring if the cache is currently not
  /// monitoring.
  virtual const Statistics<Key>& stats() const {
    if (!is_monitoring()) {
      throw LRU::Error::NotMonitoring();
    }
    return _stats.get();
  }

  /// \returns A `shared_ptr` to the statistics currently in use by the cache.
  virtual StatisticsPointer& shared_stats() {
    return _stats.shared();
  }

  /// \returns A `shared_ptr` to the statistics currently in use by the cache.
  virtual const StatisticsPointer& shared_stats() const {
    return _stats.shared();
  }

  /////////////////////////////////////////////////////////////////////////////
  // CALLBACK INTERFACE
  /////////////////////////////////////////////////////////////////////////////

  /// Registers a new hit callback.
  ///
  /// \param hit_callback The hit callback function to register with the cache.
  template <typename Callback,
            typename = Internal::enable_if_same<HitCallback, Callback>>
  void hit_callback(Callback&& hit_callback) {
    _callback_manager.hit_callback(std::forward<Callback>(hit_callback));
  }

  /// Registers a new miss callback.
  ///
  /// \param miss_callback The miss callback function to register with the
  ///                       cache.
  template <typename Callback,
            typename = Internal::enable_if_same<MissCallback, Callback>>
  void miss_callback(Callback&& miss_callback) {
    _callback_manager.miss_callback(std::forward<Callback>(miss_callback));
  }

  /// Registers a new access callback.
  ///
  /// \param access_callback The access callback function to register with the
  ///                        cache.
  template <typename Callback,
            typename = Internal::enable_if_same<AccessCallback, Callback>>
  void access_callback(Callback&& access_callback) {
    _callback_manager.access_callback(std::forward<Callback>(access_callback));
  }

  /// Clears all hit callbacks.
  void clear_hit_callbacks() {
    _callback_manager.clear_hit_callbacks();
  }

  /// Clears all miss callbacks.
  void clear_miss_callbacks() {
    _callback_manager.clear_miss_callbacks();
  }

  /// Clears all access callbacks.
  void clear_access_callbacks() {
    _callback_manager.clear_access_callbacks();
  }

  /// Clears all callbacks.
  void clear_all_callbacks() {
    _callback_manager.clear();
  }

  /// \returns All hit callbacks.
  const HitCallbackContainer& hit_callbacks() const noexcept {
    return _callback_manager.hit_callbacks();
  }

  /// \returns All miss callbacks.
  const MissCallbackContainer& miss_callbacks() const noexcept {
    return _callback_manager.miss_callbacks();
  }

  /// \returns All access callbacks.
  const AccessCallbackContainer& access_callbacks() const noexcept {
    return _callback_manager.access_callbacks();
  }

 protected:
  using MapInsertionResult = decltype(Map().emplace());
  using LastAccessed =
      typename Internal::LastAccessed<Key, Information, KeyEqual>;
  /// Moves the key pointed to by the iterator to the front of the order.
  ///
  /// \param iterator The iterator pointing to the key to move.
  virtual void _move_to_front(QueueIterator iterator) const {
    if (size() == 1) return;
    // Extract the current linked-list node and insert (splice it) at the end
    // The original iterator is not invalidated and now points to the new
    // position (which is still the same node).
    _order.splice(_order.end(), _order, iterator);
  }

  /// Moves the key pointed to by the iterator to the front of the order and
  /// assigns a new value.
  ///
  /// \param iterator The iterator pointing to the key to move.
  /// \param new_value The updated value to move the key with.
  virtual void _move_to_front(MapIterator iterator, const Value& new_value) {
    // Extract the current linked-list node and insert (splice it) at the end
    // The original iterator is not invalidated and now points to the new
    // position (which is still the same node).
    _move_to_front(iterator->second.order);
    iterator->second.value = new_value;
  }

  /// Erases the element most recently inserted into the cache.
  virtual void _erase_lru() {
    _erase(_map.find(_order.front()));
  }

  /// Erases the element pointed to by the iterator.
  ///
  /// \param iterator The iterator pointing to the key to erase.
  virtual void _erase(MapConstIterator iterator) {
    if (_last_accessed == iterator) {
      _last_accessed.invalidate();
    }

    _order.erase(iterator->second.order);
    _map.erase(iterator);
  }

  /// Erases the given key.
  ///
  /// This method is useful if the key and information are already present, to
  /// avoid an additional hash lookup to get an iterator to the corresponding
  /// map entry.
  ///
  /// \param key The key to erase.
  /// \param information The information associated with the key to erase.
  virtual void _erase(const Key& key, const Information& information) {
    if (key == _last_accessed) {
      _last_accessed.invalidate();
    }

    // To be sure, we should do this first, since the order stores a reference
    // to the key in the map.
    _order.erase(information.order);

    // Requires an additional hash-lookup, whereas erase(iterator) doesn't
    _map.erase(key);
  }

  /// Convenience methhod to get the value for an insertion result into a map.
  /// \returns The value for the given result.
  virtual Value& _value_from_result(MapInsertionResult& result) noexcept {
    // `result.first` is the map iterator (to a pair), whose `second` member
    // is
    // the information object, whose `value` member is the value stored.
    return result.first->second.value;
  }

  /// The main use of this method is that it may be override by a base class
  /// if
  /// there are any stronger constraints (such as time expiration) as to when
  /// the last-accessed object may be used to access a key.
  ///
  /// \param key The key to compare the last accessed object against.
  /// \returns True if the last-accessed object is valid.
  virtual bool _last_accessed_is_ok(const Key& key) const noexcept {
    return true;
  }

  /// \copydoc _value_for_last_accessed() const
  virtual Value& _value_for_last_accessed() {
    return _last_accessed.value();
  }

  /// Attempts to access the last accessed key's value.
  /// \returns The value of the last accessed object.
  /// \details This method exists so that derived classes may perform
  /// additional
  /// checks (and possibly throw exceptions) or perform other operations to
  /// retrieve the value.
  virtual const Value& _value_for_last_accessed() const {
    return _last_accessed.value();
  }

  /// Registers a hit for the key and performs appropriate actions.
  /// \param key The key to register a hit for.
  /// \param value The value that was found for the key.
  virtual void _register_hit(const Key& key, const Value& value) const {
    if (is_monitoring()) {
      _stats.register_hit(key);
    }

    _callback_manager.hit(key, value);
  }

  /// Registers a miss for the key and performs appropriate actions.
  /// \param key The key to register a miss for.
  virtual void _register_miss(const Key& key) const {
    if (is_monitoring()) {
      _stats.register_miss(key);
    }

    _callback_manager.miss(key);
  }

  /// The common part of both range assignment operators.
  ///
  /// \param range The range to assign to.
  template <typename Range>
  void _clear_and_increase_capacity(const Range& range) {
    using std::begin;
    using std::end;

    clear();

    auto distance = std::distance(begin(range), end(range));
    if (distance > _capacity) {
      _capacity = distance;
    }
  }

  /// Looks up each key in the queue and re-assigns it to the proper key in the
  /// map.
  ///
  /// After a copy, the reference (wrappers) in the order queue point
  /// to the keys of the other cache's map. Thus we need to re-assign them.
  void _reassign_references() noexcept {
    for (auto& key_reference : _order) {
      key_reference = std::ref(_map.find(key_reference)->first);
    }
  }

  /// Inserts a new key into the queue.
  ///
  /// If the cache is full, the LRU node is re-used.
  /// Else a node is inserted at the order.
  ///
  /// \returns The resulting iterator.
  QueueIterator _insert_new_key(const Key& key) {
    if (_is_too_full()) {
      _evict_lru_for(key);
    } else {
      _order.emplace_back(key);
    }

    return std::prev(_order.end());
  }

  /// Evicts the LRU element for the given new key.
  ///
  /// \param key The new key to insert into the queue.
  void _evict_lru_for(const Key& key) {
    _map.erase(_order.front());
    _order.front() = std::ref(key);
    _move_to_front(_order.begin());
  }

  /// \returns True if the cache is too full and an element must be evicted,
  /// else false.
  bool _is_too_full() const noexcept {
    return size() > _capacity;
  }

 private:
  /// The map from keys to information objects.
  Map _map;

  /// The queue keeping track of the insertion order of elements.
  mutable Queue _order;

  /// The object to mutate statistics if any are registered.
  mutable Internal::StatisticsMutator<Key> _stats;

  /// The last-accessed cache object.
  mutable LastAccessed _last_accessed;

  /// The callback manager to store any callbacks.
  mutable CallbackManagerType _callback_manager;

  /// The current capacity of the cache.
  size_t _capacity;
};

}  // namespace LRU

#endif  // LRU_CACHE_HPP
