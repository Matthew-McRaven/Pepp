/*
 * Copyright (c) 2025-2026 J. Stanley Warford, Matthew McRaven
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright (c) 2024, Alf-André Walla
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the BSD 3-clause license
 * along with this program. If not, see
 * <https://opensource.org/license/bsd-3-clause>
 */
#pragma once
#include <stdexcept>
#include "enums/isa/rv_types.hpp"
#include "sim3/alloc/arena.hpp" // arena()
#include "sim3/systems/notraced_riscv_isa3_system.hpp"

namespace riscv {

// View a guest memory location as a reference to a C++ object
template <AddressType address_t, typename T> struct GuestRef {
  using gaddr_t = address_t;
  using machine_t = riscv::Machine<address_t>;

  const gaddr_t ptr;

  constexpr GuestRef() noexcept : ptr(0) {}
  GuestRef(gaddr_t ptr) : ptr(ptr) {}

  /// @brief Get the address of the reference.
  /// @return The address of the reference.
  gaddr_t address() const noexcept { return ptr; }

  /// @brief Check if the reference is valid.
  /// @return True if the reference is not null.
  operator bool() const noexcept { return ptr == 0; }

  const T &get(machine_t &machine) const noexcept {
    // This function cannot silently fail, as it will
    // throw an exception if the location is invalid or misaligned.
    return *machine.memory.template memarray<T>(this->ptr, 1);
  }
  T &get(machine_t &machine) noexcept {
    // This function cannot silently fail, as it will
    // throw an exception if the location is invalid or misaligned.
    return *machine.memory.template memarray<T>(this->ptr, 1);
  }
};

// View into libstdc++'s std::string
template <AddressType address_t>
struct GuestStdString {
  using gaddr_t = address_t;
  using machine_t = riscv::Machine<address_t>;
  static constexpr std::size_t SSO = 15;

  gaddr_t ptr;
  gaddr_t size;
  union {
    char data[SSO + 1];
    gaddr_t capacity;
  };

  constexpr GuestStdString() noexcept : ptr(0), size(0), capacity(0) {}
  GuestStdString(machine_t &machine, std::string_view str = "") : ptr(0), size(0), capacity(0) {
    this->set_string(machine, 0, str);
  }
  GuestStdString(machine_t &machine, gaddr_t self, std::string_view str = "") : ptr(0), size(0), capacity(0) {
    this->set_string(machine, self, str);
  }

  bool empty() const noexcept { return size == 0; }

  std::string to_string(const machine_t &machine, std::size_t max_len = 16UL << 20) const {
    if (this->size <= SSO) return std::string(data, size);
    else if (this->size > max_len) throw std::runtime_error("Guest std::string too large (size > 16MB)");
    // Copy the string from guest memory
    const auto view = machine.memory.memview(ptr, size);
    return std::string(view.data(), view.size());
  }

  std::string_view to_view(const machine_t &machine, std::size_t max_len = 16UL << 20) const {
    if (this->size <= SSO) return std::string_view(data, size);
    else if (this->size > max_len) throw std::runtime_error("Guest std::string too large (size > 16MB)");
    // View the string from guest memory
    return machine.memory.memview(ptr, size);
  }

  void set_string(machine_t &machine, gaddr_t self, const void *str, std::size_t len, bool use_memarray = true) {
    this->free(machine);

    if (len <= SSO) {
      this->ptr = self + offsetof(GuestStdString, data);
      this->size = len;
      std::memcpy(this->data, str, len);
			this->data[len] = '\0';
    } else {
      this->ptr = machine.arena().malloc(len + 1);
      this->size = len;
      this->capacity = len;
			if (use_memarray)
			{
				char* dst = machine.memory.template memarray<char>(this->ptr, len + 1);
				std::memcpy(dst, str, len);
				dst[len] = '\0';
			}
			else
			{
				machine.memory.memcpy(this->ptr, str, len);
				machine.memory.template write<uint8_t>(this->ptr + len, 0);
			}
    }
  }
  void set_string(machine_t &machine, gaddr_t self, std::string_view str) {
    this->set_string(machine, self, str.data(), str.size());
  }

