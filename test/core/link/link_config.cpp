/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 *
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

#include "core/link/link_config.hpp"
#include <catch.hpp>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>
#include "./helpers.hpp"
#include "core/link/linker_passes.hpp"

namespace {
using namespace link_test;
using std::filesystem::path;

LinkConfig config_of(auto &&...inputs) {
  LinkConfig ret;
  (ret.inputs.push_back({std::forward<decltype(inputs)>(inputs)}), ...);
  return ret;
}

std::vector<std::string> messages_of(const LinkContext &context) {
  std::vector<std::string> ret;
  for (const auto &diagnostic : context.diagnostics) ret.push_back(describe(context, diagnostic));
  return ret;
}
} // namespace

TEST_CASE("e2e linking", "[scope:core][scope:core.link][kind:unit][arch:*]") {
  using enum SymbolBinding;
  using enum SymbolType;
  write_bytes("link_config_a.o", bytes_of({{"main", true}, {"needed", false}, {"also", false}}));
  write_bytes("link_config_lib.a", archive_bytes_of({{{"needed", true}, {"missing", false}}}));
  auto in_memory = archive_of({{{"also", true}}});

  // The class comes from the first input's member. a.o is linked twice.
  const auto result = do_link(config_of(path("link_config_lib.a"), path("link_config_a.o"),
                                        to_input_archive(in_memory).value(),
                                        object_of({{"t", true, STB_GLOBAL, STT_TLS}}),
                                        object_of({{"t", false, STB_GLOBAL, STT_OBJECT}}), path("link_config_a.o")));
  // non-fatal errors do not stop link
  CHECK(result.diagnostics.count(Severity::Fatal) == 0);
  CHECK(messages_of(result) ==
        std::vector<std::string>{
            "link_config_lib.a(0.o): undefined reference to `missing'",
            "t: TLS definition in <memory> mismatches non-TLS reference in <memory>",
            "link_config_a.o: multiple definition of `main'; link_config_a.o: first defined here"});

  // A fatal error stops the link.
  const auto missing = do_link(config_of(object_of({{"a", true}}), path("link_config_missing.o")));
  CHECK(entries_of(missing.diagnostics) ==
        std::vector<Diagnostic>{{Severity::Fatal, InputGroupError::Code::CannotOpen, ConfigInputID{1}}});
  CHECK(messages_of(missing) == std::vector<std::string>{"link_config_missing.o: cannot open file"});
  CHECK(messages_of(do_link(config_of(path("link_config_missing.o")))) ==
        std::vector<std::string>{"link_config_missing.o: cannot open file"});
  CHECK(messages_of(do_link(config_of())) == std::vector<std::string>{"no ELF input files"});
}
