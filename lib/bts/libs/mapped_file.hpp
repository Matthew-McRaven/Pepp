#pragma once

#include <string>
#include "bts/bitmanip/integers.h"
#include "bts/bitmanip/span.hpp"
namespace pepp::bts {

class MappedFile : public std::enable_shared_from_this<MappedFile> {
  struct private_ctor_tag {};

public:
  class Slice {
  public:
    Slice() = default;
    // Public to only make_shared to work, but should be treated as-if private.
    Slice(private_ctor_tag, std::shared_ptr<MappedFile> file, size_t file_offset, size_t file_len, bool readonly);
    ~Slice();
    Slice(const Slice &) = delete;
    Slice &operator=(const Slice &) = delete;
    Slice(Slice &&o) noexcept = default;
    Slice &operator=(Slice &&o) noexcept;

    bits::span<u8> get() noexcept;
    bits::span<const u8> get() const noexcept;
    size_t size() const noexcept;
    void flush();
    inline bool readonly() const noexcept { return _readonly; }

  private:
    inline void ensure_loaded() const {
      if (!_loaded && _file != nullptr) _file->load_mapped(*const_cast<Slice *>(this));
    }
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
  void open_file() const;
  // Close the underlying file
  void release() noexcept;

  static std::size_t page_size();

  void load_mapped(Slice &) const;
  void load_fallback(Slice &) const;
  void write_fallback(const Slice &);
  void flush(const Slice &);
};

} // namespace pepp::bts
