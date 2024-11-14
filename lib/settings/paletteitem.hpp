#pragma once
#include <QColor>
#include <QFont>
#include <QObject>
#include <QtQmlIntegration>

namespace pepp::settings {
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
  struct PreferenceOptions {
    PaletteItem *parent = nullptr;
    QColor fg = Qt::black;
    QColor bg = Qt::white;
    QFont font;
  };
  explicit PaletteItem(PreferenceOptions opts, QObject *parent = nullptr);
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
  void setFont(const QFont font);

  bool hasOwnForeground() const;
  bool hasOwnBackground() const;
  bool hasOwnFont() const;
  Q_INVOKABLE void overrideBold(bool bold);
  Q_INVOKABLE void overrideItalic(bool italic);
  Q_INVOKABLE void overrideUnderline(bool underline);
  Q_INVOKABLE void overrideStrikeout(bool strikeout);

signals:
  void fontChanged();
  void preferenceChanged();

public slots:
  void onParentChanged();

private:
  PaletteItem *_parent{nullptr};
  std::optional<QColor> _foreground{std::nullopt};
  std::optional<QColor> _background{std::nullopt};
  std::optional<QFont> _font{std::nullopt};
  struct FontOverride {
    std::optional<bool> strikeout{std::nullopt}, bold{std::nullopt}, underline{std::nullopt}, italic{std::nullopt};
    std::optional<int> weight{std::nullopt};
  } _fontOverrides;
};
namespace detail {
bool isAncestorOf(const PaletteItem *maybeAncestor, const PaletteItem *maybeDescendant);
QJsonObject toJson();
bool updateFromJson(const QJsonObject &json);
static PaletteItem *fromJson(const QJsonObject &json);
} // namespace detail

} // namespace pepp::settings
