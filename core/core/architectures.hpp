#pragma once

namespace pepp {
// Give it a unique name to not break everything all at once
enum class Architecture_Enums {
  NO_ARCH = -1, //! Architecture is unspecified.
  PEP8 = 80,    //! The figure must be used with the Pep/8 toolchain.
  PEP9 = 90,    //! The figure must be used with the Pep/9 toolchain.
  PEP10 = 100,  //! The figure must be use with the Pep/10 toolchain
  RISCV = 1000, //! The figure must be used with the RISC-V toolchain, which is
  //! undefined as of 2023-02-14.
};
} // namespace pepp