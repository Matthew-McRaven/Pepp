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
#include "core/interactive_test/core_words.hpp"
#include "core/interactive_test/dict.hpp"
#include "core/interactive_test/interp.hpp"
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

bool hnd_create_system(Context &ctx, std::span<std::string_view> args) {
  ctx._systems.push(std::make_unique<System>());
  return true;
}

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

std::unordered_map<std::string, Handler> _commands = {
    {"create_sys", hnd_create_system},
    {"sys_dev_count", hnd_sys_dev_count},
    {"create_dense", hnd_create_dense},
    {"help",
     [](Context &ctx, std::span<std::string_view> args) {
       std::cout << "Available commands:" << std::endl;
       for (const auto &[cmd, _] : _commands) std::cout << "  " << cmd << std::endl;
       return true;
     }},
};

void InteractiveTask::run() {
  std::cout << "Interactive mode. Type something and press enter (Ctrl+D to exit):" << std::endl;
  std::string input;
  Interpreter p;
  auto op_dup16 = p.register_native(native_dup16);
  auto op_add16i = p.register_native(native_add16i);
  auto op_dot = p.register_native([](Interpreter *i) {
    auto tos = i->pop_psp<i16>();
    std::cout << tos;
  });
  auto op_stop = p.register_native(native_halt);
  auto op_docol = p.register_native(native_docol);
  auto op_exitcol = p.register_native(native_exitcol);
  auto op_push_num = p.register_native([](Interpreter *i) { i->push_psp<i16>(7); });

  auto d_dup = dict_insert(&p, "dup", {}, {std::array<u16, 1>{op_dup16}});
  auto d_add16i = dict_insert(&p, "+", {}, {std::array<u16, 1>{op_add16i}});
  auto d_dot = dict_insert(&p, ".", {}, {std::array<u16, 1>{op_dot}});
  auto d_stop = dict_insert(&p, "STOP", {}, {std::array<u16, 1>{op_stop}});
  auto d_docol = dict_insert(&p, "docol", {}, {std::array<u16, 1>{op_docol}});
  auto d_exit = dict_insert(&p, "exit", {}, {std::array<u16, 1>{op_exitcol}});
  auto d_push_num = dict_insert(&p, "pushd", {}, {std::array<u16, 1>{op_push_num}});
  auto d_14 = dict_insert(&p, "14", {}, std::array<u16, 5>{d_push_num.cfa(), d_dup.cfa(), d_add16i.cfa(), d_exit.cfa()},
                          d_docol.cfa() + 2);
  auto d_28 = dict_insert(&p, "28", {}, std::array<u16, 5>{d_14.cfa(), d_14.cfa(), d_add16i.cfa(), d_exit.cfa()},
                          d_docol.cfa() + 2);
  auto d_42 = dict_insert(&p, "42", {}, std::array<u16, 5>{d_14.cfa(), d_28.cfa(), d_add16i.cfa(), d_exit.cfa()},
                          d_docol.cfa() + 2);
  auto b = begin(&p), e = end(&p);
  while (b != e) {
    auto v = *b;
    std::cout << fmt::format("{:9}: 0x{:04x}\n", v.name(), b.link());
    std::cout << "  " << (u16)v.strlen_flags() << std::endl;
    std::cout << fmt::format(" 0x{:04x}\n", (i16)v.codeword());
    b++;
  }
  std::array<u16, 3> code = {d_42.cfa(), d_dot.cfa(), d_stop.cfa()};
  p.write(0, code);
  p.run();
  return emit finished(0);
  while (std::getline(std::cin, input)) {
    if (input.empty()) break;
    // Copy into p
    // auto r = p.eval(input);
    // if (r.flow == tcl::ControlFlow::FERROR) std::cerr << "  Error: " << r.value << std::endl;
    // else if (!r.value.empty()) std::cout << r.value << std::endl;
  }
}
