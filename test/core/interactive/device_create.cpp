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
#include <catch.hpp>
#include "core/interactive_test/hostobjs/vocab.hpp"
#include "core/interactive_test/interp.hpp"
#include "core/interactive_test/vocab/core_words.hpp"
#include "core/sim/api/memory.hpp"

TEST_CASE("REPL -- simulator & device mangament", "[scope:core][scope:core.repl][kind:unit][arch:*]") {

  SECTION("Create a dense RAM device") {
    const std::string input = "cfg.alloc root sys.alloc\n"
                              "cfg.alloc ram,dense\n"
                              "cfg.set min_offset 0\n"
                              "cfg.set max_offset 0xfeed\n"
                              "cfg.set basename test\n"
                              "dev.alloc dev.id .\n"
                              "dev.fullname\n"
                              ". print0\n";
    Interpreter p;
    register_common_words(&p);
    register_devicemgmt_words(&p);
    p.output = std::make_unique<BufferedOutput>();
    auto out = static_cast<BufferedOutput *>(p.output.get());
    p.run_on(input);
    CHECK(out->buffer.size() == 3);
    CHECK(out->buffer[0].starts_with("1"));
    CHECK(out->buffer[1].starts_with("5"));
    CHECK(out->buffer[2].starts_with("/test"));
  }
  SECTION("Read RAM device") {
    const std::string input = "cfg.alloc root sys.alloc\n"
                              "cfg.alloc ram,dense\n"
                              "cfg.set min_offset 0\n"
                              "cfg.set max_offset 100\n"
                              "cfg.set basename test\n"
                              "cfg.set fill 15\n"
                              "dev.alloc\n";
    Interpreter p;

    // Initialize a memory device that we can fill
    register_common_words(&p);
    register_devicemgmt_words(&p);
    p.output = std::make_unique<BufferedOutput>();
    auto out = static_cast<BufferedOutput *>(p.output.get());
    p.run_on(input);

    // Access underlying memory object, and fill the addresses that we will try to read.
    auto obj = p.get_object(4);
    CHECK(obj != nullptr);
    CHECK(obj->type_code() == AValue::Type::Device);
    auto dev = std::dynamic_pointer_cast<DeviceValue>(obj);
    CHECK(dev != nullptr);
    REQUIRE(dev->dev != nullptr);
    auto tgt = dev->dev->capability<Target>();
    REQUIRE(tgt != nullptr);
    std::vector<u8> buffer({9, 10, 11, 12, 13, 14, 15, 16});
    tgt->write(9, buffer, {});

    // Then try and read consecutive integers
    // t2 should hold {10..14}
    p.run_on("10 5 t2 tgt.read16 t2 .\n");
    CHECK(out->buffer.size() == 1);
    auto int_as_str = out->buffer[0];
    u16 value = std::stoi(int_as_str);
    CHECK(value != 0);
    auto span = p.memspan(value, 6);
    CHECK(span[0] == 10);
    CHECK(span[1] == 11);
    CHECK(span[2] == 12);
    CHECK(span[3] == 13);
    CHECK(span[4] == 14);
    CHECK(span[5] != 15);
  }
}
