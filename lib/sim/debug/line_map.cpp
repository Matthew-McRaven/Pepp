#include "line_map.hpp"

Lines2Addresses::Lines2Addresses(std::vector<std::pair<int, u32>> source, std::vector<std::pair<int, u32>> list)
    : _source(source), _listing(list) {}

std::optional<quint32> Lines2Addresses::source2Address(int l) { return _source.address(l); }

std::optional<quint32> Lines2Addresses::list2Address(int l) { return _listing.address(l); }

std::optional<int> Lines2Addresses::address2Source(quint32 address) { return _source.line(address); }

std::optional<int> Lines2Addresses::address2List(quint32 address) { return _listing.line(address); }

std::optional<int> Lines2Addresses::source2List(int source) {
  auto addr = source2Address(source);
  if (!addr) return std::nullopt;
  return address2List(*addr);
}

std::optional<int> Lines2Addresses::list2Source(int list) {
  auto addr = list2Address(list);
  if (!addr) return std::nullopt;
  return address2Source(*addr);
}

ScopedLines2Addresses::ScopedLines2Addresses(QObject *parent) : QObject(parent) {}

void ScopedLines2Addresses::addScope(QString name) { auto scopeIndex = add_or_get_scope(name); }

void ScopedLines2Addresses::addScope(QString name, const Lines2Addresses &map) {
  auto scopeIndex = add_or_get_scope(name);
  for (auto [line, addr] : map._source) {
    _source2Addr[scopeIndex][line] = addr;
    _addr2Source[addr] = std::make_tuple(scopeIndex, line);
  }
  for (auto [line, addr] : map._listing) {
    _list2Addr[scopeIndex][line] = addr;
    _addr2List[addr] = std::make_tuple(scopeIndex, line);
  }
}

std::optional<QString> ScopedLines2Addresses::scope2name(scope s) const {
  auto f = _scopeNames.find(s);
  if (f == _scopeNames.cend()) return std::nullopt;
  else return f->second;
}

std::optional<ScopedLines2Addresses::scope> ScopedLines2Addresses::name2scope(QString scope) const {
  for (const auto &[s, name] : _scopeNames)
    if (name == scope) return s;
  return std::nullopt;
}

std::optional<quint32> ScopedLines2Addresses::source2Address(int sourceLine, scope s) const {
  if (s < 0 || s >= _source2Addr.size()) return std::nullopt;
  auto m = _source2Addr[s];
  if (auto r = m.find(sourceLine); r == m.cend()) return std::nullopt;
  else return r->second;
}

std::optional<quint32> ScopedLines2Addresses::list2Address(int listLine, scope s) const {
  if (s < 0 || s >= _list2Addr.size()) return std::nullopt;
  auto m = _list2Addr[s];
  if (auto r = m.find(listLine); r == m.cend()) return std::nullopt;
  else return r->second;
}

std::optional<int> ScopedLines2Addresses::list2Source(int list, scope s) const {
  auto addr = list2Address(list, s);
  if (!addr) return std::nullopt;
  else if (auto r = address2Source(*addr); !r) return std::nullopt;
  else return std::get<1>(*r);
}

std::optional<int> ScopedLines2Addresses::source2List(int source, scope s) const {
  auto addr = source2Address(source, s);
  if (!addr) return std::nullopt;
  else if (auto r = address2List(*addr); !r) return std::nullopt;
  else return std::get<1>(*r);
}

std::optional<std::tuple<ScopedLines2Addresses::scope, int>>
ScopedLines2Addresses::address2Source(quint32 address) const {
  if (_addr2Source.contains(address)) return _addr2Source.at(address);
  return std::nullopt;
}

std::optional<std::tuple<ScopedLines2Addresses::scope, int>>
ScopedLines2Addresses::address2List(quint32 address) const {
  if (_addr2List.contains(address)) return _addr2List.at(address);
  return std::nullopt;
}

void ScopedLines2Addresses::onReset() {
  _source2Addr.clear(), _list2Addr.clear();
  _addr2Source.clear(), _addr2List.clear();
  _scopeNames.clear();
  emit wasReset();
}

ScopedLines2Addresses::scope ScopedLines2Addresses::add_or_get_scope(QString name) {
  auto it = std::find_if(_scopeNames.begin(), _scopeNames.end(), [&name](const auto &it) { return it.second == name; });
  if (it == _scopeNames.end()) {
    auto scopeIndex = static_cast<scope>(_scopeNames.size());
    _scopeNames[scopeIndex] = name;
    _source2Addr.emplace_back();
    _list2Addr.emplace_back();
    return scopeIndex;
  } else {
    return it->first;
  }
}
