#include "./string_utils.hpp"
#include <QtCore/qobject.h>
#include "core/bitmanip/strings.hpp"

bool bits::startsWithHexPrefix(const QString &string) { return string.startsWith("0x") || string.startsWith("0X"); }

qsizetype bits::escapedStringLength(const QString &string) {
  auto asUTF = string.toUtf8();
  auto start = asUTF.begin();
  bool okay = true;
  size_t accumulated_size = 0;
  uint8_t _;
  while (start != asUTF.end()) {
    okay &= bits::charactersToByte(start, asUTF.end(), _);
    accumulated_size++;
  }
  if (!okay) throw std::logic_error("Not a valid string!");
  return accumulated_size;
}

bool bits::escapedStringToBytes(const QString &string, QByteArray &output) {
  auto asUTF = string.toUtf8();
  auto start = asUTF.begin();
  bool okay = true;
  uint8_t temp = 0;
  while (start != asUTF.end()) {
    okay &= bits::charactersToByte(start, asUTF.end(), temp);
    output.push_back(temp);
  }
  return okay;
}
