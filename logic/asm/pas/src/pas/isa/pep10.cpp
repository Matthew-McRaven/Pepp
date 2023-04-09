#include "./pep10.hpp"

pas::isa::Pep10ISA::Mnemonic pas::isa::Pep10ISA::defaultMnemonic() {
  return Mnemonic::INVALID;
}

pas::isa::Pep10ISA::AddressingMode pas::isa::Pep10ISA::defaultAddressingMode() {
  return AddressingMode::INVALID;
}

pas::isa::Pep10ISA::AddressingMode
pas::isa::Pep10ISA::defaultAddressingMode(Mnemonic mnemonic) {
  if (isAType(mnemonic))
    return AddressingMode::I;
  else
    return defaultAddressingMode();
}

quint8 pas::isa::Pep10ISA::opcode(Mnemonic mnemonic) {
  return static_cast<quint8>(mnemonic);
}

quint8 pas::isa::Pep10ISA::opcode(Mnemonic mnemonic, AddressingMode addr) {
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

pas::isa::Pep10ISA::AddressingMode
pas::isa::Pep10ISA::parseAddressingMode(const QString &addr) {
  using AM = AddressingMode;
  bool ok = true;
  auto ret = (AM)QMetaEnum::fromType<AddressingMode>().keyToValue(
      addr.toUpper().toUtf8().data(), &ok);
  if (!ok || ret == AM::ALL || ret == AM::NONE)
    return AM::INVALID;
  else
    return ret;
}

pas::isa::Pep10ISA::Mnemonic
pas::isa::Pep10ISA::parseMnemonic(const QString &mnemonic) {
  bool ok = true;
  auto ret = QMetaEnum::fromType<Mnemonic>().keyToValue(
      mnemonic.toUpper().toUtf8().data(), &ok);
  if (!ok)
    return Mnemonic::INVALID;
  else
    return (Mnemonic)ret;
}

QString pas::isa::Pep10ISA::string(Mnemonic mnemonic) {
  return QString(QMetaEnum::fromType<Mnemonic>().valueToKey((int)mnemonic))
      .toUpper();
}

QString pas::isa::Pep10ISA::string(AddressingMode addr) {
  return QString(QMetaEnum::fromType<AddressingMode>().valueToKey((int)addr))
      .toLower();
}

bool pas::isa::Pep10ISA::isMnemonicUnary(Mnemonic mnemonic) {
  return isMnemonicUnary(opcode(mnemonic));
}

bool pas::isa::Pep10ISA::isMnemonicUnary(quint8 opcode) {
  using T = detail::pep10::InstructionType;
  auto type = opcodeLUT[opcode].instr.type;
  return type == T::R_none || type == T::U_none;
}

bool pas::isa::Pep10ISA::isOpcodeUnary(Mnemonic mnemonic) {
  return isOpcodeUnary(opcode(mnemonic));
}

bool pas::isa::Pep10ISA::isOpcodeUnary(quint8 opcode) {
  return opcodeLUT[opcode].instr.unary;
}

bool pas::isa::Pep10ISA::isStore(Mnemonic mnemonic) {
  using M = detail::pep10::Mnemonic;
  if (mnemonic == M::STBA || mnemonic == M::STWA || mnemonic == M::STBX ||
      mnemonic == M::STWX)
    return true;
  else
    return false;
}

bool pas::isa::Pep10ISA::isStore(quint8 opcode) {
  return isStore(opcodeLUT[opcode].instr.mnemon);
}

bool pas::isa::Pep10ISA::isUType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::U_none;
}

bool pas::isa::Pep10ISA::isRType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::R_none;
}

bool pas::isa::Pep10ISA::isAType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::A_ix;
}

bool pas::isa::Pep10ISA::isValidATypeAddressingMode(Mnemonic,
                                                    AddressingMode addr) {
  using AM = AddressingMode;
  return addr == AM::I || addr == AM::X;
}

bool pas::isa::Pep10ISA::isAAAType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::AAA_all || type == T::AAA_i;
}

bool pas::isa::Pep10ISA::isValidAAATypeAddressingMode(Mnemonic,
                                                      AddressingMode addr) {
  using AM = AddressingMode;
  return !(addr == AM::ALL || addr == AM::INVALID || addr == AM::NONE);
}

bool pas::isa::Pep10ISA::isRAAAType(Mnemonic mnemonic) {
  using T = InstructionType;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return type == T::RAAA_all || type == T::RAAA_noi;
}

bool pas::isa::Pep10ISA::isValidRAAATypeAddressingMode(Mnemonic mnemonic,
                                                       AddressingMode addr) {
  using T = InstructionType;
  using AM = AddressingMode;
  auto type = opcodeLUT[opcode(mnemonic)].instr.type;
  return !(addr == AM::ALL || addr == AM::INVALID || addr == AM::NONE ||
           (type == T::RAAA_noi && addr == AM::I));
}

bool pas::isa::Pep10ISA::requiresAddressingMode(Mnemonic mnemonic) {
  return isAAAType(mnemonic) | isRAAAType(mnemonic);
}

bool pas::isa::Pep10ISA::canElideAddressingMode(Mnemonic mnemonic,
                                                AddressingMode addr) {
  return isAType(mnemonic) && addr == AddressingMode::I;
}

bool pas::isa::Pep10ISA::isLegalDirective(QString directive) {
  static const auto valid = QSet<QString>{
      "ALIGN", "ASCII",  "BLOCK", "BYTE",  "EQUATE",  "EXPORT", "IMPORT",
      "INPUT", "OUTPUT", "ORG",   "SCALL", "SECTION", "USCALL", "WORD"};
  return valid.contains(directive.toUpper());
}
