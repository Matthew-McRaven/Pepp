#ifndef THEME_HPP
#define THEME_HPP

#include <QObject>
#include <QFont>
#include <vector>

#include "preference.hpp"

class Theme : public QObject {
  Q_OBJECT

  Q_PROPERTY(QFont   font  READ font WRITE setFont    NOTIFY fontChanged)
  Q_PROPERTY(Preference* surface     READ surface     NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* container   READ container   NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* primary     READ primary     NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* secondary   READ secondary   NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* tertiary    READ tertiary    NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* error       READ error       NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* warning     READ warning     NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* rowNumber   READ rowNumber   NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* breakpoint  READ breakpoint  NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* seqCircuit  READ seqCircuit  NOTIFY preferenceChanged)
  Q_PROPERTY(Preference* circuitGreen READ circuitGreen NOTIFY preferenceChanged)
  Q_PROPERTY(QString name            READ name        NOTIFY themesChanged)
  Q_PROPERTY(bool systemTheme        READ systemTheme NOTIFY themesChanged)
  Q_PROPERTY(QStringList themes      READ themes      NOTIFY themesChanged)

  QString name_ = "Default";
  QString version_ = "0.2";
  bool system_{false};
  QFont font_;
  std::vector<Preference*> prefs_;

  //  Track all themes
  QString systemPath_, userPath_;
  QStringList themes_;

public:
  enum Roles {
    SurfaceRole = 0,
    ContainerRole,
    PrimaryRole,
    SecondaryRole,
    TertiaryRole,
    ErrorRole,
    WarningRole,

    RowNumberRole,
    BreakpointRole,

    SeqCircuitRole,
    CircuitGreenRole,

    Total, // Must be last
  };

  explicit Theme(QObject *parent = nullptr);

  //  Call back from QML to save specified theme
  Q_INVOKABLE void selectTheme(const QString file);
  Q_INVOKABLE void copyTheme(const QString file);
  Q_INVOKABLE void importTheme(const QString file);
  Q_INVOKABLE void exportTheme(const QString file) const;
  Q_INVOKABLE void deleteTheme(const QString file);

  void load(const QString& file);
  bool save(const QString& file) const;

  QFont       font() const;
  Preference* preference(int role);
  Preference* preference(int role) const;

  void setFont(QFont font);

  QString name() const;
  void setName(QString name);

  bool systemTheme() const;

  QStringList themes() const;

  //  Accessor when outside delegate
  Preference* surface() const {
    return preference(Theme::Roles::SurfaceRole);  }
  Preference* container() const {
    return preference(Theme::Roles::ContainerRole); }
  Preference* primary() const {
    return preference(Theme::Roles::PrimaryRole); }
  Preference* secondary() const {
    return preference(Theme::Roles::SecondaryRole); }
  Preference* tertiary() const {
    return preference(Theme::Roles::TertiaryRole); }
  Preference* error() const {
    return preference(Theme::Roles::ErrorRole); }
  Preference* warning() const {
    return preference(Theme::Roles::WarningRole); }
  Preference* rowNumber() const {
    return preference(Theme::Roles::RowNumberRole); }
  Preference* breakpoint() const {
    return preference(Theme::Roles::BreakpointRole); }
  Preference* seqCircuit() const {
    return preference(Theme::Roles::SeqCircuitRole); }
  Preference* circuitGreen() const {
    return preference(Theme::Roles::CircuitGreenRole); }

signals:
  void fontChanged();
  void preferenceChanged();
  void themesChanged();

private:
  QJsonObject toJson() const;
  void fromJson(const QJsonObject &json);

  void loadMissing();
  void loadThemeList();
};

#endif // THEME_HPP
