#pragma once

#include <string_view>
#include "./header.hpp"
#include "bts/bitmanip/integers.h"
namespace pepp::bts {

struct RV32LE {
  static constexpr std::string_view name = "riscv32";
  static constexpr bool is_64 = false;
  static constexpr bool is_le = true;
  static constexpr bool is_rela = true;
  static constexpr u32 e_machine = to_underlying(MachineType::EM_RISCV);
};

struct RV64LE {
  static constexpr std::string_view name = "riscv64";
  static constexpr bool is_64 = true;
  static constexpr bool is_le = true;
  static constexpr bool is_rela = true;
  static constexpr u32 e_machine = to_underlying(MachineType::EM_RISCV);
};

struct BE32 {
  static constexpr bool is_64 = false;
  static constexpr bool is_le = false;
};

struct PEP10 : public BE32 {
  static constexpr std::string_view name = "Pep/10";
  static constexpr bool is_rela = false;
  static constexpr u32 e_machine = 'p' << 8 | 'x';
};
struct PEP9 : public BE32 {
  static constexpr std::string_view name = "Pep/9";
  static constexpr bool is_rela = false;
  static constexpr u32 e_machine = 'p' << 8 | '9';
};
struct PEP8 : public BE32 {
  static constexpr std::string_view name = "Pep/8";
  static constexpr bool is_rela = false;
  static constexpr u32 e_machine = 'p' << 8 | '8';
};

} // namespace pepp::bts
