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
#include <fmt/format.h>
#include "core/link/linker_passes.hpp"

LdTask::LdTask(pepp::link::LinkConfig &config, QObject *parent) : Task(parent), _config(config) {}

void LdTask::run() {
  using pepp::link::Severity;
  if (!_config.output) _config.output = "a.out";
  const auto context = pepp::link::do_link(std::move(_config));
  for (const auto &diagnostic : context.diagnostics)
    fmt::print("ld: {}{}\n", diagnostic.severity == Severity::Warning ? "warning: " : "",
               pepp::link::describe(context, diagnostic));
  return emit finished(context.ok() ? 0 : 1);
}
