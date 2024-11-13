#pragma once
#include <QColor>
#include <QFont>
#include <QObject>
#include <QtQmlIntegration>

namespace pepp::settings {
class OverrideStateHelper : public QObject {
  Q_GADGET
  QML_NAMED_ELEMENT(OverrideState)
  QML_UNCREATABLE("Error:Only enums")

public:
  enum class OverrideState : uint32_t {
    True,
    False,
    Unset,
  };
  Q_ENUM(OverrideState)
  OverrideStateHelper(QObject *parent = nullptr);
};
using OverrideState = OverrideStateHelper::OverrideState;

class PaletteItem : public QObject {
  Q_OBJECT
  Q_PROPERTY(PaletteItem *parent READ parent WRITE setParent NOTIFY preferenceChanged)
  Q_PROPERTY(QColor foreground READ foreground WRITE setForeground NOTIFY preferenceChanged)
  Q_PROPERTY(QColor background READ background WRITE setBackground NOTIFY preferenceChanged)
  Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY preferenceChanged);
  Q_PROPERTY(bool hasOwnForeground READ hasOwnForeground NOTIFY preferenceChanged)
  Q_PROPERTY(bool hasOwnBackground READ hasOwnBackground NOTIFY preferenceChanged)
  Q_PROPERTY(bool hasOwnFont READ hasOwnFont NOTIFY preferenceChanged)
  Q_PROPERTY(OverrideState boldOverride READ boldOverride WRITE setBoldOverride NOTIFY preferenceChanged)
  Q_PROPERTY(OverrideState italicOverride READ italicOverride WRITE setItalicOverride NOTIFY preferenceChanged)
  Q_PROPERTY(OverrideState underlineOverride READ underlineOverride WRITE setUnderlineOverride NOTIFY preferenceChanged)
  Q_PROPERTY(OverrideState strikeoutOverride READ strikeoutOverride WRITE setStrikeoutOverride NOTIFY preferenceChanged)
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
  OverrideState boldOverride() const;
  void setBoldOverride(OverrideState bold);
  OverrideState italicOverride() const;
  void setItalicOverride(OverrideState italic);
  OverrideState underlineOverride() const;
  void setUnderlineOverride(OverrideState underline);
  OverrideState strikeoutOverride() const;
  void setStrikeoutOverride(OverrideState strikeout);

signals:
  void fontChanged();
  void preferenceChanged();

public slots:
  void onParentChanged();

private:
  PaletteItem *_parent{nullptr};
  std::optional<QColor> _foreground{Qt::black};
  std::optional<QColor> _background{Qt::white};
  std::optional<QFont> _font;
  struct FontOverride {
    std::optional<bool> strikeout, bold, underline, italic;
    int weight;
  } _fontOverrides;
};
namespace detail {
bool isAncestorOf(const PaletteItem *maybeAncestor, const PaletteItem *maybeDescendant);
QJsonObject toJson();
bool updateFromJson(const QJsonObject &json);
static PaletteItem *fromJson(const QJsonObject &json);
} // namespace detail

} // namespace pepp::settings
