#pragma once
#include <QtCore>
#include <QtQmlIntegration>
#include <bitset>

namespace pepp::debug {
class BreakpointSet : public QObject {
public:
  explicit BreakpointSet();
  void addBP(quint16 address);
  void removeBP(quint16 address);
  bool hasBP(quint16 address) const;
  void clearBPs();
  void notifyPCChanged(quint16 newValue);
  std::size_t count() const;

  bool hit() const;
  void clearHit();

private:
  // Used to enable constant-time detection of the common case (no breakpoints).
  // Packs an 8-byte interval into each bit, and a 64-byte per interval per byte.
  // Should consume ~1024 bytes of memory total for Pepp procesors.
  std::bitset<0x1'00'00 / 8> _bitmask;
  std::set<quint16> _breakpoints;
  bool _hit = false;
};

class BreakpointTableModel : public QAbstractTableModel {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(pepp::debug::BreakpointSet *breakpoints READ breakpoints WRITE setBreakpoints NOTIFY breakpointsChanged)
public:
  explicit BreakpointTableModel(QObject *parent = nullptr);
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;

  BreakpointSet *breakpoints();
  void setBreakpoints(BreakpointSet *breakpoints);

signals:
  void breakpointsChanged();

private:
  BreakpointSet *_breakpoints = nullptr;
};

class Debugger {
public:
  Debugger();
  ~Debugger() = default;
  std::unique_ptr<BreakpointSet> bps = nullptr;
};
}; // namespace pepp::debug
