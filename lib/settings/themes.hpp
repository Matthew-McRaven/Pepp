#pragma once
#include <QColor>
#include <QFont>
#include <QObject>
#include <QtQmlIntegration>

class Preference : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString name READ name CONSTANT)
  Q_PROPERTY(QColor foreground READ foreground WRITE setForeground NOTIFY preferenceChanged)
  Q_PROPERTY(QColor background READ background WRITE setBackground NOTIFY preferenceChanged)
  Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY preferenceChanged);
  Q_PROPERTY(bool bold READ bold WRITE setBold NOTIFY preferenceChanged);
  Q_PROPERTY(bool italics READ italics WRITE setItalics NOTIFY preferenceChanged);
  Q_PROPERTY(bool underline READ underline WRITE setUnderline NOTIFY preferenceChanged);
  Q_PROPERTY(bool strikeout READ strikeOut WRITE setStrikeOut NOTIFY preferenceChanged);
  QML_UNCREATABLE("")
  QML_ELEMENT

public:
  struct PreferenceOptions {
    bool bold = false;
    bool italics = false;
    bool underline = false;
    bool strikeOut = false;
    QColor fg = Qt::black;
    QColor bg = Qt::white;
    QFont font;
  };
  explicit Preference(const QString name, PreferenceOptions opts, QObject *parent = nullptr);
  int id() const { return 0; }

  QString name() const;
  void setName(const QString name);
  QColor foreground() const;
  void setForeground(const QColor foreground);
  QColor background() const;
  void setBackground(const QColor background);

  QFont font() const;
  void setFont(const QFont font);
  bool bold() const;
  void setBold(const bool bold);
  bool italics() const;
  void setItalics(const bool italics);
  bool underline() const;
  void setUnderline(const bool underline);
  bool strikeOut() const;
  void setStrikeOut(const bool strikeOut);

  QJsonObject toJson() const;
  bool updateFromJson(const QJsonObject &json);
  static Preference *fromJson(const QJsonObject &json);

signals:
  void preferenceChanged();

private:
  QString _name{};
  QColor _foreground{Qt::black};
  QColor _background{Qt::white};
  QFont _font;
};

class Theme : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("")
  Q_PROPERTY(QString name READ name NOTIFY preferenceChanged)
  //  See https://doc.qt.io/qt-6/qml-qtquick-colorgroup.html for color explanation
  Q_PROPERTY(Preference *base READ base NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *window READ window NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *button READ button NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *highlight READ highlight NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *tooltip READ tooltip NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *alternateBase READ alternateBase NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *accent READ accent NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *light READ light NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *midlight READ midlight NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *mid READ mid NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *dark READ dark NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *shadow READ shadow NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *link READ link NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *linkVisited READ linkVisited NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *brightText READ brightText NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *placeholderText READ placeholderText NOTIFY preferenceChanged)

  //  Custom colors
  Q_PROPERTY(Preference *symbol READ symbol NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *mnemonic READ mnemonic NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *directive READ directive NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *macro READ macro NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *character READ character NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *string READ string NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *comment READ comment NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *error READ error NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *warning READ warning NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *rowNumber READ rowNumber NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *breakpoint READ breakpoint NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *seqCircuit READ seqCircuit NOTIFY preferenceChanged)
  Q_PROPERTY(Preference *circuitGreen READ circuitGreen NOTIFY preferenceChanged)
