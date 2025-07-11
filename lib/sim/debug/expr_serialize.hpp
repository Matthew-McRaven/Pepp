#pragma once
#include <QString>
#include <map>
#include <zpp_bits.h>

namespace pepp::debug::types {
struct StringInternPool {
  quint32 add(const QString &);
  const char *at(quint32);
  quint32 size() const;
  const char *data() const;

private:
  std::map<QString, quint32> _added;
  std::vector<char> _data = {0};
};

} // namespace pepp::debug::types
