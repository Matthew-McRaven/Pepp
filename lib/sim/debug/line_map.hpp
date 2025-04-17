#pragma once
#include <QtCore>

struct Lines2Addresses {
  Lines2Addresses() {};
  Lines2Addresses(QList<QPair<int, quint32>> source, QList<QPair<int, quint32>> list);
  std::optional<quint32> source2Address(int sourceLine);
  std::optional<quint32> list2Address(int listLine);
  std::optional<int> address2Source(quint32 address);
  std::optional<int> address2List(quint32 address);
  std::optional<int> source2List(int source);
  std::optional<int> list2Source(int list);

private:
  QMap<int, quint32> _source2Addr{}, _list2Addr{};
  QMap<quint32, int> _addr2Source{}, _addr2List{};
};
