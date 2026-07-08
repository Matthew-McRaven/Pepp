/*
 * Copyright (c) 2024-2026 J. Stanley Warford, Matthew McRaven
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

#include "./interactive.hpp"
#include <catch.hpp>
#include <iostream>
#include "core/interactive_test/dict.hpp"
#include "core/interactive_test/interp.hpp"
#include "core/interactive_test/objheap.hpp"
#include "core/interactive_test/vocab/core_words.hpp"
#include "core/sim/system.hpp"
#include "fmt/format.h"
#include "fmt/ranges.h"

InteractiveTask::InteractiveTask(QObject *parent) : Task(parent) {}

std::vector<std::string_view> tokenize(std::string_view input) {
  constexpr std::string_view ws = " \t\r\n";
  std::vector<std::string_view> out;
  size_t start = input.find_first_not_of(ws);
  while (start != std::string_view::npos) {
    size_t end = input.find_first_of(ws, start);
    if (end == std::string_view::npos) end = input.size();
    out.push_back(input.substr(start, end - start));
    start = input.find_first_not_of(ws, end);
  }
  return out;
}

struct Context {
  std::stack<std::unique_ptr<System>> _systems;
};

using Handler = std::function<bool(Context &, std::span<std::string_view>)>;

bool hnd_sys_dev_count(Context &ctx, std::span<std::string_view> args) {
  if (ctx._systems.empty()) {
    std::cerr << "  No system created yet." << std::endl;
    return false;
  }
  auto &sys = ctx._systems.top();
  auto dt = sys->root();
  auto count = std::distance(dt->begin(), dt->end());
  std::cout << "  System has " << count << " devices." << std::endl;
  return true;
}
// Many name=values
bool hnd_create_dense(Context &ctx, std::span<std::string_view> args) {
  if (ctx._systems.empty()) {
    std::cerr << "  No system created yet." << std::endl;
    return false;
  }
  std::cout << fmt::format("  {}", fmt::join(args.begin(), args.end(), ",")) << std::endl;

  return true;
}

static const inline NativeOpcode make_machine{.name = "sys.create", .h = [](Interpreter *interp) {
                                                // interp->push_object((void)nullptr);
                                              }};

void InteractiveTask::run() {
  std::cout << "Interactive mode. Type something and press enter (Ctrl+D to exit):" << std::endl;
  std::string input;
  Interpreter p;
  register_common_words(&p);
  register_native_heap_fns(&p);
  std::cout.flush();
  std::cerr.flush();

  // Force all definitions to be preloaded prior to start REPL.
  p.buffered.clear();
  p.run_on("");
  p.input_source = std::make_unique<StdinInput>();
  p.run();
  return emit finished(0);
}
