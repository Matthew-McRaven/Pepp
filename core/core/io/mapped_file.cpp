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

#include "core/io/mapped_file.hpp"

// mapped_file_posix.cpp and mapped_file_win32.cpp define
#if !defined(_WIN32) && !((defined(__unix__) || defined(__APPLE__)) && !defined(__EMSCRIPTEN__))
// Emscripten's mmap() emulation backs MAP_SHARED mappings with a full WASM linear-memory page and writes the whole page
// back to the file on unmap/msync, silently inflating small files to 65536 bytes.
std::shared_ptr<pepp::bts::MappedFile> pepp::bts::MappedFile::open_platform(const std::string &, Mode,
                                                                            std::optional<u64>) {
  return nullptr;
}
#endif

std::shared_ptr<pepp::bts::MappedFile> pepp::bts::MappedFile::open_readonly(std::string path) {
  if (auto ret = open_platform(path, Mode::ReadOnly, std::nullopt)) return ret;
  return BufferedFile::open_readonly(std::move(path));
}

std::shared_ptr<pepp::bts::MappedFile> pepp::bts::MappedFile::open_readwrite(std::string path,
                                                                             std::optional<u64> size) {
  if (auto ret = open_platform(path, Mode::ReadWrite, size)) return ret;
  return BufferedFile::open_readwrite(std::move(path), size);
}

std::shared_ptr<pepp::bts::MappedFile> pepp::bts::MappedFile::open_truncated(std::string path, u64 size) {
  if (auto ret = open_platform(path, Mode::Truncate, size)) return ret;
  return BufferedFile::open_truncated(std::move(path), size);
}

std::shared_ptr<pepp::bts::MappedFile> pepp::bts::MappedFile::open_anonymous(size_t size) {
  if (auto ret = open_platform({}, Mode::Anonymous, size)) return ret;
  return BufferedFile::open_anonymous(size);
}
