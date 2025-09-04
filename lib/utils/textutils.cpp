#include "textutils.hpp"

QString removeLeading0(const QString &str) {
  for (int it = 0; it < str.size(); it++) {
    if (str.at(it) != '0') return str.mid(it);
  }
  // Should be unreacheable, but here for safety.
  return str;
}
