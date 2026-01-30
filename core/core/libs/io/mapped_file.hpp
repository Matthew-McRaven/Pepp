/*
 * /Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include "core/libs/bitmanip/integers.h"
#include "core/libs/bitmanip/span.hpp"
namespace pepp::bts {

/* A class which provide platform-agnostic, RAII style access to memory-mapped files
 * On platforms without memory-mapping support, it falls back fstreams.
 * MappedFile lazily opens a file, which can be viewed in spans (slices).
 * Slices lazily map the file region on first access using information from the original MappedFile instance.
 */
class MappedFile : public std::enable_shared_from_this<MappedFile> {
  struct private_ctor_tag {};

public:
  /*
   * Represents a span of the data in a MappedFile. It is lazily loaded on first access.
   * On systems which do not support memory-mapping, it falls back to reading the data into a vector.
   * I think Slices must be released before the containing MappedFile is destroyed because the fd must remain valid for
   * the lifetime of a mapping, but I'm frankly unsure.
   *
   * While it is possible to create multiple Slices that overlap the same region of a MappedFile, I consider that to be
   * illegal usage and do not guarantee correct behavior in that case. However, I cannot detect or prevent it.
   */
  class Slice {
  public:
    Slice() = default;
    // Public to only make_shared to work, but should be treated as-if private.
    // It takes a shared_ptr to a "containing" mapped to ensure that the fd remains open longer than all mappings are
    // active.
    Slice(private_ctor_tag, std::shared_ptr<MappedFile> file, size_t file_offset, size_t file_len, bool readonly);
    // Must release() in dtor.
    ~Slice();
    Slice(const Slice &) = delete;
    Slice &operator=(const Slice &) = delete;
    Slice(Slice &&o) noexcept = default;
    Slice &operator=(Slice &&o) noexcept;

    // Methods that do not trigger a load.
    size_t size() const noexcept;
    inline bool readonly() const noexcept { return _readonly; }

    // Methods that trigger a load.
    bits::span<u8> get() noexcept;
    bits::span<const u8> get() const noexcept;
    // Ensure any changes are flushed to the containing MappedFile.
    void flush();

  private:
    inline void ensure_loaded() const {
      if (!_loaded && _file != nullptr) _file->load_mapped(*const_cast<Slice *>(this));
    }
    // Perform platform-specific release of mapped memory within this class.
    void release() noexcept;
    friend class MappedFile;
    bool _readonly = false, _loaded = false;
    mutable std::size_t _file_offset = 0, _file_len = 0, _map_len = 0;
#if defined(_WIN32)
    mutable void *_hMap = nullptr;
#endif
    mutable void *_map_base = nullptr;
    mutable std::shared_ptr<MappedFile> _file = nullptr;
    mutable std::vector<u8> _fallback_buf{};
    mutable bits::span<u8> _data_view = {};
  };
  MappedFile() = delete;
  // Public to only make_shared to work, but should be treated as-if private.
  MappedFile(private_ctor_tag, std::string path, bool readonly);
  ~MappedFile();
  MappedFile(const MappedFile &) = delete;
  MappedFile &operator=(const MappedFile &) = delete;
  MappedFile(MappedFile &&o) noexcept = default;
  MappedFile &operator=(MappedFile &&o) noexcept;

  static std::shared_ptr<MappedFile> open_readonly(std::string path);
  static std::shared_ptr<MappedFile> open_readwrite(std::string path);

  std::shared_ptr<Slice> slice(size_t off, size_t len) noexcept;
  std::shared_ptr<const Slice> slice(size_t off, size_t len) const noexcept;

private:
  bool _readonly = false;
  mutable bool _opened = false, _use_fallback = false;
  std::string _path = 0;

#if defined(_WIN32)
  mutable void *_hFile = nullptr;
#elif defined(__unix__) || defined(__APPLE__)
  mutable int _fd = -1;
#endif
  inline void ensure_opened() const {
    if (!_opened || !_use_fallback) open_file();
  }
  // Paltform specific file opening for future memory-mapping of file offsets.
  void open_file() const;
  // Close the underlying file. I'm pretty sure this is only safe to call when no mapped regions are active.
  // So, please only call it from the DTOR.
  void release() noexcept;

  // Returns the size of an OS page so that we can page-align our loads
  static std::size_t page_size();

  // Helpers to reach into the guts of a slice and read/write its contents.
  void load_mapped(Slice &) const;
  void load_fallback(Slice &) const;
  void write_fallback(const Slice &);
  void flush(const Slice &);
};

} // namespace pepp::core
