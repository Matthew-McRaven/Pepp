#pragma once
#include <QObject>
#include <QtQmlIntegration>
#include "./constants.hpp"

namespace pepp::settings {
class Palette;
class PaletteModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  QML_STRUCTURED_VALUE
  Q_PROPERTY(Palette *palette READ palette WRITE setPalette NOTIFY paletteChanged)

public:
  enum class Role : int {
    // Role which contails PaletteRole for a given item... sorry for the name.
    PaletteRoleRole = Qt::UserRole + 1,
    PaletteItemRole,
    RequiresMonoFontRole,
  };
  explicit PaletteModel(QObject *parent = nullptr);
  explicit Q_INVOKABLE PaletteModel(Palette *palette);
  Palette *palette() const;
  void setPalette(Palette *palette);
  int rowCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;
signals:
  void paletteChanged();

private:
  Palette *_palette{nullptr};
};

class PaletteFilterModel : public QSortFilterProxyModel {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QVariant category READ category WRITE setCategory NOTIFY categoryChanged)
public:
  explicit PaletteFilterModel(QObject *parent = nullptr);
  QVariant category() const;
  void setCategory(QVariant category);
signals:
  void categoryChanged();

protected:
  bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
  std::optional<PaletteCategory> _cat = std::nullopt;
};
} // namespace pepp::settings
