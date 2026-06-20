#include <catch.hpp>
#include <cmrc/cmrc.hpp>
#include <spdlog/spdlog.h>
#include "core/integers.h"

CMRC_DECLARE(pepp_figures);

u32 enumerate_recurse(const auto &fs, std::string path) {
  u32 ret = 0;
  for (const auto &it : fs.iterate_directory(path)) {
    const auto inner = fmt::format("{}/{}", path, it.filename());
    if (it.is_directory()) {
      SPDLOG_DEBUG("Found dirs: {}", inner);
      ret += enumerate_recurse(fs, inner);
    } else {
      SPDLOG_DEBUG("Found file: {}", inner);
      ret += 1;
    }
  }
  return ret;
}

TEST_CASE("Enumerate CMakeRC books", "[demo]") {
  auto fs = cmrc::pepp_figures::get_filesystem();
  // Figure filesystem should have a large number of files in it.
  CHECK(enumerate_recurse(fs, std::string{""}) > 100);
}
