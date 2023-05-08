#pragma once
#include <QtCore>
namespace bits {
template <typename Iterator>
bool charactersToByte(Iterator &start, Iterator end, uint8_t &value) {
  // If start == end, then there are no characters to parse!
  if (start == end) {
    return false;
  }
  char head = *start++;
  if (head == '\\') {
    if (start == end)
      return false;
    head = *start++;
    if (head == 'b') { // backspace
      value = 8;
    } else if (head == 'f') { // form feed
      value = 12;
    } else if (head == 'n') { // line feed (new line)
      value = 10;
    } else if (head == 'r') { // carriage return
      value = 13;
    } else if (head == 't') { // horizontal tab
      value = 9;
    } else if (head == 'v') { // vertical tab
      value = 11;
    } else if (head == 'x' || head == 'X') { // hex strings!
      // Need at least two more characters to consume.
      if (end - start < 2)
        return false;
      else {
        char *end;
        char copied[] = {*(start++), *(start++), '\0'};
        value = strtol(copied, &end, 16);
        if (*end != '\0')
          return false;
      }
    } else {
      throw std::logic_error("I don't know where this was ever used!");
      value = static_cast<uint8_t>('\\');
    }
  } else {
    value = head;
  }
  return true;
}

bool startsWithHexPrefix(const QString &string);
qsizetype escapedStringLength(const QString string);
bool escapedStringToBytes(const QString &string, QByteArray &output);
// Separates every byte with a space.
qsizetype bytesToAsciiHex(char *out, qsizetype outLength, const quint8 *in,
                          quint16 inLength, bool separator);
} // namespace bits
