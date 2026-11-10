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

#include "ld.hpp"
#include <charconv>
#include <fmt/format.h>
#include "core/link/linker_passes.hpp"

std::optional<std::pair<std::string, u64>> parse_section_start(std::string_view arg) {
  const auto eq = arg.find('=');
  if (eq == arg.npos || eq == 0) return std::nullopt;
  auto address = arg.substr(eq + 1);
  if (address.starts_with("0x") || address.starts_with("0X")) address.remove_prefix(2);
  u64 value = 0;
  const auto end = address.data() + address.size();
  if (const auto [ptr, ec] = std::from_chars(address.data(), end, value, 16); ec != std::errc{} || ptr != end)
    return std::nullopt;
  return std::pair{std::string(arg.substr(0, eq)), value};
}

LdTask::LdTask(pepp::link::LinkConfig &config, QObject *parent) : Task(parent), _config(config) {}

void LdTask::run() {
  using pepp::link::Severity;
  const auto context = pepp::link::do_link(std::move(_config));
  for (const auto &diagnostic : context.diagnostics)
    fmt::print("ld: {}{}\n", diagnostic.severity == Severity::Warning ? "warning: " : "",
               pepp::link::describe(context, diagnostic));
  return emit finished(context.ok() ? 0 : 1);
}
