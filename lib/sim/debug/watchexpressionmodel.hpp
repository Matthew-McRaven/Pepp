#pragma once
#include <QAbstractTableModel>
#include <QtQmlIntegration>
#include "sim/debug/expr_ast.hpp"
#include "sim/debug/expr_parser.hpp"

namespace pepp::debug {
class WatchExpressionEditor : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("");

public:
  struct Item {
    using TermPtr = std::shared_ptr<pepp::debug::Term>;
    explicit Item() = default;
    // Call if compilation was a success. Update type manually after construction.
    explicit Item(TermPtr term);
    // Call if compilation failed. Update type manually after construction.
    explicit Item(QString term);
    ~Item() = default;
    pepp::debug::Term *term();
    void set_term(TermPtr term);
    void set_term(QString term);
    void evaluate(pepp::debug::CachePolicy policy, pepp::debug::Environment &env);
    void clear_value();
    std::optional<TypedBits> value() const;

    bool needs_update() const;
    bool dirty() const;
    void make_dirty();
    void make_clean(bool force_update = false);

    bool is_wip() const;
    QString expression_text() const;
    QString type_text() const;

  private:
    bool _dirty = false;       // On most recent update to update_volatile_values, was the term dirty?
    bool _needs_update = true; // Is the term: (currently dirty) | (dirty prior to update_volatile_values)
    QString _wip_term = "";    // Most recent text submitted to <> iff compilation failed.
    QString _wip_type = "";    // Most recent text submitted to the type compiler iff compliation failed.
    TermPtr _term = nullptr;   // The term itself, if compilation succeeded. Nullptr otherwise.
    // If term != nullptr && wip_type.empty() && type: promote terms result to this type.
    // If term != nullptr && wip_type.empty() && !type: use terms result type.
    // If term != nullptr && !wip_type.empty(): do not render value, and place <invalid> in type field.
    std::optional<ExpressionType> _type = std::nullopt;
    std::optional<TypedBits> _recent_value = std::nullopt; // Most recent value of the term.
  };
  explicit WatchExpressionEditor(pepp::debug::ExpressionCache *cache, pepp::debug::Environment *env,
                                 QObject *parent = nullptr);
  std::span<const Item> items() const;
  void add_item(const QString &new_expr, const QString new_type = "");
  bool edit_term(int index, const QString &new_expr);
  bool edit_type(int index, const QString &new_type);
  bool delete_at(int index);
  void update_volatile_values();

public slots:
  void onSimulationStart();
signals:
  void fullUpdateModel();

private:
  struct VolatileCache {
    // Preserve last evaluated value of the term.
    // A volatile may have been updated many times by the breakpoint system before we've been given a chacne to view it.
    // The term itself may not be dirty, but from our perspective it is different than its last rendered value.
    TypedBits v;
    std::shared_ptr<pepp::debug::Term> term;
  };
  void gather_volatiles();
  pepp::debug::Environment *_env;
  pepp::debug::ExpressionCache *_cache;
  std::vector<VolatileCache> _volatiles;
  std::vector<Item> _items;
};

QVariant variant_from_bits(const pepp::debug::TypedBits &bits);

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
  Q_PROPERTY(WatchExpressionEditor *watchExpressions READ expressionModel WRITE setExpressionModel NOTIFY
                 expressionModelChanged)
  QML_ELEMENT
public:
  explicit WatchExpressionTableModel(QObject *parent = nullptr);
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role) override;
  bool removeRows(int row, int count, const QModelIndex &parent) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QHash<int, QByteArray> roleNames() const override;

  WatchExpressionEditor *expressionModel();
  void setExpressionModel(WatchExpressionEditor *new_model);
public slots:
  void onUpdateModel();
  void onFullUpdateModel();
signals:
  void expressionModelChanged();

private:
  // Not owning!! Do not delete!!
  WatchExpressionEditor *_expressionModel = nullptr;
};
} // namespace pepp::debug
