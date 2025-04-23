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

ScopedLines2Addresses::ScopedLines2Addresses(QObject *parent) : QObject(parent) {}

void ScopedLines2Addresses::addScope(QString name, const Lines2Addresses &map) {
  auto scopeIndex = static_cast<scope>(_scopeNames.size());
  _scopeNames[scopeIndex] = name;
  _source2Addr.emplace_back();
  _list2Addr.emplace_back();

  for (auto [line, addr] : map._source2Addr) {
    _source2Addr[scopeIndex][line] = addr;
    _addr2Source[addr] = std::make_tuple(scopeIndex, line);
  }
  for (auto [line, addr] : map._list2Addr) {
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
