/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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
 */
#include "core/sim/debugger/tvm_loader_backend.hpp"

namespace tvm {

LoaderBackend::LoaderBackend(std::shared_ptr<pepp::bts::BufferManager> mgr, System *system)
    : ApplyBackend(std::move(mgr), system) {}

void LoaderBackend::set_file(u16 index, const ElfImage *image) {
  if (image == nullptr) _files.erase(index);
  else _files[index] = image;
}

const ElfImage *LoaderBackend::file(u16 index) const {
  if (auto it = _files.find(index); it != _files.end()) return it->second;
  return nullptr;
}

void LoaderBackend::clear_files() {
  _files.clear();
  _context = {};
}

void LoaderBackend::set_context(u16 file, u16 segment) { _context = {.valid = true, .file = file, .segment = segment}; }

} // namespace tvm
