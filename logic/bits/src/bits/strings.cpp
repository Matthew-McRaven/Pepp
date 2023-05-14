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

qsizetype bits::bytesToAsciiHex(span<char> out, span<const quint8> in,
                                QVector<SeparatorRule> separators) {
  static const quint8 chars[] = "0123456789ABCDEF";
  qsizetype outIt = 0;
  for (int inIt = 0; inIt < in.size(); inIt++) {
    if (outIt + 2 <= out.size()) {
      out[outIt++] = chars[(in[inIt] >> 4) & 0x0f];
      out[outIt++] = chars[in[inIt] & 0xf];
    } else
      break;

    if (outIt + 1 > out.size())
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

std::optional<QList<quint8>> bits::asciiHexToByte(span<const char> in) {
  QList<quint8> ret = {};
  ret.reserve(in.size() / 3 + 2);
  qsizetype inIt = 0;
  char *endptr = nullptr;
  while (inIt + 3 < in.size()) {
    ret.push_back(strtol(in.subspan(inIt).data(), &endptr, 16));
    if (endptr > in.subspan(inIt + 2).data())
      return std::nullopt;
    inIt += 3;
  }
  return ret;
}