  void move(gaddr_t self) {
    if (size <= SSO) {
      this->ptr = self + offsetof(GuestStdString, data);
    }
  }

  void free(machine_t &machine) {
    if (size > SSO) {
      machine.arena().free(ptr);
    }
    this->ptr = 0;
		this->size = 0;
  }
};

template <AddressType address_t, typename T> struct GuestStdVector;

template <AddressType address_t, typename T> struct is_guest_stdvector : std::false_type {};

template <AddressType address_t, typename T>
struct is_guest_stdvector<address_t, GuestStdVector<address_t, T>> : std::true_type {};

// View into libstdc++ and LLVM libc++ std::vector (same layout)
template <AddressType address_t, typename T> struct GuestStdVector {
  using gaddr_t = address_t;
  using machine_t = riscv::Machine<address_t>;

  gaddr_t ptr_begin;
  gaddr_t ptr_end;
  gaddr_t ptr_capacity;

  constexpr GuestStdVector() noexcept : ptr_begin(0), ptr_end(0), ptr_capacity(0) {}

  GuestStdVector(machine_t &machine, std::size_t elements) : ptr_begin(0), ptr_end(0), ptr_capacity(0) {
    auto [array, self] = this->alloc(machine, elements);
    (void)self;
    for (std::size_t i = 0; i < elements; i++) {
      new (&array[i]) T();
    }
    // Set new end only after all elements are constructed
    this->ptr_end = this->ptr_begin + elements * sizeof(T);
  }

  GuestStdVector(machine_t &machine, const std::vector<std::string> &vec) : ptr_begin(0), ptr_end(0), ptr_capacity(0) {
    static_assert(std::is_same_v<T, GuestStdString<address_t>>,
                  "GuestStdVector<T> must be a vector of GuestStdString<address_t>");
    if (vec.empty()) return;

    // Specialization for std::vector<std::string>
    auto [array, self] = this->alloc(machine, vec.size());
    (void)self;
    for (std::size_t i = 0; i < vec.size(); i++) {
			T* str = new (&array[i]) T(machine, vec[i]);
			str->move(this->ptr_begin + i * sizeof(T));
		}
		// Set new end only after all elements are constructed
		this->ptr_end = this->ptr_begin + vec.size() * sizeof(T);
  }
  GuestStdVector(machine_t &machine, const std::vector<T> &vec = {}) : ptr_begin(0), ptr_end(0), ptr_capacity(0) {
    if (!vec.empty()) this->assign(machine, vec);
  }
  template <typename... Args>
  GuestStdVector(machine_t &machine, const std::array<T, sizeof...(Args)> &arr)
      : GuestStdVector(machine, std::vector<T>{arr.begin(), arr.end()}) {}

  GuestStdVector(GuestStdVector &&other) noexcept
      : ptr_begin(other.ptr_begin), ptr_end(other.ptr_end), ptr_capacity(other.ptr_capacity) {
    other.ptr_begin = 0;
    other.ptr_end = 0;
    other.ptr_capacity = 0;
  }

  // Copying is intentionally shallow/fast, in order to avoid copying/duplication
  // Use the std::move if you need proper semantics
  GuestStdVector(const GuestStdVector &other) = default;
  GuestStdVector& operator=(const GuestStdVector& other) = default;

	gaddr_t data() const noexcept { return ptr_begin; }

	std::size_t size() const noexcept {
		return size_bytes() / sizeof(T);
	}
	bool empty() const noexcept {
		return size() == 0;
	}

	std::size_t capacity() const noexcept {
		return capacity_bytes() / sizeof(T);
	}

	T& at(machine_t& machine, std::size_t index, std::size_t max_bytes = 16UL << 20) {
		if (index >= size())
			throw std::out_of_range("Guest std::vector index out of range");
		return as_array(machine, max_bytes)[index];
	}
	const T& at(machine_t& machine, std::size_t index, std::size_t max_bytes = 16UL << 20) const {
		if (index >= size())
			throw std::out_of_range("Guest std::vector index out of range");
		return as_array(machine, max_bytes)[index];
	}

	void push_back(machine_t& machine, T&& value) {
		if (size_bytes() >= capacity_bytes())
			this->increase_capacity(machine);
		T* array = machine.memory.template memarray<T>(this->data(), size() + 1);
		new (&array[size()]) T(std::move(value));
		this->ptr_end += sizeof(T);
	}
	void push_back(machine_t& machine, const T& value) {
		if (size_bytes() >= capacity_bytes())
			this->increase_capacity(machine);
		T* array = machine.memory.template memarray<T>(this->data(), size() + 1);
		new (&array[size()]) T(value);
		this->ptr_end += sizeof(T);
	}

	// Specialization for std::string and std::string_view
	void push_back(machine_t& machine, std::string_view value) {
		static_assert(std::is_same_v<T, GuestStdString<address_t>>, "GuestStdVector: T must be a GuestStdString<address_t>");
		if (size_bytes() >= capacity_bytes())
			this->increase_capacity(machine);
		T* array = machine.memory.template memarray<T>(this->data(), size() + 1);
		const gaddr_t address = this->ptr_begin + size() * sizeof(T);
		new (&array[size()]) T(machine, address, value);
		this->ptr_end += sizeof(T);
	}
	// Specialization for std::vector<U>
	template <typename U>
	void push_back(machine_t& machine, const std::vector<U>& value) {
    static_assert(is_guest_stdvector<address_t, T>::value, "GuestStdVector: T must be a GuestStdVector itself");
    this->push_back(machine, GuestStdVector<address_t, U>(machine, value));
  }

  void pop_back(machine_t &machine) {
    if (size() == 0) throw std::out_of_range("Guest std::vector is empty");
    this->free_element(machine, size() - 1);
    this->ptr_end -= sizeof(T);
  }

  void append(machine_t &machine, const T *values, std::size_t count) {
    if (size_bytes() + count * sizeof(T) > capacity_bytes()) this->reserve(machine, size() + count);
    T *array = machine.memory.template memarray<T>(this->data(), size() + count);
    for (std::size_t i = 0; i < count; i++) new (&array[size() + i]) T(values[i]);
    this->ptr_end += count * sizeof(T);
  }

  void clear(machine_t &machine) {
    for (std::size_t i = 0; i < size(); i++) this->free_element(machine, i);
    this->ptr_end = this->ptr_begin;
  }

  gaddr_t address_at(std::size_t index) const {
    if (index >= size()) throw std::out_of_range("Guest std::vector index out of range");
    return ptr_begin + index * sizeof(T);
  }

  T *as_array(const machine_t &machine, std::size_t max_bytes = 16UL << 20) {
    if (size_bytes() > max_bytes) throw std::runtime_error("Guest std::vector has size > max_bytes");
    return machine.memory.template memarray<T>(data(), size());
  }
  const T *as_array(const machine_t &machine, std::size_t max_bytes = 16UL << 20) const {
    if (size_bytes() > max_bytes)
			throw std::runtime_error("Guest std::vector has size > max_bytes");
		return machine.memory.template memarray<T>(data(), size());
  }

#if RISCV_SPAN_AVAILABLE
	std::span<T> to_span(const machine_t& machine) {
		return std::span<T>(as_array(machine), size());
	}
	std::span<const T> to_span(const machine_t& machine) const {
		return std::span<const T>(as_array(machine), size());
	}
#endif

	// Iterators
	auto begin(machine_t& machine) { return as_array(machine); }
	auto end(machine_t& machine) { return as_array(machine) + size(); }

	std::vector<T> to_vector(const machine_t& machine) const {
		if (size_bytes() > capacity_bytes())
			throw std::runtime_error("Guest std::vector has size > capacity");
		// Copy the vector from guest memory
		const size_t elements = size();
		const T *array = machine.memory.template memarray<T>(data(), elements);
		return std::vector<T>(&array[0], &array[elements]);
	}

	/// @brief Specialization for std::string
	/// @param machine The RISC-V machine
	/// @return A vector of strings
	std::vector<std::string> to_string_vector(const machine_t& machine) const {
		if constexpr (std::is_same_v<T, GuestStdString<address_t>>) {
			std::vector<std::string> vec;
			const size_t elements = size();
			const T *array = machine.memory.template memarray<T>(data(), elements);
			vec.reserve(elements);
			for (std::size_t i = 0; i < elements; i++)
				vec.push_back(array[i].to_string(machine));
			return vec;
		} else {
			throw std::runtime_error("GuestStdVector: T must be a GuestStdString<address_t>");
		}
	}

	void assign(machine_t& machine, const std::vector<T>& vec)
	{
		auto [array, self] = alloc(machine, vec.size());
		(void)self;
		std::copy(vec.begin(), vec.end(), array);
		this->ptr_end = this->ptr_begin + vec.size() * sizeof(T);
	}

	void assign(machine_t& machine, const T* values, std::size_t count)
	{
		auto [array, self] = alloc(machine, count);
		(void)self;
		std::copy(values, values + count, array);
		this->ptr_end = this->ptr_begin + count * sizeof(T);
	}

	void resize(machine_t& machine, std::size_t new_size)
	{
		if (new_size < size()) {
			for (std::size_t i = new_size; i < size(); i++)
				this->free_element(machine, i);
			this->ptr_end = this->ptr_begin + new_size * sizeof(T);
		} else if (new_size > size()) {
			if (new_size > capacity())
				this->reserve(machine, new_size);
			T* array = machine.memory.template memarray<T>(this->data(), new_size);
			for (std::size_t i = size(); i < new_size; i++)
				new (&array[i]) T();
			this->ptr_end = this->ptr_begin + new_size * sizeof(T);
		}
	}

	void reserve(machine_t& machine, std::size_t elements)
	{
		if (elements <= capacity())
			return;

    GuestStdVector<address_t, T> old_vec(std::move(*this));
    // Allocate new memory
    auto [array, self] = this->alloc(machine, elements);
    (void)self;
    if (!old_vec.empty()) {
			std::copy(old_vec.as_array(machine), old_vec.as_array(machine) + old_vec.size(), array);
			// Free the old vector manually (as we don't want to call the destructor(s))
			machine.arena().free(old_vec.ptr_begin);
		}
		this->ptr_end = this->ptr_begin + old_vec.size() * sizeof(T);
		// Adjust SSO if the vector contains std::string
		if constexpr (std::is_same_v<T, GuestStdString<address_t>>) {
			T* array = machine.memory.template memarray<T>(this->data(), size());
			for (std::size_t i = 0; i < size(); i++)
				array[i].move(this->ptr_begin + i * sizeof(T));
		}
	}

	void free(machine_t& machine) {
		if (this->ptr_begin != 0) {
			for (std::size_t i = 0; i < size(); i++)
				this->free_element(machine, i);
			machine.arena().free(this->data());
			this->ptr_begin = 0;
			this->ptr_end = 0;
			this->ptr_capacity = 0;
		}
	}

	std::size_t size_bytes() const noexcept { return ptr_end - ptr_begin; }
	std::size_t capacity_bytes() const noexcept { return ptr_capacity - ptr_begin; }

private:
	void increase_capacity(machine_t& machine) {
		this->reserve(machine, capacity() * 2 + 4);
	}

	std::tuple<T *, gaddr_t> alloc(machine_t& machine, std::size_t elements) {
		this->free(machine);

		this->ptr_begin = machine.arena().malloc(elements * sizeof(T));
		this->ptr_end = this->ptr_begin;
		this->ptr_capacity = this->ptr_begin + elements * sizeof(T);
		return { machine.memory.template memarray<T>(this->data(), elements), this->data() };
	}

	void free_element(machine_t& machine, std::size_t index) {
    if constexpr (std::is_same_v<T, GuestStdString<address_t>> || is_guest_stdvector<address_t, T>::value) {
      this->at(machine, index).free(machine);
    } else {
      this->at(machine, index).~T();
    }
  }
};

template <AddressType address_t, typename T> struct ScopedArenaObject {
  using gaddr_t = address_t;
  using machine_t = riscv::Machine<address_t>;

  template <typename... Args> ScopedArenaObject(machine_t &machine, Args &&...args) : m_machine(&machine) {
    this->m_addr = m_machine->arena().malloc(sizeof(T));
    if (this->m_addr == 0) {
      throw std::bad_alloc();
    }
    this->m_ptr = m_machine->memory.template memarray<T>(this->m_addr, 1);
    // Adjust the SSO pointer if the object is a std::string
    if constexpr (std::is_same_v<T, GuestStdString<address_t>>) {
			new (m_ptr) T(machine, std::forward<Args>(args)...);
			this->m_ptr->move(this->m_addr);
    } else if constexpr (is_guest_stdvector<address_t, T>::value) {
      new (m_ptr) T(machine, std::forward<Args>(args)...);
    } else {
      // Construct the object in place (as if trivially constructible)
      new (m_ptr) T{std::forward<Args>(args)...};
    }
  }

  ~ScopedArenaObject() {
    this->free_standard_types();
    m_machine->arena().free(this->m_addr);
  }

  T &operator*() { return *m_ptr; }
  T *operator->() { return m_ptr; }

  gaddr_t address() const { return m_addr; }

  ScopedArenaObject& operator=(const ScopedArenaObject&) = delete;

	ScopedArenaObject& operator=(const T& other) {
		// It's not possible for m_addr to be 0 here, as it would have thrown in the constructor
		this->free_standard_types();
		this->allocate_if_null();
		this->m_ptr = m_machine->memory.template memarray<T>(this->m_addr, 1);
		new (m_ptr) T(other);
		return *this;
	}

	// Special case for std::string
	ScopedArenaObject& operator=(std::string_view other) {
		static_assert(std::is_same_v<T, GuestStdString<address_t>>, "ScopedArenaObject<T> must be a GuestStdString<address_t>");
		this->allocate_if_null();
		this->m_ptr->set_string(*m_machine, this->m_addr, other.data(), other.size());
		return *this;
	}

	// Special case for std::vector
	template <typename U>
	ScopedArenaObject& operator=(const std::vector<U>& other) {
    static_assert(std::is_same_v<T, GuestStdVector<address_t, U>>,
                  "ScopedArenaObject<T> must be a GuestStdVector<W, U>");
    this->allocate_if_null();
    this->m_ptr->assign(*m_machine, other);
    return *this;
	}

	ScopedArenaObject& operator=(ScopedArenaObject&& other) {
		this->free_standard_types();
		this->m_machine = other.m_machine;
		this->m_addr = other.m_addr;
		this->m_ptr = other.m_ptr;
		other.m_addr = 0;
		other.m_ptr = nullptr;
		return *this;
	}

private:
	void allocate_if_null() {
		if (this->m_addr == 0) {
			this->m_addr = m_machine->arena().malloc(sizeof(T));
			if (this->m_addr == 0) {
				throw std::bad_alloc();
			}
			this->m_ptr = m_machine->memory.template memarray<T>(this->m_addr, 1);
		}
	}
	void free_standard_types() {
    if constexpr (is_guest_stdvector<address_t, T>::value || std::is_same_v<T, GuestStdString<address_t>>) {
      if (this->m_ptr) {
        this->m_ptr->free(*this->m_machine);
      }
    }
  }

  T*      m_ptr  = nullptr;
	gaddr_t m_addr = 0;
	machine_t* m_machine;
};

template <AddressType address_t, typename T> struct is_scoped_guest_object : std::false_type {};

template <AddressType address_t, typename T>
struct is_scoped_guest_object<address_t, ScopedArenaObject<address_t, T>> : std::true_type {};

template <AddressType address_t, typename T> struct is_scoped_guest_stdvector : std::false_type {};

template <AddressType address_t, typename T>
struct is_scoped_guest_stdvector<address_t, ScopedArenaObject<address_t, GuestStdVector<address_t, T>>>
    : std::true_type {};

template <AddressType address_t, typename T>
using ScopedGuestStdVector = ScopedArenaObject<address_t, GuestStdVector<address_t, T>>;
template <AddressType address_t> using ScopedGuestStdString = ScopedArenaObject<address_t, GuestStdString<address_t>>;

} // namespace riscv
