/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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

#include "toolchain/pas/driver/pep10.hpp"
#include <catch.hpp>
#include "toolchain/pas/obj/pep10.hpp"
#include "toolchain/pas/operations/generic/errors.hpp"
#include "toolchain/pas/operations/pepp/string.hpp"
#include "help/builtins/book.hpp"
#include "help/builtins/figure.hpp"
#include "help/builtins/registry.hpp"
#include "isa/pep10.hpp"
#include "toolchain/link/memmap.hpp"
#include "toolchain/link/mmio.hpp"
#include "toolchain/macro/registry.hpp"
#include "sim/device/simple_bus.hpp"
#include "targets/isa3/system.hpp"

namespace {

static const auto is_charIn = [](const auto &x) {
  return x.name == "charIn" && x.type == obj::IO::Type::kInput && x.minOffset == 0xFFFD && x.maxOffset == 0xFFFD;
};
static const auto is_charOut = [](const auto &x) {
  return x.name == "charOut" && x.type == obj::IO::Type::kOutput && x.minOffset == 0xFFFE && x.maxOffset == 0xFFFE;
};
static const auto is_pwrOff = [](const auto &x) {
  return x.name == "pwrOff" && x.type == obj::IO::Type::kOutput && x.minOffset == 0xFFFF && x.maxOffset == 0xFFFF;
};

void loadBookMacros(QSharedPointer<const builtins::Book> book, QSharedPointer<macro::Registry> registry) {
  for (auto &macro : book->macros()) registry->registerMacro(macro::types::Core, macro);
}

void injectFakeSCallMacros(QSharedPointer<macro::Registry> registry) {
  static const QStringList nonunary = {"DECI", "CHARI", "CHARO", "STRO", "DECO", "PRINTF"};
  for (auto &macro : nonunary)
    registry->registerMacro(macro::types::Core,
                            QSharedPointer<macro::Parsed>::create(macro, 2, "LDWA 0,i\nSCALL $1, $2", "pep/10"));
}
} // namespace

