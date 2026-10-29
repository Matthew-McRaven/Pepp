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
#include <memory>
#include <utility>
#include <variant>
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

const PackedInputElfGroupLE32 &group_of(const LinkResult &result) {
  REQUIRE(std::holds_alternative<std::unique_ptr<PackedInputElfGroupLE32>>(result.group));
  return *std::get<std::unique_ptr<PackedInputElfGroupLE32>>(result.group);
}
} // namespace

TEST_CASE("Link from a LinkConfig", "[scope:core][scope:core.link][kind:unit][arch:*]") {
  SECTION("Inputs from disk and memory") {
    using enum Liveness;
    write_bytes("link_config_a.o", bytes_of({{"main", true}, {"needed", false}, {"also", false}}));
    auto lib = archive_of({{{"needed", true}}}), in_memory = archive_of({{{"also", true}}});
    write_bytes("link_config_lib.a", archive_bytes(lib).value());

    // The class comes from the first input's member. One file is duplicated (link_config_a).
    const auto result = do_link(config_of(path("link_config_lib.a"), path("link_config_a.o"),
                                          to_input_archive(in_memory).value(), object_of({{"other", true}}),
                                          path("link_config_a.o")));
    REQUIRE(result);
    const auto &group = group_of(*result);
    REQUIRE(group.size() == 5);
    CHECK(group.archive_count() == 2);
    CHECK(std::holds_alternative<ElfFromArchive>(group.provenance(ElfFileID{1})));
    CHECK(std::get<ElfFromPath>(group.provenance(ElfFileID{2})).path == "link_config_a.o");
    CHECK(std::holds_alternative<ElfFromArchive>(group.provenance(ElfFileID{3})));
    CHECK(std::holds_alternative<ElfInMemory>(group.provenance(ElfFileID{4})));
    // main's references pull in both members.
    CHECK(result->context.files == std::vector{Excluded, Live, Live, Live, Live, Live});
    const auto errors = result->context.symbols.errors();
    REQUIRE(errors.size() == 1);
    CHECK(errors[0].code == ResolveError::Code::MultipleDefinition);
  }
}
