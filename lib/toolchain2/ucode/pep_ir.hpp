#pragma once
#include <QtCore>

namespace pepp::tc::ir {
// A test condition which gets/sets a memory address
struct MemTest {
  MemTest() = default;
  MemTest(quint16 addr, quint16 value);
  MemTest(quint16 addr, quint8 value);
  ~MemTest() = default;
  MemTest(const MemTest &) = default;
  MemTest(MemTest &&) = default;
  MemTest &operator=(const MemTest &) = default;
  MemTest &operator=(MemTest &&) = default;
  quint8 value[2] = {0, 0};
  quint16 address = 0;
  quint8 size = 1; // Either 1 or 2 bytes.
  operator QString() const;
};
// A test condition which gets/sets a variable-sized register
template <typename registers> struct RegisterTest {
  typename registers::NamedRegisters reg;
  // Based on the size of the register, you MUST mask this down to [1-3] bytes.
  // You will also need to byteswap and shift depending on the architecture
  quint32 value = 0;
  operator QString() const {
    const auto size = registers::register_byte_size(reg);
    static const quint32 masks[4] = {0, 0xFF, 0xFF'FF, 0xFF'FF'FF'FF};
    return QString("%1=0x%2")
        .arg(registers::register_name(reg))
        .arg(QString::number(value & masks[size], 16).rightJustified(2 * size, '0'));
  }
};
// A test condition which gets/sets a variable-sized register
template <typename registers> struct CSRTest {
  typename registers::CSRs reg;
  bool value = false;
  operator QString() const { return QString("%1=0x%2").arg(registers::csr_name(reg)).arg(QString::number(value)); }
};

// A UnitPre or UnitPost is a comma-delimited set of memory tests or register tests
template <typename registers> using Test = std::variant<MemTest, RegisterTest<registers>, CSRTest<registers>>;
template <typename registers> QString toString(const Test<registers> &test) {
  if (std::holds_alternative<MemTest>(test)) return std::get<MemTest>(test);
  else if (std::holds_alternative<RegisterTest<registers>>(test)) return std::get<RegisterTest<registers>>(test);
  else if (std::holds_alternative<CSRTest<registers>>(test)) return std::get<CSRTest<registers>>(test);
  else return QString();
}

// Wrap control signals (e.g., our object code) with some assembler IR
template <typename uarch, typename registers> struct Line {
  typename uarch::CodeWithEnables controls;
  std::optional<QString> comment, symbolDecl;
  uint16_t address = -1; // Needed to compute branch targets in control section
  // Some signals allow symbolic values, whose values are only known after parsing completes.
  // This records which signals must be updated after all lines have been parsed.
  QMap<typename uarch::Signals, QString> deferredValues;
  // Will always be code unless this is a UnitPre or UnitPost.
  enum class Type { Code, Pre, Post } type = Type::Code;
  // List of all tests defined in UnitPre/UnitPost. See type to determine which.
  QList<Test<registers>> tests;
};

// Given an assembled line, produce the cannonical formatted representation of that line.
template <typename uarch, typename registers> QString format(const Line<uarch, registers> &line) {
  QString symbolDecl;
  if (line.symbolDecl.has_value()) symbolDecl = *line.symbolDecl + ": ";
  std::string _signals = line.controls.toString();
  return QString("%1%2%3").arg(symbolDecl, QString::fromStdString(_signals),
                               line.comment.has_value() ? *line.comment : "");
}
} // namespace pepp::tc::ir
