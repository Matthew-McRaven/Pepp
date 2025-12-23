/*
 * Copyright (c) 2024-2026 J. Stanley Warford, Matthew McRaven
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once
#include <QObject>
#include <QtQmlIntegration>
namespace pepp::settings {

class PaletteRoleHelper : public QObject {
  Q_GADGET
  QML_NAMED_ELEMENT(PaletteRole)
  QML_UNCREATABLE("Error:Only enums")

public:
  enum class Role : uint32_t {
    BaseRole = 0,
    BaseMonoRole,
    WindowRole,
    ButtonRole,
    HighlightRole,
    TooltipRole,
    AlternateBaseRole,
    AccentRole,
    LightRole,
    MidLightRole,
    MidRole,
    DarkRole,
    ShadowRole,
    LinkRole,
    LinkVisitedRole,
    BrightTextRole,
    PlaceHolderTextRole,

    //  Editor enums
    MnemonicRole,
    SymbolRole,
    DirectiveRole,
    MacroRole,
    CharacterRole,
    StringRole,
    CommentRole,
    RowNumberRole,
    BreakpointRole,
    ErrorRole,
    WarningRole,

    //  Circuit enums
    SeqCircuitRole,
    CircuitGreenRole,

    Total, // Must be last valid theme
    //  Indicates invalid state from parsing input files
    Invalid = 0xffffffff,
  };
  Q_ENUM(Role)
  PaletteRoleHelper(QObject *parent = nullptr);
  // Convert category to a string which is easy to parse for a program
  // e.g., Role::BaseMonoRole => "BaseMonoRole"
  Q_INVOKABLE static QString string(Role role);
  // Convert to a pretty string which looks nice to users.
  // e.g., Role::BaseMonoRole => "Base + Monospace font"
  Q_INVOKABLE static QString prettyString(Role role);
  Q_INVOKABLE static bool requiresMonoFont(Role role);
};
using PaletteRole = PaletteRoleHelper::Role;

class ValidPaletteParentModel : public QAbstractListModel {
  Q_OBJECT
  QML_CONSTRUCTIBLE_VALUE
  Q_PROPERTY(PaletteRole role READ role WRITE setRole NOTIFY roleChanged)
  QML_ELEMENT
public:
  explicit ValidPaletteParentModel(QObject *parent = nullptr);
  Q_INVOKABLE ValidPaletteParentModel(PaletteRole role);
  PaletteRole role() const;
  void setRole(PaletteRole role);
  int rowCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

signals:
  void roleChanged();

private:
  PaletteRole _role = PaletteRole::BaseRole;
};

class CategoryHelper : public QObject {
  Q_GADGET
  QML_NAMED_ELEMENT(Category)
  QML_UNCREATABLE("Error:Only enums")
public:
  enum class PaletteCategory : uint32_t {
    General,
    Editor,
    Circuit,
  };
  Q_ENUM(PaletteCategory)
  CategoryHelper(QObject *parent = nullptr);

  Q_INVOKABLE QString string(PaletteCategory cat) const;
};
using PaletteCategory = CategoryHelper::PaletteCategory;
PaletteCategory categoryForRole(PaletteRole role);

} // namespace pepp::settings
