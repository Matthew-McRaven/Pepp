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
  explicit WatchExpressionModel(QObject *parent = nullptr);
  std::span<const std::shared_ptr<pepp::debug::Term>> root_terms() const;
  void add_root(std::shared_ptr<pepp::debug::Term>);
  pepp::debug::Environment &env();

private:
  pepp::debug::Environment _env;
  pepp::debug::ExpressionCache _c;
  std::vector<std::shared_ptr<pepp::debug::Term>> _root_terms;
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
  QHash<int, QByteArray> roleNames() const override;

  WatchExpressionModel *expressionModel();
  void setExpressionModel(WatchExpressionModel *new_model);

signals:
  void expressionModelChanged();

private:
  // Not owning!! Do not delete!!
  WatchExpressionModel *_expressionModel = nullptr;
};
} // namespace pepp::debug
