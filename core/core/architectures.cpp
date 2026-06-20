#include "architectures.hpp"
#include <map>
#include "core/ds/string_compare.hpp"
#include "fmt/ranges.h"
#include "spdlog/spdlog.h"

struct ArchData {
  std::string basic, pretty;
};

std::map<pepp::Architecture, ArchData> init_archs() {
  using namespace pepp;
  std::map<pepp::Architecture, ArchData> m{};
  m[Architecture::PEP8] = {"PEP8", "Pep/8"};
  m[Architecture::PEP9] = {"PEP9", "Pep/9"};
  m[Architecture::PEP10] = {"PEP10", "Pep/10"};
  m[Architecture::RISCV] = {"RISCV", "RISC-V"};
  return m;
}
static const auto archs = init_archs();

bool pepp::is_valid_arch(Architecture arch) noexcept { return archs.find(arch) != archs.end(); }

bool pepp::is_valid_arch(int arch) noexcept { return is_valid_arch(static_cast<Architecture>(arch)); }

std::string pepp::arch_as_string(Architecture architecture) {
  if (auto it = archs.find(architecture); it != archs.end()) return it->second.basic;
  SPDLOG_WARN("Unknown architecture in arch_as_string: {}", static_cast<int>(architecture));
  return "Unknown";
}

std::string pepp::arch_as_pretty_string(Architecture architecture) {
  if (auto it = archs.find(architecture); it != archs.end()) return it->second.pretty;
  SPDLOG_WARN("Unknown architecture in arch_as_pretty_string: {}", static_cast<int>(architecture));
  return "Unknown-Pretty";
}

pepp::Architecture pepp::string_to_arch(const std::string &str, bool *okay) {
  using namespace pepp::bts;
  for (const auto &it : archs) {
    if (ci_eq()(it.second.basic, str) || ci_eq()(it.second.pretty, str)) {
      if (okay) *okay = true;
      return it.first;
    }
  }
  if (okay) *okay = false;
  return Architecture::NO_ARCH;
}

struct LevelData {
  std::string basic, pretty;
};

std::map<pepp::Abstraction, LevelData> init_levels() {
  using namespace pepp;
  std::map<pepp::Abstraction, LevelData> m{};
  m[Abstraction::MA2] = {"MA2", "Microarchitecture 2"};
  m[Abstraction::ISA3] = {"ISA3", "Instruction Set Architecture 3"};
  m[Abstraction::ASMB3] = {"Asmb3", "Assembly 3"};
  m[Abstraction::ASMB5] = {"Asmb5", "Assembly 5"};
  m[Abstraction::OS4] = {"OS4", "Operating System 4"};
  return m;
}

static const auto levels = init_levels();

bool pepp::is_valid_level(Abstraction level) noexcept { return levels.find(level) != levels.end(); }

bool pepp::is_valid_level(int level) noexcept { return is_valid_level(static_cast<Abstraction>(level)); }

std::string pepp::level_as_string(Abstraction level) {
  if (auto it = levels.find(level); it != levels.end()) return it->second.basic;
  SPDLOG_WARN("Unknown level in level_as_string: {}", static_cast<int>(level));
  return "Unknown";
}

std::string pepp::level_as_pretty_string(Abstraction level) {
  if (auto it = levels.find(level); it != levels.end()) return it->second.pretty;
  SPDLOG_WARN("Unknown level in level_as_pretty_string: {}", static_cast<int>(level));
  return "Unknown-Pretty";
}

pepp::Abstraction pepp::string_to_level(const std::string &str, bool *okay) {
  using namespace pepp::bts;
  for (const auto &it : levels) {
    if (ci_eq()(it.second.basic, str) || ci_eq()(it.second.pretty, str)) {
      if (okay) *okay = true;
      return it.first;
    }
  }
  if (okay) *okay = false;
  return Abstraction::NO_ABS;
}

std::map<pepp::Features, std::string> init_feats() {
  using enum pepp::Features;
  std::map<pepp::Features, std::string> m{};
  m[OneByte] = "OneByte";
  m[TwoByte] = "TwoByte";
  m[NoOS] = "NoOS";
  return m;
}

std::map<std::string, pepp::Features, pepp::bts::ci_lt> init_reverse_feats() {
  std::map<std::string, pepp::Features, pepp::bts::ci_lt> m{};
  for (const auto &it : init_feats()) m[it.second] = it.first;
  return m;
}

static const auto feats = init_feats();
static const auto reverse_feats = init_reverse_feats();
pepp::Features pepp::parse_features(const std::string &str) {
  using namespace bits;

  auto ret = pepp::Features::None;
  for (size_t start = 0, end = str.find(","); start != std::string::npos; start = end, end = str.find(",", start)) {
    const auto substr = str.substr(start, end - start);
    if (auto it = reverse_feats.find(substr); it == reverse_feats.end())
      spdlog::warn("Ignoring invalid feature {}", str.substr(start, end - start));
    else ret |= it->second;
  }
  return ret;
}

std::string pepp::features_as_pretty_string(pepp::Features features) {
  using namespace bits;
  using enum pepp::Features;

  std::vector<std::string> f;
  if (any(features & OneByte)) f.emplace_back(feats.at(OneByte));
  if (any(features & TwoByte)) f.emplace_back(feats.at(TwoByte));
  if (any(features & NoOS)) f.emplace_back(feats.at(NoOS));
  return f.empty() ? "None" : fmt::format("{}", fmt::join(f, ", "));
}
