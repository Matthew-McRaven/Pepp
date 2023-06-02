#include "pep10.hpp"
#include <QMetaEnum>

isa::Pep10::Mnemonic isa::Pep10::defaultMnemonic() { return Mnemonic::INVALID; }

isa::Pep10::AddressingMode isa::Pep10::defaultAddressingMode() {
  return AddressingMode::INVALID;
}

isa::Pep10::AddressingMode
isa::Pep10::defaultAddressingMode(Mnemonic mnemonic) {
  if (isAType(mnemonic))
    return AddressingMode::I;
  else
    return defaultAddressingMode();
}

quint8 isa::Pep10::opcode(Mnemonic mnemonic) {
  return static_cast<quint8>(mnemonic);
}

quint8 isa::Pep10::opcode(Mnemonic mnemonic, AddressingMode addr) {
  using M = detail::pep10::Mnemonic;
  using AM = detail::pep10::AddressingMode;
  auto base = opcode(mnemonic);
  // TODO: Look up instruction type instead of doing opcode math.
  if (base >= static_cast<quint8>(M::BR) &&
      base <= static_cast<quint8>(M::CALL))
    return base | (addr == AM::X ? 1 : 0);
  switch (addr) {
  case AM::NONE:
    throw std::logic_error("Invalid ADDR mode");
  case AM::I:
    return base | 0x0;
  case AM::D:
    return base | 0x1;
  case AM::N:
    return base | 0x2;
  case AM::S:
    return base | 0x3;
  case AM::SF:
    return base | 0x4;
  case AM::X:
    return base | 0x5;
  case AM::SX:
    return base | 0x6;
  case AM::SFX:
    return base | 0x7;
  case AM::ALL:
    throw std::logic_error("Invalid ADDR mode");
  case AM::INVALID:
    throw std::logic_error("Invalid ADDR mode");
  }
  throw std::logic_error("Unreachable");
}

isa::Pep10::AddressingMode
isa::Pep10::parseAddressingMode(const QString &addr) {
  using AM = AddressingMode;
  bool ok = true;
  auto ret = (AM)QMetaEnum::fromType<AddressingMode>().keyToValue(
      addr.toUpper().toUtf8().data(), &ok);
  if (!ok || ret == AM::ALL || ret == AM::NONE)
    return AM::INVALID;
  else
    return ret;
}

isa::Pep10::Mnemonic isa::Pep10::parseMnemonic(const QString &mnemonic) {
  bool ok = true;
  auto ret = QMetaEnum::fromType<Mnemonic>().keyToValue(
      mnemonic.toUpper().toUtf8().data(), &ok);
  if (!ok)
    return Mnemonic::INVALID;
  else
    return (Mnemonic)ret;
}

QString isa::Pep10::string(Mnemonic mnemonic) {
  return QString(QMetaEnum::fromType<Mnemonic>().valueToKey((int)mnemonic))
      .toUpper();
}

QString isa::Pep10::string(AddressingMode addr) {
  return QString(QMetaEnum::fromType<AddressingMode>().valueToKey((int)addr))
      .toLower();
}

bool isa::Pep10::isMnemonicUnary(Mnemonic mnemonic) {
  return isMnemonicUnary(opcode(mnemonic));
}

bool isa::Pep10::isMnemonicUnary(quint8 opcode) {
  using T = detail::pep10::InstructionType;
  auto type = opcodeLUT[opcode].instr.type;
  return type == T::R_none || type == T::U_none;
}

bool isa::Pep10::isOpcodeUnary(Mnemonic mnemonic) {
  return isOpcodeUnary(opcode(mnemonic));
}

bool isa::Pep10::isOpcodeUnary(quint8 opcode) {
  return opcodeLUT[opcode].instr.unary;
}

bool isa::Pep10::isStore(Mnemonic mnemonic) {
  using M = detail::pep10::Mnemonic;
  if (mnemonic == M::STBA || mnemonic == M::STWA || mnemonic == M::STBX ||
      mnemonic == M::STWX)
    return true;
  else
    return false;
}

bool isa::Pep10::isStore(quint8 opcode) {
  return isStore(opcodeLUT[opcode].instr.mnemon);
}

bool isa::Pep10::isUType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::U_none;
}

bool isa::Pep10::isRType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::R_none;
}

bool isa::Pep10::isAType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::A_ix;
}

bool isa::Pep10::isValidATypeAddressingMode(Mnemonic, AddressingMode addr) {
  using AM = AddressingMode;
  return addr == AM::I || addr == AM::X;
}

bool isa::Pep10::isAAAType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::AAA_all || type == T::AAA_i;
}

bool isa::Pep10::isValidAAATypeAddressingMode(Mnemonic, AddressingMode addr) {
  using AM = AddressingMode;
  return !(addr == AM::ALL || addr == AM::INVALID || addr == AM::NONE);
}

bool isa::Pep10::isRAAAType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::RAAA_all || type == T::RAAA_noi;
}

bool isa::Pep10::isValidRAAATypeAddressingMode(Mnemonic mnemonic,
                                               AddressingMode addr) {
  using T = InstructionType;
  using AM = AddressingMode;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return !(addr == AM::ALL || addr == AM::INVALID || addr == AM::NONE ||
           (type == T::RAAA_noi && addr == AM::I));
}

bool isa::Pep10::isValidAddressingMode(Mnemonic mnemonic, AddressingMode addr) {
  using T = InstructionType;
  using AM = AddressingMode;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  switch (type) {
  case detail::pep10::InstructionType::Invalid:
    [[fallthrough]];
  case detail::pep10::InstructionType::U_none:
    [[fallthrough]];
  case detail::pep10::InstructionType::R_none:
    return false;
  case detail::pep10::InstructionType::A_ix:
    return addr == AM::X || addr == AM::I;
  case detail::pep10::InstructionType::AAA_i:
    return addr == AM::I;
  case detail::pep10::InstructionType::AAA_all:
  case detail::pep10::InstructionType::RAAA_all:
    return !(addr == AM::ALL || addr == AM::INVALID || addr == AM::NONE);
  case detail::pep10::InstructionType::RAAA_noi:
    return !(addr == AM::ALL || addr == AM::INVALID || addr == AM::NONE ||
             addr == AM::I);
  }
  return false;
}

bool isa::Pep10::requiresAddressingMode(Mnemonic mnemonic) {
  return isAAAType(mnemonic) | isRAAAType(mnemonic);
}

bool isa::Pep10::canElideAddressingMode(Mnemonic mnemonic,
                                        AddressingMode addr) {
  return isAType(mnemonic) && addr == AddressingMode::I;
}

bool isa::Pep10::isLegalDirective(QString directive) {
  static const auto valid = QSet<QString>{
      "ALIGN", "ASCII",  "BLOCK", "BYTE",  "EQUATE",  "EXPORT", "IMPORT",
      "INPUT", "OUTPUT", "ORG",   "SCALL", "SECTION", "USCALL", "WORD"};
  return valid.contains(directive.toUpper());
}
