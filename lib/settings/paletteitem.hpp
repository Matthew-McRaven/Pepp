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
#include <QColor>
#include <QFont>
#include <QObject>
#include <QtQmlIntegration>
#include "./constants.hpp"

namespace pepp::settings {

QFont default_ui_font();
QFont default_mono_font();
class PaletteItem : public QObject {
  Q_OBJECT
  Q_PROPERTY(PaletteItem *parent READ parent WRITE setParent NOTIFY preferenceChanged)
  Q_PROPERTY(QColor foreground READ foreground WRITE setForeground NOTIFY preferenceChanged)
  Q_PROPERTY(QColor background READ background WRITE setBackground NOTIFY preferenceChanged)
  Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY preferenceChanged);
  Q_PROPERTY(bool hasOwnForeground READ hasOwnForeground NOTIFY preferenceChanged)
  Q_PROPERTY(bool hasOwnBackground READ hasOwnBackground NOTIFY preferenceChanged)
  Q_PROPERTY(bool hasOwnFont READ hasOwnFont NOTIFY preferenceChanged)
  QML_UNCREATABLE("")
  QML_ELEMENT

public:
  struct Options {
    PaletteItem *parent = nullptr;
    std::optional<QColor> fg{std::nullopt};
    std::optional<QColor> bg{std::nullopt};
    std::optional<QFont> font{std::nullopt};
  };
  explicit PaletteItem(Options opts, PaletteRole ownRole, QObject *parent = nullptr);
  PaletteRole ownRole() const;
  PaletteItem *parent();
  const PaletteItem *parent() const;
  Q_INVOKABLE void clearParent();
  // May fail if setting parent would induce a cycle in reltaionship graph.
  void setParent(PaletteItem *newParent);
  QColor foreground() const;
  Q_INVOKABLE void clearForeground();
  void setForeground(const QColor foreground);
  QColor background() const;
  Q_INVOKABLE void clearBackground();
  void setBackground(const QColor background);
  QFont font() const;
  Q_INVOKABLE void clearFont();
  void setFont(const QFont font);

  bool hasOwnForeground() const;
  bool hasOwnBackground() const;
  bool hasOwnFont() const;
  Q_INVOKABLE void overrideBold(bool bold);
  Q_INVOKABLE void overrideItalic(bool italic);
  Q_INVOKABLE void overrideUnderline(bool underline);
  Q_INVOKABLE void overrideStrikeout(bool strikeout);

  virtual bool updateFromJson(const QJsonObject &json, PaletteRole ownRole = PaletteRole::Invalid,
                              PaletteItem *parent = nullptr);
  virtual QJsonObject toJson();
  virtual void updateFromSettings(QSettings &settings, PaletteItem *parent = nullptr);
  virtual void toSettings(QSettings &settings) const;
signals:
  void preferenceChanged();

public slots:
  void onParentChanged();
  void emitChanged();

protected:
  PaletteItem *_parent{nullptr};
  std::optional<QColor> _foreground{std::nullopt};
  std::optional<QColor> _background{std::nullopt};
  std::optional<QFont> _font{std::nullopt};
  struct FontOverride {
    std::optional<bool> strikeout{std::nullopt}, bold{std::nullopt}, underline{std::nullopt}, italic{std::nullopt};
    std::optional<int> weight{std::nullopt};
  } _fontOverrides;
  // Apply new font to this item if it doesn't violate monospace requirements.
  void updateFont(const QFont newFont);
  // Something about our parent changed... make sure we don't inherit a non-mono font.
  void preventNonMonoParent();
  PaletteRole _ownRole;
};

class EditorPaletteItem : public PaletteItem {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QFont macroFont READ macroFont WRITE setMacroFont NOTIFY preferenceChanged);
  QML_UNCREATABLE("EditorPaletteItem is only creatable from C++")

public:
  struct EditorOptions {
    std::optional<QFont> macroFont{std::nullopt};
  };
  explicit EditorPaletteItem(EditorOptions editor, Options base, PaletteRole ownRole, QObject *parent = nullptr);

  QFont macroFont() const;
  Q_INVOKABLE void clearMacroFont();
  bool hasOwnMacroFont() const;
  void setMacroFont(const QFont font);

  bool updateFromJson(const QJsonObject &json, PaletteRole ownRole, PaletteItem *parent) override;
  QJsonObject toJson() override;
  void updateFromSettings(QSettings &settings, PaletteItem *parent) override;
  void toSettings(QSettings &settings) const override;

protected:
  std::optional<QFont> _macroFont{std::nullopt};
  void updateMacroFont(const QFont newFont);
};

namespace detail {
bool isAncestorOf(const PaletteItem *maybeAncestor, const PaletteItem *maybeDescendant);
} // namespace detail
} // namespace pepp::settings
