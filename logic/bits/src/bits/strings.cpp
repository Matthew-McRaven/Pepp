#include "strings.hpp"

bool bits::startsWithHexPrefix(const QString &string) {
  return string.startsWith("0x") || string.startsWith("0X");
}

qsizetype bits::escapedStringLength(const QString string) {
  auto asUTF = string.toUtf8();
  auto start = asUTF.begin();
  bool okay = true;
  size_t accumulated_size = 0;
  uint8_t _;
  while (start != asUTF.end()) {
    okay &= bits::charactersToByte(start, asUTF.end(), _);
    accumulated_size++;
  }
  if (!okay)
    throw std::logic_error("Not a valid string!");
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

qsizetype bits::bytesToAsciiHex(char *out, qsizetype outLength,
                                const quint8 *in, quint16 inLength,
                                QVector<SeparatorRule> separators) {
  static const quint8 chars[] = "0123456789ABCDEF";
  qsizetype outIt = 0;
  for (int inIt = 0; inIt < inLength; inIt++) {
    if (outIt + 2 <= outLength) {
      out[outIt++] = chars[(in[inIt] >> 4) & 0x0f];
      out[outIt++] = chars[in[inIt] & 0xf];
    } else
      break;

    if (outIt + 1 > outLength)
      break;
    for (auto &rule : separators) {
      if ((inIt + 1) % rule.modulus == 0) {
        out[outIt++] = rule.separator;
        break;
      }
    }
  }
  return outIt;
}

std::optional<QList<quint8>> bits::asciiHexToByte(const char *in,
                                                  quint16 inLength) {
  QList<quint8> ret = {};
  ret.reserve(inLength / 3 + 2);
  qsizetype inIt = 0;
  char *endptr = nullptr;
  while (inIt + 3 < inLength) {
    ret.push_back(strtol(in + inIt, &endptr, 16));
    if (endptr > in + inIt + 2)
      return std::nullopt;
    inIt += 3;
  }
  return ret;
}
