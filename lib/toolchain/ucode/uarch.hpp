#pragma once
#include <QtCore>
#include <bitset>
#include <stdint.h>
namespace pepp::ucode {
namespace detail::pep9_1byte {
Q_NAMESPACE
enum class Signals {
  MemRead = 0,
  MemWrite,
  A,
  B,
  AMux,
  ALU,
  CSMux,
  AndZ,
  CMux,
  C,
  MDRMux,
  NCk,
  ZCk,
  VCk,
  CCk,
  SCk,
  MARCk,
  LoadCk,
  MDRCk
};
Q_ENUM_NS(Signals);
enum class NamedRegisters {
  A = 0,
  X = 2,
  SP = 4,
  PC = 6,
  IR = 8,
  T1 = 11,
  T2 = 12,
  T3 = 14,
  T4 = 16,
  T5 = 18,
  T6 = 20,
  M1 = 22,
  M2 = 24,
  M3 = 26,
  M4 = 28,
  M5 = 30,
};
Q_ENUM_NS(NamedRegisters);

static constexpr uint8_t signal_bit_size_helper(Signals s) {
  switch (s) {
  case Signals::A: [[fallthrough]];
  case Signals::B: [[fallthrough]];
  case Signals::C: return 5;
  case Signals::ALU: return 4;
  default: return 1;
  }
}
} // namespace detail::pep9_1byte

struct Pep9ByteBus {
  using Signals = detail::pep9_1byte::Signals;
  inline static constexpr uint8_t signal_bit_size(Signals s) { return detail::pep9_1byte::signal_bit_size_helper(s); }
  static uint8_t signal_group(Signals s);
  struct Code {
    uint8_t MemRead : signal_bit_size_helper(Signals::MemRead) = 0;
    uint8_t MemWrite : signal_bit_size_helper(Signals::MemWrite) = 0;
    uint8_t A : signal_bit_size_helper(Signals::A) = 0;
    uint8_t B : signal_bit_size_helper(Signals::B) = 0;
    uint8_t AMux : signal_bit_size_helper(Signals::AMux) = 0;
    uint8_t ALU : signal_bit_size_helper(Signals::ALU) = 0;
    uint8_t CSMux : signal_bit_size_helper(Signals::CSMux) = 0;
    uint8_t AndZ : signal_bit_size_helper(Signals::AndZ) = 0;
    uint8_t CMux : signal_bit_size_helper(Signals::CMux) = 0;
    uint8_t C : signal_bit_size_helper(Signals::C) = 0;
    uint8_t MDRMux : signal_bit_size_helper(Signals::MDRMux) = 0;
    uint8_t NCk : signal_bit_size_helper(Signals::NCk) = 0;
    uint8_t ZCk : signal_bit_size_helper(Signals::ZCk) = 0;
    uint8_t VCk : signal_bit_size_helper(Signals::VCk) = 0;
    uint8_t CCk : signal_bit_size_helper(Signals::CCk) = 0;
    uint8_t SCk : signal_bit_size_helper(Signals::SCk) = 0;
    uint8_t MARCk : signal_bit_size_helper(Signals::MARCk) = 0;
    uint8_t LoadCk : signal_bit_size_helper(Signals::LoadCk) = 0;
    uint8_t MDRCk : signal_bit_size_helper(Signals::MDRCk) = 0;
    void set(Signals s, uint8_t value);
    uint8_t get(Signals s) const;
  };
  struct CodeWithEnables {
    Code code;
    std::bitset<static_cast<int>(Signals::MDRCk) + 1> enables;
    bool enabled(Signals s) const;
    void clear(Signals s);
    void set(Signals s, uint8_t value);
    uint8_t get(Signals s) const;
  };
  using NamedRegisters = detail::pep9_1byte::NamedRegisters;
  static uint8_t register_byte_size(NamedRegisters reg);
};

namespace detail::pep9_2byte {
Q_NAMESPACE
enum class Signals {
  MemRead = 0,
  MemWrite,
  A,
  B,
  EOMux,
  MARMux,
  AMux,
  ALU,
  CSMux,
  AndZ,
  CMux,
  C,
  MDROMux,
  MDREMux,
  NCk,
  ZCk,
  VCk,
  CCk,
  SCk,
  MARCk,
  LoadCk,
  MDROCk,
  MDRECk
};
Q_ENUM_NS(Signals);

static constexpr uint8_t signal_bit_size_helper(Signals s) {
  switch (s) {
  case Signals::A: [[fallthrough]];
  case Signals::B: [[fallthrough]];
  case Signals::C: return 5;
  case Signals::ALU: return 4;
  default: return 1;
  }
}
} // namespace detail::pep9_2byte
struct Pep9WordBus {
  using Signals = detail::pep9_2byte::Signals;
  inline static constexpr uint8_t signal_bit_size(Signals s) { return detail::pep9_2byte::signal_bit_size_helper(s); }
  static uint8_t signal_group(Signals s);
  struct Code {
    uint8_t MemRead : signal_bit_size_helper(Signals::MemRead) = 0;
    uint8_t MemWrite : signal_bit_size_helper(Signals::MemWrite) = 0;
    uint8_t A : signal_bit_size_helper(Signals::A) = 0;
    uint8_t B : signal_bit_size_helper(Signals::B) = 0;
    uint8_t EOMux : signal_bit_size_helper(Signals::EOMux) = 0;
    uint8_t MARMux : signal_bit_size_helper(Signals::MARMux) = 0;
    uint8_t AMux : signal_bit_size_helper(Signals::AMux) = 0;
    uint8_t ALU : signal_bit_size_helper(Signals::ALU) = 0;
    uint8_t CSMux : signal_bit_size_helper(Signals::CSMux) = 0;
    uint8_t AndZ : signal_bit_size_helper(Signals::AndZ) = 0;
    uint8_t CMux : signal_bit_size_helper(Signals::CMux) = 0;
    uint8_t C : signal_bit_size_helper(Signals::C) = 0;
    uint8_t MDROMux : signal_bit_size_helper(Signals::MDROMux) = 0;
    uint8_t MDREMux : signal_bit_size_helper(Signals::MDREMux) = 0;
    uint8_t NCk : signal_bit_size_helper(Signals::NCk) = 0;
    uint8_t ZCk : signal_bit_size_helper(Signals::ZCk) = 0;
    uint8_t VCk : signal_bit_size_helper(Signals::VCk) = 0;
    uint8_t CCk : signal_bit_size_helper(Signals::CCk) = 0;
    uint8_t SCk : signal_bit_size_helper(Signals::SCk) = 0;
    uint8_t MARCk : signal_bit_size_helper(Signals::MARCk) = 0;
    uint8_t LoadCk : signal_bit_size_helper(Signals::LoadCk) = 0;
    uint8_t MDROCk : signal_bit_size_helper(Signals::MDROCk) = 0;
    uint8_t MDRECk : signal_bit_size_helper(Signals::MDRECk) = 0;

    void set(Signals s, uint8_t value);
    uint8_t get(Signals s) const;
  };
  struct CodeWithEnables {
    Code code;
    std::bitset<static_cast<int>(Signals::MDRECk) + 1> enables;
    bool enabled(Signals s) const;
    void clear(Signals s);
    void set(Signals s, uint8_t value);
    uint8_t get(Signals s) const;
  };
  using NamedRegisters = detail::pep9_1byte::NamedRegisters;
  static uint8_t register_byte_size(NamedRegisters reg);
};
} // namespace pepp::ucode