public:
  enum class Roles : uint32_t {
    BaseRole = 0,
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
    SymbolRole,
    MnemonicRole,
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
  Q_ENUM(Roles)
  enum class Category : uint32_t {
    General,
    Editor,
    Circuit,
  };
  Q_ENUM(Category)

  QString name() const { return _name; };
  bool isSystem() const { return true; }

  Preference *preference(int role);
  Preference *preference(int role) const;
  Preference *preference(Roles role);
  Preference *preference(Roles role) const;

  Preference *base() const { return preference(Roles::BaseRole); }
  Preference *window() const { return preference(Roles::WindowRole); }
  Preference *button() const { return preference(Roles::ButtonRole); }
  Preference *highlight() const { return preference(Roles::HighlightRole); }
  Preference *tooltip() const { return preference(Roles::TooltipRole); }
  Preference *alternateBase() const { return preference(Roles::AlternateBaseRole); }
  Preference *accent() const { return preference(Roles::AccentRole); }
  Preference *light() const { return preference(Roles::LightRole); }
  Preference *midlight() const { return preference(Roles::MidLightRole); }
  Preference *mid() const { return preference(Roles::MidRole); }
  Preference *dark() const { return preference(Roles::DarkRole); }
  Preference *shadow() const { return preference(Roles::ShadowRole); }
  Preference *link() const { return preference(Roles::LinkRole); }
  Preference *linkVisited() const { return preference(Roles::LinkVisitedRole); }
  Preference *brightText() const { return preference(Roles::BrightTextRole); }
  Preference *placeholderText() const { return preference(Roles::PlaceHolderTextRole); }
  Q_INVOKABLE QList<Preference *> window_preferences() const { return {}; };

  Preference *symbol() const { return preference(Roles::SymbolRole); }
  Preference *mnemonic() const { return preference(Roles::MnemonicRole); }
  Preference *directive() const { return preference(Roles::DirectiveRole); }
  Preference *macro() const { return preference(Roles::MacroRole); }
  Preference *character() const { return preference(Roles::CharacterRole); }
  Preference *string() const { return preference(Roles::StringRole); }
  Preference *comment() const { return preference(Roles::CommentRole); }
  Preference *error() const { return preference(Roles::ErrorRole); }
  Preference *warning() const { return preference(Roles::WarningRole); }
  Preference *rowNumber() const { return preference(Roles::RowNumberRole); }
  Preference *breakpoint() const { return preference(Roles::BreakpointRole); }
  Q_INVOKABLE QList<Preference *> editor_preferences() const { return {}; };

  Preference *seqCircuit() const { return preference(Roles::SeqCircuitRole); }
  Preference *circuitGreen() const { return preference(Roles::CircuitGreenRole); }

  void resetToDefault();
  Preference *defaultForRole(Roles role);
  QJsonObject toJson() const;
  // Update the theme in place from a JSON object.
  bool updateFromJSON(const QJsonObject &json);
  // Create a (parentless) theme from the JSON at a given file.
  // Returns a nullptr if loading fails.
  static Theme *fromFile(QString path);
signals:
  void fontChanged();
  void preferenceChanged();

private:
  friend class ThemeModel;
  Theme(QObject *parent = nullptr);
  QString _name = {};
  enum Ranges : uint32_t {
    GeneralCategoryStart = (uint32_t)Roles::BaseRole,
    GeneralCategoryEnd = (uint32_t)Roles::SymbolRole,
    EditorCategoryStart = GeneralCategoryEnd,
    EditorCategoryEnd = (uint32_t)Roles::SeqCircuitRole,
    CircuitCategoryStart = EditorCategoryEnd,
    CircuitCategoryEnd = (uint32_t)Roles::Total,
  };
  QList<Preference *> _prefs;
  //  Dirty flag is cleared on save (a const function)
  mutable bool _isDirty{false};
  bool _isSystem{false};
  static const int _version{7};
};

class CategoryModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
public:
  explicit CategoryModel(QObject *parent = nullptr);
  int rowCount(const QModelIndex &parent) const override;
  bool removeRows(int row, int count, const QModelIndex &parent) override;
  QVariant data(const QModelIndex &index, int role) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QHash<int, QByteArray> roleNames() const override;
};

class ThemeModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT

public:
  explicit ThemeModel(QObject *parent = nullptr);
  int rowCount(const QModelIndex &parent) const override;
  bool removeRows(int row, int count, const QModelIndex &parent) override;
  QVariant data(const QModelIndex &index, int role) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QHash<int, QByteArray> roleNames() const override;
  // Q_INVOKABLE void loadFromDirectory(const QString &directory);
  // Q_INVOKABLE void loadFromJson(const QString &json);

private:
  QList<Theme *> loadSystemThemes();
  QList<Theme *> _themes;
};

class ThemeFilterModel : public QSortFilterProxyModel {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(Theme::Category category READ category WRITE setCategory NOTIFY categoryChanged)
public:
  explicit ThemeFilterModel(QObject *parent = nullptr);
  Theme::Category category() const;
  void setCategory(Theme::Category category);
signals:
  void categoryChanged();

protected:
  bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
  Theme::Category _cat = Theme::Category::General;
};
