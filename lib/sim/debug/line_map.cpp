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
