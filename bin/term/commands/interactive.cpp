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

  // opcodes without dict entries.
  auto op_stop = p.register_native(native_halt);

  auto op_dup16 = p.register_native(native_dup16);
  auto d_dup = dict_insert(&p, "dup", {}, {std::array<u16, 1>{op_dup16}});

  auto op_add16i = p.register_native(native_add16i);
  auto d_add16i = dict_insert(&p, "+", {}, {std::array<u16, 1>{op_add16i}});

  auto op_dot = p.register_native([](Interpreter *i) {
    auto tos = i->pop_psp<i16>();
    std::cout << tos;
  });
  auto d_dot = dict_insert(&p, ".", {}, {std::array<u16, 1>{op_dot}});

  auto op_docol = p.register_native(native_docol);
  auto d_docol = dict_insert(&p, "docol", {}, {std::array<u16, 1>{op_docol}});
  auto op_exitcol = p.register_native(native_exitcol);
  auto d_exit = dict_insert(&p, "exit", {}, {std::array<u16, 1>{op_exitcol}});

  const u16 spad = p.cb.here;
  p.cb.here += 32;
  auto op_word = p.register_native([&spad](Interpreter *i) { native_word(i, spad); });
  auto d_word = dict_insert(&p, "word", {}, {std::array<u16, 1>{op_word}});

  auto op_create = p.register_native(native_create);
  auto d_create = dict_insert(&p, "create", {}, {std::array<u16, 1>{op_create}});
  auto op_comma = p.register_native(native_comma);
  auto d_comma = dict_insert(&p, ",", {}, {std::array<u16, 1>{op_comma}});
  auto op_lit = p.register_native(native_lit);
  auto d_lit = dict_insert(&p, "lit", {}, {std::array<u16, 1>{op_lit}});

  auto op_latest = p.register_native(native_latest);
  auto d_latest = dict_insert(&p, "latest", {}, {std::array<u16, 1>{op_latest}});

  auto op_fetch = p.register_native(native_fetch);
  auto d_fetch = dict_insert(&p, "@", {}, {std::array<u16, 1>{op_fetch}});

  auto op_hidden = p.register_native(native_hidden);
  auto d_hidden = dict_insert(&p, "hidden", {}, {std::array<u16, 1>{op_hidden}});

  auto op_rbrac = p.register_native(native_rbrac);
  auto d_rbrac = dict_insert(&p, "]", Flags::IMMEDIATE, {std::array<u16, 1>{op_rbrac}});
  auto op_lbrac = p.register_native(native_lbrac);
  auto d_lbrac = dict_insert(&p, "[", {}, {std::array<u16, 1>{op_lbrac}});

  auto op_colon = std::array<u16, 9>{d_word.cfa(),   d_create.cfa(), d_lit.cfa(),   d_docol.cfa(), d_comma.cfa(),
                                     d_latest.cfa(), d_hidden.cfa(), d_rbrac.cfa(), d_exit.cfa()};
  auto d_colon = dict_insert(&p, ":", {}, op_colon);
  auto op_semi = std::array<u16, 7>{d_lit.cfa(),    d_exit.cfa(),  d_comma.cfa(), d_latest.cfa(),
                                    d_hidden.cfa(), d_lbrac.cfa(), d_exit.cfa()};
  auto d_semi = dict_insert(&p, ";", Flags::IMMEDIATE, op_semi);

  auto op_rspinitval = p.register_native([](Interpreter *i) { i->push_psp(Interpreter::INITIAL_RSP); });
  auto d_rspinit = dict_insert(&p, "r0", {}, {std::array<u16, 1>{op_rspinitval}});
  auto op_rspstore = p.register_native([](Interpreter *i) { i->cb.rsp = i->pop_psp<u16>(); });
  auto d_rspstore = dict_insert(&p, "rsp!", {}, {std::array<u16, 1>{op_rspstore}});

  auto op_branch = p.register_native(native_branch);
  auto d_branch = dict_insert(&p, "branch", {}, {std::array<u16, 1>{op_branch}});
  auto op_interp = p.register_native(native_interpret);
  auto d_interp = dict_insert(&p, "interpret", {}, {std::array<u16, 1>{op_interp}});

  auto op_quit = std::array<u16, 5>{d_rspinit.dfa(), d_rspstore.dfa(), d_interp.dfa(), d_branch.dfa(), (u16)-8};
  auto d_quit = dict_insert(&p, "quit", {}, op_quit);

  auto b = begin(&p), e = end(&p);
  while (b != e) {
    auto v = *b;
    std::cout << fmt::format("{:9}: 0x{:04x}\n", v.name(), b.link());
    std::cout << "  " << (u16)v.strlen_flags() << std::endl;
    std::cout << fmt::format("  0x{:04x}\n", (i16)v.codeword());
    b++;
  }
  p.write(d_quit.dfa(), 0);
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
