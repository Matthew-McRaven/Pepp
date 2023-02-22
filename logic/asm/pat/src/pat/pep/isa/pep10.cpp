#include "pep10.hpp"

Pep10ISA::Mnemonic Pep10ISA::defaultMnemonic() { return Mnemonic::INVALID; }

Pep10ISA::AddressingMode Pep10ISA::defaultAddressingMode() {
  return AddressingMode::INVALID;
}

quint8 Pep10ISA::opcode(const Mnemonic &instr) {
  return static_cast<quint8>(instr);
}

quint8 Pep10ISA::opcode(const Mnemonic &instr, AddressingMode addr) {
  using M = detail::Mnemonic;
  using AM = detail::AddressingMode;
  auto base = opcode(instr);
  // TODO: Look up instruction type instead of doing opcode math.
  if (base >= static_cast<quint8>(M::BR) &&
      base <= static_cast<quint8>(M::CALL))
    return base | (addr == AM::X ? 1 : 0);
  switch (addr) {
  case detail::AddressingMode::NONE:
    throw std::logic_error("Invalid ADDR mode");
  case detail::AddressingMode::I:
    return base | 0x0;
  case detail::AddressingMode::D:
    return base | 0x1;
  case detail::AddressingMode::N:
    return base | 0x2;
  case detail::AddressingMode::S:
    return base | 0x3;
  case detail::AddressingMode::SF:
    return base | 0x4;
  case detail::AddressingMode::X:
    return base | 0x5;
  case detail::AddressingMode::SX:
    return base | 0x6;
  case detail::AddressingMode::SFX:
    return base | 0x7;
  case detail::AddressingMode::ALL:
    throw std::logic_error("Invalid ADDR mode");
  case detail::AddressingMode::INVALID:
    throw std::logic_error("Invalid ADDR mode");
  }
  throw std::logic_error("Unreachable");
}

Pep10ISA::AddressingMode Pep10ISA::parseAddressingMode(const QString &addr) {
  using AM = AddressingMode;
  bool ok = true;
  auto ret = (AM)QMetaEnum::fromType<AddressingMode>().keyToValue(
      addr.toUtf8().data(), &ok);
  if (!ok || ret == AM::ALL || ret == AM::NONE)
    return AddressingMode::INVALID;
  else
    return ret;
}

Pep10ISA::Mnemonic Pep10ISA::parseMnemonic(const QString &mnemonic) {
  bool ok = true;
  auto ret =
      QMetaEnum::fromType<Mnemonic>().keyToValue(mnemonic.toUtf8().data(), &ok);
  if (!ok)
    return Mnemonic::INVALID;
  else
    return (Mnemonic)ret;
}

QString Pep10ISA::string(Mnemonic mnemon) {
  return QMetaEnum::fromType<Mnemonic>().valueToKey((int)mnemon);
}

QString Pep10ISA::string(AddressingMode mode) {
  return QMetaEnum::fromType<AddressingMode>().valueToKey((int)mode);
}

bool Pep10ISA::isMnemonicUnary(Mnemonic mnemon) {
  return isMnemonicUnary(opcode(mnemon));
}

bool Pep10ISA::isMnemonicUnary(quint8 opcode) {
  using T = detail::InstructionType;
  auto type = opcodeLUT[opcode].instr.type;
  return type == T::R_none || type == T::U_none;
}

bool Pep10ISA::isOpcodeUnary(Mnemonic mnemon) {

  return isOpcodeUnary(opcode(mnemon));
}

bool Pep10ISA::isOpcodeUnary(quint8 opcode) {
  return opcodeLUT[opcode].instr.unary;
}

bool Pep10ISA::isStore(Mnemonic mnemon) {
  using M = detail::Mnemonic;
  if (mnemon == M::STBA || mnemon == M::STWA || mnemon == M::STBX ||
      mnemon == M::STWX)
    return true;
  else
    return false;
}

bool Pep10ISA::isStore(quint8 opcode) {
  return isStore(opcodeLUT[opcode].instr.mnemon);
}

bool Pep10ISA::isUType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::U_none;
}

bool Pep10ISA::isRType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::R_none;
}

bool Pep10ISA::isAType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::A_ix;
}

bool Pep10ISA::isValidATypeAddressingMode(AddressingMode mode) {
  using AM = AddressingMode;
  return mode == AM::I || mode == AM::X;
}

bool Pep10ISA::isAAAType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::AAA_all;
}

bool Pep10ISA::isValidAAATypeAddressingMode(AddressingMode mode) {
  using AM = AddressingMode;
  return !(mode == AM::ALL || mode == AM::INVALID || mode == AM::NONE);
}

bool Pep10ISA::isRAAAType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::RAAA_all || type == T::RAAA_noi;
}

bool Pep10ISA::isValidRAAATypeAddressingMode(AddressingMode mode) {
  using AM = AddressingMode;
  return !(mode == AM::ALL || mode == AM::INVALID || mode == AM::NONE);
}
