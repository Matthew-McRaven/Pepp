#pragma once

#include <QObject>
#include <QFont>
#include <vector>

#include "preference.hpp"
#include "themes.hpp"

class PREFS_EXPORT Theme : public QObject {
  Q_OBJECT

  //  Location in registry or config file for Theme settings
  static const char *themeSettings;
  /*  See https://doc.qt.io/qt-6/qml-qtquick-colorgroup.html for
      color explanation

    First block of colors relates to standard QT palette
  */
  Q_PROPERTY(QFont   font  READ font WRITE setFont    NOTIFY fontChanged)
  Q_PROPERTY(Preference* base       READ base         NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* window     READ window       NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* button     READ button       NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* highlight  READ highlight    NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* tooltip    READ tooltip      NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* alternateBase READ alternateBase NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* accent     READ accent       NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* light      READ light        NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* midlight   READ midlight     NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* mid        READ mid          NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* dark       READ dark         NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* shadow       READ shadow     NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* link       READ link         NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* linkVisited READ linkVisited NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* brightText READ brightText   NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* placeholderText READ placeholderText NOTIFY preferenceChanged)

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
  Q_PROPERTY(Preference* breakpoint  READ breakpoint  NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* seqCircuit  READ seqCircuit  NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* circuitGreen READ circuitGreen NOTIFY preferenceChanged)

  Q_PROPERTY(QString name            READ name        NOTIFY themesChanged)
  Q_PROPERTY(bool systemTheme        READ systemTheme NOTIFY themesChanged)
  Q_PROPERTY(bool isDirty            READ isDirty     NOTIFY themesChanged)
  Q_PROPERTY(QStringList themes      READ themes      NOTIFY themesChanged)

  QString name_ = "Default";
  QString version_ = "0.6";
  bool system_{true};

  //  Dirty flag is cleared on save (a const function)
  mutable bool isDirty_{false};

  QFont font_;
  //  Preference is a QObject with a pointer to it's parent
  //  No smart pointers are necessary since child preferences will
  //  go out of scope with the parent class
  std::vector<Preference*> prefs_;

  //  Track all themes
  QString systemPath_, userPath_;
  QString currentTheme_;
  QStringList themes_;

public:
  enum Ranges {
    GeneralCategoryStart = Themes::BaseRole,  //  Only used for iteration
    GeneralCategoryEnd  = Themes::RowNumberRole, //  Only used for iteration
    EditorCategoryEnd   = Themes::SeqCircuitRole,
    CircuitCategoryEnd  = Themes::Total, //  Only used for iteration
  };

  explicit Theme(QObject *parent = nullptr);
  virtual ~Theme();

  //  Call back from QML to save specified theme
  Q_INVOKABLE void selectTheme(const QString file);
  Q_INVOKABLE void saveTheme();
  Q_INVOKABLE void copyTheme(const QString file);
  Q_INVOKABLE void importTheme(const QString file);
  Q_INVOKABLE void exportTheme(const QString file) const;
  Q_INVOKABLE void deleteTheme(const QString file);

  void load(const QString& file);
  bool save(const QString& file) const;
  bool isDirty() const;

  QFont       font() const;
  Preference* preference(int role);
  Preference* preference(int role) const;

  void setFont(QFont font);

  QString name() const;
  void setName(QString name);

  bool systemTheme() const;

  QStringList themes() const;

  //  Accessor when outside delegate
  Preference *base() const { return preference(Themes::Roles::BaseRole); }
  Preference *window() const { return preference(Themes::Roles::WindowRole); }
  Preference *button() const { return preference(Themes::Roles::ButtonRole); }
  Preference *highlight() const { return preference(Themes::Roles::HighlightRole); }
  Preference *tooltip() const { return preference(Themes::Roles::TooltipRole); }
  Preference *alternateBase() const { return preference(Themes::Roles::AlternateBaseRole); }
  Preference *accent() const { return preference(Themes::Roles::AccentRole); }
  Preference *light() const { return preference(Themes::Roles::LightRole); }
  Preference *midlight() const { return preference(Themes::Roles::MidLightRole); }
  Preference *mid() const { return preference(Themes::Roles::MidRole); }
  Preference *dark() const { return preference(Themes::Roles::DarkRole); }
  Preference *shadow() const { return preference(Themes::Roles::ShadowRole); }
  Preference *link() const { return preference(Themes::Roles::LinkRole); }
  Preference *linkVisited() const { return preference(Themes::Roles::LinkVisitedRole); }
  Preference *brightText() const { return preference(Themes::Roles::BrightTextRole); }
  Preference *placeholderText() const { return preference(Themes::Roles::PlaceHolderTextRole); }

  Preference *symbol() const { return preference(Themes::Roles::SymbolRole); }
  Preference *mnemonic() const { return preference(Themes::Roles::MnemonicRole); }
  Preference *directive() const { return preference(Themes::Roles::DirectiveRole); }
  Preference *macro() const { return preference(Themes::Roles::MacroRole); }
  Preference *character() const { return preference(Themes::Roles::CharacterRole); }
  Preference *string() const { return preference(Themes::Roles::StringRole); }
  Preference *comment() const { return preference(Themes::Roles::CommentRole); }
  Preference *error() const { return preference(Themes::Roles::ErrorRole); }
  Preference *warning() const { return preference(Themes::Roles::WarningRole); }
  Preference *rowNumber() const { return preference(Themes::Roles::RowNumberRole); }
  Preference *breakpoint() const { return preference(Themes::Roles::BreakpointRole); }

  Preference *seqCircuit() const { return preference(Themes::Roles::SeqCircuitRole); }
  Preference *circuitGreen() const { return preference(Themes::Roles::CircuitGreenRole); }
signals:
  void fontChanged();
  void preferenceChanged();
  void themesChanged();


public slots:
  void clearIsDirty();
  void setIsDirty();

private:
  QJsonObject toJson() const;
  void fromJson(const QJsonObject &json);

  void loadMissing();
  void loadThemeList();
  void setDirty(bool flag=true);
  void setFont(const QString family, const int pointSize);
};
