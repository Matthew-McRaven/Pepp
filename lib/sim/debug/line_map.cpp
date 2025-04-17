#include "line_map.hpp"

Lines2Addresses::Lines2Addresses(QList<QPair<int, quint32>> source, QList<QPair<int, quint32>> list) {
  for (auto [line, addr] : source) {
    _source2Addr[line] = addr;
    _addr2Source[addr] = line;
  }
  for (auto [line, addr] : list) {
    _list2Addr[line] = addr;
    _addr2List[addr] = line;
  }
}

std::optional<quint32> Lines2Addresses::source2Address(int sourceLine) {
  if (_source2Addr.contains(sourceLine)) return _source2Addr[sourceLine];
  return std::nullopt;
}

std::optional<quint32> Lines2Addresses::list2Address(int listLine) {
  if (_list2Addr.contains(listLine)) return _list2Addr[listLine];
  return std::nullopt;
}

std::optional<int> Lines2Addresses::address2Source(quint32 address) {
  if (_addr2Source.contains(address)) return _addr2Source[address];
  return std::nullopt;
}

std::optional<int> Lines2Addresses::address2List(quint32 address) {
  if (_addr2List.contains(address)) return _addr2List[address];
  return std::nullopt;
}

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

ScopedLines2Addresses::ScopedLines2Addresses() {}

void ScopedLines2Addresses::addScope(QString name, const Lines2Addresses &map) {
  auto scopeIndex = static_cast<scope>(_scopeNames.size());
  _scopeNames[scopeIndex] = name;

  for (auto [line, addr] : map._source2Addr) {
    _source2Addr[line] = std::make_tuple(scopeIndex, addr);
    _addr2Source[addr] = std::make_tuple(scopeIndex, line);
  }
  for (auto [line, addr] : map._list2Addr) {
    _list2Addr[line] = std::make_tuple(scopeIndex, addr);
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

std::optional<std::tuple<ScopedLines2Addresses::scope, quint32>>
ScopedLines2Addresses::source2Address(int sourceLine) const {
  if (_source2Addr.contains(sourceLine)) return _source2Addr.at(sourceLine);
  return std::nullopt;
}

std::optional<std::tuple<ScopedLines2Addresses::scope, quint32>>
ScopedLines2Addresses::list2Address(int listLine) const {
  if (_list2Addr.contains(listLine)) return _list2Addr.at(listLine);
  return std::nullopt;
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

std::optional<quint32> ScopedLines2Addresses::source2Address(int sourceLine, scope s) const {
  if (auto r = source2Address(sourceLine); !r) return std::nullopt;
  else if (std::get<0>(*r) != s) return std::nullopt;
  else return std::get<1>(*r);
}

std::optional<quint32> ScopedLines2Addresses::list2Address(int listLine, scope s) const {
  if (auto r = list2Address(listLine); !r) return std::nullopt;
  else if (std::get<0>(*r) != s) return std::nullopt;
  else return std::get<1>(*r);
}

std::optional<int> ScopedLines2Addresses::address2Source(quint32 address, scope s) const {
  if (auto r = address2Source(address); !r) return std::nullopt;
  else if (std::get<0>(*r) != s) return std::nullopt;
  else return std::get<1>(*r);
}

std::optional<int> ScopedLines2Addresses::address2List(quint32 address, scope s) const {
  if (auto r = address2List(address); !r) return std::nullopt;
  else if (std::get<0>(*r) != s) return std::nullopt;
  else return std::get<1>(*r);
}

std::optional<quint32> ScopedLines2Addresses::source2Address(int sourceLine, QString scope) const {
  if (auto s = name2scope(scope); !s) return std::nullopt;
  else if (auto r = source2Address(sourceLine, *s); !r) return std::nullopt;
  else return *r;
}

std::optional<quint32> ScopedLines2Addresses::list2Address(int listLine, QString scope) const {
  if (auto s = name2scope(scope); !s) return std::nullopt;
  else if (auto r = list2Address(listLine, *s); !r) return std::nullopt;
  else return *r;
}

std::optional<int> ScopedLines2Addresses::address2Source(quint32 address, QString scope) const {
  if (auto s = name2scope(scope); !s) return std::nullopt;
  else if (auto r = address2Source(address, *s); !r) return std::nullopt;
  else return *r;
}

std::optional<int> ScopedLines2Addresses::address2List(quint32 address, QString scope) const {
  if (auto s = name2scope(scope); !s) return std::nullopt;
  else if (auto r = address2List(address, *s); !r) return std::nullopt;
  else return *r;
}

std::optional<int> ScopedLines2Addresses::list2Source(int list) const {
  auto addr = list2Address(list);
  if (!addr) return std::nullopt;
  else if (auto r = address2Source(std::get<1>(*addr)); !r) return std::nullopt;
  else return std::get<1>(*r);
}

std::optional<int> ScopedLines2Addresses::source2List(int source) const {
  auto addr = source2Address(source);
  if (!addr) return std::nullopt;
  else if (auto r = address2List(std::get<1>(*addr)); !r) return std::nullopt;
  else return std::get<1>(*r);
}
