#pragma once
#include <QtCore>
#include <QtQmlIntegration>
#include <bitset>
#include "sim/debug/expr_parser.hpp"
#include "sim/debug/line_map.hpp"
#include "symtab/symbolmodel.hpp"
#include "watchexpressionmodel.hpp"

namespace pepp::debug {
class BreakpointSet : public QObject {
  Q_OBJECT
public:
  explicit BreakpointSet();
  void addBP(quint16 address, pepp::debug::Term *condition = nullptr);
  void removeBP(quint16 address);
  bool hasBP(quint16 address) const;
  void clearBPs();
  void notifyPCChanged(quint16 newValue);
  std::size_t count() const;

  std::span<quint16> breakpoints();
  std::span<pepp::debug::Term *> conditions();

  bool hit() const;
  void clearHit();
signals:
  void breakpointAdded(quint16 address);
  void breakpointRemoved(quint16 address);
  void breakpointsCleared();

private:
  // Used to enable constant-time detection of the common case (no breakpoints).
  // Packs an 8-byte interval into each bit, and a 64-byte per interval per byte.
  // Should consume ~1024 bytes of memory total for Pepp procesors.
  std::bitset<0x1'00'00 / 8> _bitmask;
  std::vector<quint16> _breakpoints;
  std::vector<pepp::debug::Term *> _conditions;
  pepp::debug::Environment *_env;
  bool _hit = false;
};

class BreakpointTableModel : public QAbstractTableModel {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(pepp::debug::BreakpointSet *breakpointModel READ breakpointModel WRITE setBreakpointModel NOTIFY
                 breakpointModelChanged)
  Q_PROPERTY(ScopedLines2Addresses *lines2address READ lines2address WRITE setLines2Address NOTIFY lines2addressChanged)

public:
  explicit BreakpointTableModel(QObject *parent = nullptr);
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role) override;
  bool removeRows(int row, int count, const QModelIndex &parent) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;

  BreakpointSet *breakpointModel();
  void setBreakpointModel(BreakpointSet *breakpoints);
  ScopedLines2Addresses *lines2address();
  void setLines2Address(ScopedLines2Addresses *lines2address);

public slots:
  void onBreakpointsChanged();
signals:
  void breakpointModelChanged();
  void lines2addressChanged();

private:
  QMap<quint16, WatchExpressionEditor::Item> _conditionEditor;
  BreakpointSet *_breakpoints = nullptr;
  ScopedLines2Addresses *_lines2address = nullptr;
};

class Environment;
class ExpressionCache;
class Debugger {
public:
  explicit Debugger(pepp::debug::Environment *env);
  ~Debugger() = default;
  pepp::debug::Environment *env = nullptr;
  std::unique_ptr<pepp::debug::BreakpointSet> bps = nullptr;
  std::unique_ptr<pepp::debug::ExpressionCache> cache = nullptr;
  std::unique_ptr<pepp::debug::WatchExpressionEditor> watch_expressions = nullptr;
  std::unique_ptr<ScopedLines2Addresses> line_maps = nullptr;
  std::unique_ptr<StaticSymbolModel> static_symbol_model = nullptr;
};
}; // namespace pepp::debug
