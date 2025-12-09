#include "textutils.hpp"

QString removeLeading0(const QString &str) {
  for (int it = 0; it < str.size(); it++) {
    if (str.at(it) != '0') return str.mid(it);
  }
  // Should be unreacheable, but here for safety.
  return str;
}

QStringView rtrimmed(const QString &str) {
  // Perform right-strip of string. `QString::trimmed() const` trims both ends.
  qsizetype lastIndex = str.size() - 1;
  while (QChar(str[lastIndex]).isSpace() && lastIndex > 0) lastIndex--;
  // If line is all spaces, then the string should be empty.
  if (lastIndex == 0) return QStringView();
  // Otherwise, we need to add 1 to last index to convert index (0-based) to size (1-based).
  return QStringView(str).left(lastIndex + 1);
}
