#pragma once
#include "utils/constants.hpp"
namespace project {
// Additional options requested for a project.
// A particular (arch, level) tuple may only support a subset of features.
enum class Features : int {
  None = 0,
  OneByte,
  TwoByte,
  NoOS,
};

struct Environment {
  utils::Architecture::Value arch;
  utils::Abstraction::Value level;
  Features features;
};
} // namespace project
