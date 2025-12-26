/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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

#include "addr2line.hpp"
#include <elfio/elfio.hpp>
#include <elfio/elfio_dump.hpp>
#include <iostream>
#include "help/builtins/figure.hpp"
#include "toolchain/helpers/asmb.hpp"
#include "toolchain/pas/obj/common.hpp"

Addr2LineTask::Addr2LineTask(Options &opts, QObject *parent) : Task(parent), _opts(opts) {}

void Addr2LineTask::run() {
  using namespace Qt::StringLiterals;
  auto elf = ELFIO::elfio{};

  if (!elf.load(_opts.elffile)) {
    qWarning() << "Failed to open elf file: " << _opts.elffile;
    return emit finished(1);
  }
  using namespace Qt::StringLiterals;
  auto linemaps = pas::obj::common::getLineMappings(elf);
  std::sort(linemaps.begin(), linemaps.end());
  for (const auto address : _opts.addresses) {
    auto lm = std::find_if(linemaps.cbegin(), linemaps.cend(), [address](auto in) { return in.address == address; });
    if (lm != linemaps.cend()) {
      if (_opts.listing_numbers) std::cout << fmt::format("{:04x} => {}", address, lm->listLine) << std::endl;
      else std::cout << fmt::format("{:04x} => {}", address, lm->srcLine) << std::endl;

    } else std::cout << fmt::format("{:04x} => ??", address) << std::endl;
  }

  return emit finished(0);
}
