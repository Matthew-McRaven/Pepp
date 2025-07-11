#include "./expr_serialize.hpp"

quint32 pepp::debug::types::StringInternPool::add(const QString &str) {
  if (_added.contains(str)) return _added[str];
  auto ret = _added[str] = _data.size();
  auto utf8 = str.toUtf8();
  _data.insert(_data.end(), utf8.begin(), utf8.end());
  _data.emplace_back(0);
  return ret;
}

const char *pepp::debug::types::StringInternPool::at(quint32 i) { return _data.data() + i; }

quint32 pepp::debug::types::StringInternPool::size() const { return _data.size(); }

const char *pepp::debug::types::StringInternPool::data() const { return _data.data(); }
