#pragma once
#include <QMetaType>
#include <QObject>
#include <QtQmlIntegration>
#include "./constants.hpp"
#include "paletteitem.hpp"

namespace pepp::settings {
class PaletteCategoryModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
public:
  explicit PaletteCategoryModel(QObject *parent = nullptr);
  int rowCount(const QModelIndex &parent) const override;
  bool removeRows(int row, int count, const QModelIndex &parent) override;
  QVariant data(const QModelIndex &index, int role) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QHash<int, QByteArray> roleNames() const override;
};

class Palette : public QObject {
  Q_OBJECT
  QML_NAMED_ELEMENT(ExtendedPalette)

  //  See https://doc.qt.io/qt-6/qml-qtquick-colorgroup.html for color explanation
  Q_PROPERTY(PaletteItem *base READ base)
  Q_PROPERTY(PaletteItem *window READ window)
  Q_PROPERTY(PaletteItem *button READ button)
  Q_PROPERTY(PaletteItem *highlight READ highlight)
  Q_PROPERTY(PaletteItem *tooltip READ tooltip)
  Q_PROPERTY(PaletteItem *alternateBase READ alternateBase)
  Q_PROPERTY(PaletteItem *accent READ accent)
  Q_PROPERTY(PaletteItem *light READ light)
  Q_PROPERTY(PaletteItem *midlight READ midlight)
  Q_PROPERTY(PaletteItem *mid READ mid)
  Q_PROPERTY(PaletteItem *dark READ dark)
  Q_PROPERTY(PaletteItem *shadow READ shadow)
  Q_PROPERTY(PaletteItem *link READ link)
  Q_PROPERTY(PaletteItem *linkVisited READ linkVisited)
  Q_PROPERTY(PaletteItem *brightText READ brightText)
  Q_PROPERTY(PaletteItem *placeholderText READ placeholderText)

  // Editor
  Q_PROPERTY(PaletteItem *symbol READ symbol)
  Q_PROPERTY(PaletteItem *mnemonic READ mnemonic)
  Q_PROPERTY(PaletteItem *directive READ directive)
  Q_PROPERTY(PaletteItem *macro READ macro)
  Q_PROPERTY(PaletteItem *character READ character)
  Q_PROPERTY(PaletteItem *string READ string)
  Q_PROPERTY(PaletteItem *comment READ comment)
  Q_PROPERTY(PaletteItem *error READ error)
  Q_PROPERTY(PaletteItem *warning READ warning)
  Q_PROPERTY(PaletteItem *rowNumber READ rowNumber)
  Q_PROPERTY(PaletteItem *breakpoint READ breakpoint)

  // Circuit
  Q_PROPERTY(PaletteItem *seqCircuit READ seqCircuit)
  Q_PROPERTY(PaletteItem *circuitGreen READ circuitGreen)
public:
  Palette(QObject *parent = nullptr);
  // Return -1 if not found, or (int) PaletteRole if found.
  Q_INVOKABLE int itemToRole(const PaletteItem *item) const;

  PaletteItem *item(int role);
  PaletteItem *item(int role) const;
  Q_INVOKABLE PaletteItem *item(PaletteRole role);
  PaletteItem *item(PaletteRole role) const;

  // General
  PaletteItem *base() const { return item(PaletteRole::BaseRole); }
  PaletteItem *window() const { return item(PaletteRole::WindowRole); }
  PaletteItem *button() const { return item(PaletteRole::ButtonRole); }
  PaletteItem *highlight() const { return item(PaletteRole::HighlightRole); }
  PaletteItem *tooltip() const { return item(PaletteRole::TooltipRole); }
  PaletteItem *alternateBase() const { return item(PaletteRole::AlternateBaseRole); }
  PaletteItem *accent() const { return item(PaletteRole::AccentRole); }
  PaletteItem *light() const { return item(PaletteRole::LightRole); }
  PaletteItem *midlight() const { return item(PaletteRole::MidLightRole); }
  PaletteItem *mid() const { return item(PaletteRole::MidRole); }
  PaletteItem *dark() const { return item(PaletteRole::DarkRole); }
  PaletteItem *shadow() const { return item(PaletteRole::ShadowRole); }
  PaletteItem *link() const { return item(PaletteRole::LinkRole); }
  PaletteItem *linkVisited() const { return item(PaletteRole::LinkVisitedRole); }
  PaletteItem *brightText() const { return item(PaletteRole::BrightTextRole); }
  PaletteItem *placeholderText() const { return item(PaletteRole::PlaceHolderTextRole); }

  // Editor
  PaletteItem *symbol() const { return item(PaletteRole::SymbolRole); }
  PaletteItem *mnemonic() const { return item(PaletteRole::MnemonicRole); }
  PaletteItem *directive() const { return item(PaletteRole::DirectiveRole); }
  PaletteItem *macro() const { return item(PaletteRole::MacroRole); }
  PaletteItem *character() const { return item(PaletteRole::CharacterRole); }
  PaletteItem *string() const { return item(PaletteRole::StringRole); }
  PaletteItem *comment() const { return item(PaletteRole::CommentRole); }
  PaletteItem *error() const { return item(PaletteRole::ErrorRole); }
  PaletteItem *warning() const { return item(PaletteRole::WarningRole); }
  PaletteItem *rowNumber() const { return item(PaletteRole::RowNumberRole); }
  PaletteItem *breakpoint() const { return item(PaletteRole::BreakpointRole); }

  // Circuit
  PaletteItem *seqCircuit() const { return item(PaletteRole::SeqCircuitRole); }
  PaletteItem *circuitGreen() const { return item(PaletteRole::CircuitGreenRole); }

  bool updateFromJson(const QJsonObject &json);
  QJsonObject toJson();

private:
  //  Dirty flag is cleared on save (a const function)
  mutable bool _isDirty{false};
  static const int _version{8};
  QList<PaletteItem *> _items;

  void loadDefaults();
};

} // namespace pepp::settings
