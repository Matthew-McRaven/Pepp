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

struct Environemnt {
  utils::Architecture arch;
  utils::Abstraction level;
  Features features;
};
} // namespace project
