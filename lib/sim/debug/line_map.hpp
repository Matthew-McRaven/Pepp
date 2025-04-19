#pragma once
#include <QtCore>
#include <QtQmlIntegration>
class ScopedLines2Addresses;
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
  friend class ScopedLines2Addresses;
  std::map<int, quint32> _source2Addr{}, _list2Addr{};
  std::map<quint32, int> _addr2Source{}, _addr2List{};
};

class ScopedLines2Addresses : public QObject {
  Q_OBJECT
  QML_ELEMENT

public:
  using scope = int;
  ScopedLines2Addresses(QObject *parent = nullptr);
  ~ScopedLines2Addresses() = default;
  void addScope(QString name, const Lines2Addresses &map);
  std::optional<QString> scope2name(scope) const;
  std::optional<scope> name2scope(QString) const;
  std::optional<std::tuple<scope, quint32>> source2Address(int sourceLine) const;
  std::optional<std::tuple<scope, quint32>> list2Address(int listLine) const;
  std::optional<std::tuple<scope, int>> address2Source(quint32 address) const;
  std::optional<std::tuple<scope, int>> address2List(quint32 address) const;
  std::optional<quint32> source2Address(int sourceLine, scope) const;
  std::optional<quint32> list2Address(int listLine, scope) const;
  std::optional<int> address2Source(quint32 address, scope) const;
  std::optional<int> address2List(quint32 address, scope) const;
  std::optional<quint32> source2Address(int sourceLine, QString) const;
  std::optional<quint32> list2Address(int listLine, QString) const;
  std::optional<int> address2Source(quint32 address, QString) const;
  std::optional<int> address2List(quint32 address, QString) const;

  std::optional<int> source2List(int source) const;
  std::optional<int> list2Source(int list) const;

public slots:
  void onReset();

signals:
  void wasReset();

private:
  std::map<int, std::tuple<scope, quint32>> _source2Addr{}, _list2Addr{};
  std::map<quint32, std::tuple<scope, int>> _addr2Source{}, _addr2List{};
  std::map<scope, QString> _scopeNames;
};