TEST_CASE("CS6E figure assembly", "[scope:asm][kind:e2e][arch:pep10]") {
  auto book_registry = builtins::Registry(nullptr);
  auto book = book_registry.findBook("Computer Systems, 6th Edition");
  SECTION("Standalone") {
    // Load macros on each iteration to prevent macros from migrating between
    // tests.
    auto registry = QSharedPointer<macro::Registry>::create();
    REQUIRE_FALSE(book.isNull());
    for (auto &fig : book->figures()) {
      if (!fig->typesafeElements().contains("pep")) continue;
      QString chapter = fig->chapterName();
      QString figName = fig->figureName();
      QString body = fig->typesafeElements()["pep"]->contents;
      bool isOS = fig->isOS();
      DYNAMIC_SECTION(chapter.toStdString() << "." << figName.toStdString()) {
        loadBookMacros(book, registry);
        if (!isOS) injectFakeSCallMacros(registry);

        auto pipeline = pas::driver::pep10::pipeline<pas::driver::ANTLRParserTag>(
            {{body, {.isOS = isOS, .ignoreUndefinedSymbols = !isOS}}}, registry);
        auto result = pipeline->assemble(pas::driver::pep10::Stage::End);
        auto target = pipeline->pipelines[0].first;
        REQUIRE(target->bodies.contains(pas::driver::repr::Nodes::name));
        auto root = target->bodies[pas::driver::repr::Nodes::name].value<pas::driver::repr::Nodes>().value;
        // print out error messages before failing -- enables debugging broken
        // tests.
        if (!result) {
          auto errors = pas::ops::generic::collectErrors(*root);
          for (auto &error : errors) qCritical() << error.first.value.line << error.second.message;
        }

        REQUIRE(result);
      }
    }
  }
  SECTION("Unified") {
    // Load macros on each iteration to prevent macros from migrating between
    // tests.
    auto registry = QSharedPointer<macro::Registry>::create();
    REQUIRE_FALSE(book.isNull());
    loadBookMacros(book, registry);
    for (auto &fig : book->figures()) {
      auto defaultOS = fig->defaultOS();
      if (!fig->typesafeElements().contains("pep")) continue;
      else if (fig->isOS()) continue;
      else if (defaultOS == nullptr) continue;
      else if (!defaultOS->typesafeElements().contains("pep")) continue;
      QString chapter = fig->chapterName();
      QString figName = fig->figureName();
      QString osBody = defaultOS->typesafeElements()["pep"]->contents;
      QString userBody = fig->typesafeElements()["pep"]->contents;
      bool isFullOS = bool(defaultOS->figureName() == "full");

      DYNAMIC_SECTION(chapter.toStdString() << "." << figName.toStdString()) {
        auto pipeline = pas::driver::pep10::pipeline<pas::driver::ANTLRParserTag>(
            {{osBody, {.isOS = true, .ignoreUndefinedSymbols = false}},
             {userBody, {.isOS = false, .ignoreUndefinedSymbols = false}}},
            registry);
        auto result = pipeline->assemble(pas::driver::pep10::Stage::End);

        CHECK(pipeline->pipelines.size() == 2);

        auto osTarget = pipeline->pipelines[0].first;
        REQUIRE(osTarget->bodies.contains(pas::driver::repr::Nodes::name));
        auto osRoot = osTarget->bodies[pas::driver::repr::Nodes::name].value<pas::driver::repr::Nodes>().value;
        if (pipeline->pipelines[0].first->stage != pas::driver::pep10::Stage::End) {
          for (auto &error : pas::ops::generic::collectErrors(*osRoot)) qCritical() << "OS:   " << error.second.message;
          REQUIRE(false);
        }
        auto userTarget = pipeline->pipelines[1].first;
        REQUIRE(userTarget->bodies.contains(pas::driver::repr::Nodes::name));
        auto userRoot = userTarget->bodies[pas::driver::repr::Nodes::name].value<pas::driver::repr::Nodes>().value;
        // print out error messages before failing -- enables debugging broken
        // tests.
        if (!result) {
          auto lines = pas::ops::pepp::formatListing<isa::Pep10>(*userRoot);
          for (auto &line : lines) qCritical() << line;
          for (auto &error : pas::ops::generic::collectErrors(*osRoot)) qCritical() << "OS:   " << error.second.message;
          for (auto &error : pas::ops::generic::collectErrors(*userRoot))
            qCritical() << "USER: " << error.second.message;
        }
        auto elf = pas::obj::pep10::createElf();
        pas::obj::pep10::combineSections(*osRoot);
        pas::obj::pep10::writeOS(*elf, *osRoot);
        pas::obj::pep10::combineSections(*userRoot);
        pas::obj::pep10::writeUser(*elf, *userRoot);
        // Segments are not layed out correctly until saving.
        {
          std::stringstream s;
          elf->save(s);
          s.seekg(0, std::ios::beg);
          elf->load(s);
        }
        REQUIRE(result);

        // Verify MMIO information.
        auto decs = ::obj::getMMIODeclarations(*elf);
        CHECK(decs.length() == 3);
        CHECK(std::find_if(decs.cbegin(), decs.cend(), is_charIn) != decs.cend());
        CHECK(std::find_if(decs.cbegin(), decs.cend(), is_charOut) != decs.cend());
        CHECK(std::find_if(decs.cbegin(), decs.cend(), is_pwrOff) != decs.cend());

        auto memMap = obj::getLoadableSegments(*elf);
        auto mergeMap = obj::mergeSegmentRegions(memMap);
        if (isFullOS) {
          CHECK(mergeMap.size() == 3);
          // user memory + system stack
          mergeMap[0].segs = {};
          auto uMem = obj::MemoryRegion{.r = 1, .w = 1, .minOffset = 0, .maxOffset = 0xfaf9};
          CHECK(mergeMap[0] == uMem);
          // OS text
          mergeMap[1].segs = {};
          auto txt = obj::MemoryRegion{.r = 1, .w = 0, .minOffset = 0xfafa, .maxOffset = 0xfff9};
          CHECK(mergeMap[1] == txt);
          // Carveout for MMIO
          mergeMap[2].segs = {};
          auto mmio = obj::MemoryRegion{.r = 1, .w = 1, .minOffset = 0xfffa, .maxOffset = 0xffff};
          CHECK(mergeMap[2] == mmio);
        }
        QSharedPointer<targets::isa::System> sys;
        REQUIRE_NOTHROW([&sys, &elf]() { sys = targets::isa::systemFromElf(*elf, true); }());
        QVector<quint8> dump(0x1'00'00);
        sys->bus()->dump({dump.data(), std::size_t(dump.size())});
      }
    }
  }
}
