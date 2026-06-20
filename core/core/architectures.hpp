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
} // namespace pepp