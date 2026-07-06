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

  // Words with automatic registration.
  auto h_stop = dict_insert_native(&p, Halt, {});
  auto h_dup = dict_insert_native(&p, Dup16, {});
  auto h_drop = dict_insert_native(&p, Drop16, {});
  auto h_add16i = dict_insert_native(&p, Add16i, {}, "+");
  auto h_dot = dict_insert_native(&p, Dot, {}, ".");
  auto h_docol = dict_insert_native(&p, Docol, {}, "docol");
  auto h_exit = dict_insert_native(&p, Exitcol, {});
  auto h_create = dict_insert_native(&p, Create, {});
  auto h_comma = dict_insert_native(&p, Comma, {});
  auto h_lit = dict_insert_native(&p, Lit, {});
  auto h_latest = dict_insert_native(&p, Latest, {});
  auto h_fetch = dict_insert_native(&p, Fetch, {}, "@");
  auto h_hidden = dict_insert_native(&p, Hidden, {});
  auto h_rbrac = dict_insert_native(&p, Rbrac, {}, "]");
  auto h_lbrac = dict_insert_native(&p, Lbrac, Flags::IMMEDIATE, "[");
  auto h_branch = dict_insert_native(&p, Branch, {});
  auto h_lateststore = dict_insert_native(&p, LatestStore, {}, "latest!");
  auto h_rspinitval = dict_insert_native(&p, RspInitVal, {});
  auto h_rspstore = dict_insert_native(&p, RspStoreVal, {});
  auto h_dumpdict = dict_insert_native(&p, DumpDict, {});
  auto h_toggledebug = dict_insert_native(&p, ToggleDebug, {});

  // Word which require per-instance state.
  const u16 spad = p.cb.here;
  p.cb.here += 32;
  NativeOpcode Word = {
      .stack_delta = 4,
      .name = "word",
      .h = [&spad](Interpreter *i) { native_word(i, spad); },
  };
  auto h_word = dict_insert_native(&p, Word, {}, "word");

  NativeOpcode Interp = {
      .stack_delta = 0,
      .name = "interpret",
      .h = [&spad, &h_lit](Interpreter *i) { native_interpret(i, spad, h_lit.pcode()); },
  };
  auto h_interp = dict_insert_native(&p, Interp, {}, "interpret");

  // "FORTH" words, implemented in terms of docol
  auto op_colon =
      std::array<u16, 10>{h_docol.code0(), h_word.pcode(),  h_create.pcode(), h_hidden.pcode(), h_lit.pcode(),
                          h_docol.pcode(), h_fetch.pcode(), h_comma.pcode(),  h_rbrac.pcode(),  h_exit.pcode()};
  auto d_colon = dict_insert(&p, ":", {}, op_colon);
  auto op_semi = std::array<u16, 8>{h_docol.code0(),  h_lit.pcode(),    h_exit.pcode(),  h_comma.pcode(),
                                    h_latest.pcode(), h_hidden.pcode(), h_lbrac.pcode(), h_exit.pcode()};
  auto d_semi = dict_insert(&p, ";", Flags::IMMEDIATE, op_semi);

  auto op_quit = std::array<u16, 6>{h_docol.code0(),  h_rspinitval.pcode(), h_rspstore.pcode(),
                                    h_interp.pcode(), h_branch.pcode(),     (u16)-10};
  auto d_quit = dict_insert(&p, "quit", {}, op_quit);

  p.write(d_quit.pcode(), 0);
  std::cout.flush();
  std::cerr.flush();
  p.run();
  return emit finished(0);
}
