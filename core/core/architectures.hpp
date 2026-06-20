#pragma once

#include <string>

namespace pepp {

enum class Architecture {
  NO_ARCH = -1, //! Architecture is unspecified.
  PEP8 = 80,    //! The figure must be used with the Pep/8 toolchain.
  PEP9 = 90,    //! The figure must be used with the Pep/9 toolchain.
  PEP10 = 100,  //! The figure must be use with the Pep/10 toolchain.
  RISCV = 1000, //! The figure must be used with the RISC-V toolchain.
};
bool is_valid_arch(Architecture arch) noexcept;
bool is_valid_arch(int arch) noexcept;
std::string arch_as_string(Architecture architecture);
std::string arch_as_pretty_string(Architecture architecture);
Architecture string_to_arch(const std::string &str, bool *okay = nullptr);

enum class AbstractionEnu {
  NO_ABS = -1,
  // LG1 = 1,
  MA2 = 20,
  ISA3 = 30,
  ASMB3 = 31,
  OS4 = 40,
  ASMB5 = 50,
  // HOL6 = 6,
  // APP7 = 7,
};
bool is_valid_level(AbstractionEnu level) noexcept;
bool is_valid_level(int level) noexcept;
std::string level_as_string(AbstractionEnu level);
std::string level_as_pretty_string(AbstractionEnu level);
AbstractionEnu string_to_level(const std::string &str, bool *okay = nullptr);
} // namespace pepp