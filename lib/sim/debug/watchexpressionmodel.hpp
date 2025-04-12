#pragma once
#include <QAbstractTableModel>
#include <QtQmlIntegration>
#include "sim/debug/expr_ast.hpp"
#include "sim/debug/expr_parser.hpp"

namespace pepp::debug {
class WatchExpressionModel : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("");

public:
  explicit WatchExpressionModel(pepp::debug::Environment *env, QObject *parent = nullptr);
  std::span<const uint8_t> was_dirty() const;
  std::span<const std::shared_ptr<pepp::debug::Term>> root_terms() const;
  void add_root(std::shared_ptr<pepp::debug::Term>);
  void update_volatile_values();
  pepp::debug::Environment *env();
  // Compile an expression without adding it to roots
  std::shared_ptr<pepp::debug::Term> compile(const QString &new_expr);
  bool recompile(const QString &new_expr, int index);

private:
  pepp::debug::Environment *_env;
  pepp::debug::ExpressionCache _c;
  std::vector<uint8_t> _root_was_dirty;
  std::vector<std::shared_ptr<pepp::debug::Term>> _root_terms;
  std::vector<std::shared_ptr<pepp::debug::Term>> _volatiles;
};

class WatchExpressionRoles : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Error: only enums")

public:
  enum Roles {
    Changed = Qt::UserRole + 1,
    Italicize,
  };
  Q_ENUM(Roles)
};

class WatchExpressionTableModel : public QAbstractTableModel {
  Q_OBJECT
  Q_PROPERTY(WatchExpressionModel *watchExpressions READ expressionModel WRITE setExpressionModel NOTIFY
                 expressionModelChanged)
  QML_ELEMENT
public:
  explicit WatchExpressionTableModel(QObject *parent = nullptr);
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QHash<int, QByteArray> roleNames() const override;

  WatchExpressionModel *expressionModel();
  void setExpressionModel(WatchExpressionModel *new_model);
public slots:
  void onUpdateGUI();
signals:
  void expressionModelChanged();

private:
  // Not owning!! Do not delete!!
  WatchExpressionModel *_expressionModel = nullptr;
};
} // namespace pepp::debug
