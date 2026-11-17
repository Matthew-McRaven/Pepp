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
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "core/integers.h"
#include "core/math/bitmanip/span.hpp"
namespace pepp::bts {

// A file viewed through slices. The static factories map the whole file into memory if the platform supports it and the
// file opens. Otherwise, they return a BufferedFile, and throw if it cannot open the file either. Slices may overlap,
// since they all view the same data.
class MappedFile : public std::enable_shared_from_this<MappedFile> {
protected:
  struct private_ctor_tag {};

public:
  // A span of a MappedFile's data.
  class Slice {
  public:
    virtual ~Slice() = default;
    Slice(const Slice &) = delete;
    Slice &operator=(const Slice &) = delete;

    // The length passed to slice(). get() returns fewer bytes if the slice extends past the end of the file.
    virtual size_t size() const noexcept = 0;
    virtual bool readonly() const noexcept = 0;
    virtual bits::span<u8> get() noexcept = 0;
    virtual bits::span<const u8> get() const noexcept = 0;
    // Write changes to the file. Does nothing for a readonly or anonymous file.
    virtual void flush() = 0;

  protected:
    Slice() = default;
  };

  virtual ~MappedFile() = default;
  MappedFile(const MappedFile &) = delete;
  MappedFile &operator=(const MappedFile &) = delete;

  static std::shared_ptr<MappedFile> open_readonly(std::string path);
  // Create the file if it does not exist. If size is given, resize the file to size bytes.
  static std::shared_ptr<MappedFile> open_readwrite(std::string path, std::optional<u64> size = std::nullopt);
  // Replace the file's contents with size zero bytes.
  static std::shared_ptr<MappedFile> open_truncated(std::string path, u64 size);
  // Create a fixed-size, zeroed allocation not backed by a file. flush() is a no-op.
  static std::shared_ptr<MappedFile> open_anonymous(size_t size);

  // Returns true if slices view a memory mapping, or false if they view a BufferedFile's buffer.
  virtual bool memory_mapped() const noexcept = 0;
  // Byte size of the file when it was opened, or of the anonymous allocation.
  virtual u64 size() const = 0;
  virtual std::shared_ptr<Slice> slice(size_t off, size_t len) noexcept = 0;
  virtual std::shared_ptr<const Slice> slice(size_t off, size_t len) const noexcept = 0;

protected:
  MappedFile() = default;

private:
  enum class Mode : u8 { ReadOnly, ReadWrite, Truncate, Anonymous };
  // Open the file at path with the platform's memory mapping. For an Anonymous file, path is empty and size is
  // required. Returns nullptr if the platform cannot map memory, or the file cannot be opened, resized, or mapped.
  //
  // Implement this method in your _<platform>.cpp file, and update the gaurd conditions of the default implementation
  // in mapped_file.cpp to avoid linker errors.
  static std::shared_ptr<MappedFile> open_platform(const std::string &path, Mode mode, std::optional<u64> size);
};

// A fallback implementation which keeps the entire buffer in memory as a vector.
// Used on platforms without memory-mapping or when the platform's memory-mapped file cannot be opened or mapped.
class BufferedFile final : public MappedFile {
public:
  // Public only for make_shared. path is empty for an anonymous file.
  BufferedFile(private_ctor_tag, std::string path, std::vector<u8> data, bool readonly);

  static std::shared_ptr<BufferedFile> open_readonly(std::string path);
  // Create the file if it does not exist. If size is given, resize the file to size bytes before reading it.
  static std::shared_ptr<BufferedFile> open_readwrite(std::string path, std::optional<u64> size = std::nullopt);
  // Replace the file's contents with size zero bytes.
  static std::shared_ptr<BufferedFile> open_truncated(std::string path, u64 size = 0);
  static std::shared_ptr<BufferedFile> open_anonymous(size_t size);

  bool memory_mapped() const noexcept override { return false; }
  u64 size() const override;
  std::shared_ptr<Slice> slice(size_t off, size_t len) noexcept override;
  std::shared_ptr<const Slice> slice(size_t off, size_t len) const noexcept override;

private:
  class BufferedSlice;
  // Write length bytes of the buffer at offset to the file.
  void write_back(size_t offset, size_t length) const;
  std::string _path;
  bool _readonly = false;
  std::vector<u8> _data;
};

} // namespace pepp::bts
