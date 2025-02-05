#include "palettemanager.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

pepp::settings::PaletteManager::PaletteManager(QObject *parent) : QAbstractListModel(parent) { reload(); }

int pepp::settings::PaletteManager::rowCount(const QModelIndex &parent) const { return _palettes.size(); }

QVariant pepp::settings::PaletteManager::data(const QModelIndex &index, int role) const
{
  if (!index.isValid() || index.row() < 0 || index.row() >= _palettes.size()) return {};
  const auto &entry = _palettes[index.row()];
  switch (role) {
  case Qt::DisplayRole: return entry.name;
  case (int)Role::PathRole: return entry.path;
  case (int)Role::IsSystemRole: return entry.isSystem;
  default: return {};
  }
}

bool pepp::settings::PaletteManager::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (!index.isValid() || index.row() < 0 || index.row() >= _palettes.size()) return false;
  else if (role != Qt::DisplayRole) return false;
  else if (!value.canConvert<QString>()) return false;
  else if (auto asString = value.toString(); asString.isEmpty()) return false;
  else {
    auto &entry = _palettes[index.row()];
    QFile jsonFile(entry.path);
    if (!jsonFile.open(QIODevice::ReadOnly)) return false;
    auto ba = jsonFile.readAll();
    jsonFile.close();
    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(ba, &parseError);
    // Don't try to update doc in place; seems to not persist name change.
    auto root = doc.object();
    root["name"] = asString;
    if (!jsonFile.open(QIODevice::WriteOnly)) return false;
    jsonFile.write(QJsonDocument{root}.toJson());
    jsonFile.close();
#ifdef __EMSCRIPTEN__
    EM_ASM(FS.syncfs(function(){}););
#endif
    entry.name = asString;
  }
  emit dataChanged(index, index, {role});
  return true;
}

Qt::ItemFlags pepp::settings::PaletteManager::flags(const QModelIndex &index) const
{
  auto isSystem = data(index, (int)Role::IsSystemRole);
  if (isSystem.isValid() && isSystem.toBool()) return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  else return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QHash<int, QByteArray> pepp::settings::PaletteManager::roleNames() const
{
  return {{Qt::DisplayRole, "display"}, {(int)Role::PathRole, "path"}, {(int)Role::IsSystemRole, "isSystem"}};
}

// Can't be static, because we need information from main();
QString userThemeDir() {
#ifdef Q_OS_WASM
  return "/themes";
#else
  return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/themes";
#endif
}
void pepp::settings::PaletteManager::reload()
{
  beginResetModel();
  _palettes.clear();
  loadFrom(":/themes");
  loadFrom(userThemeDir());
  endResetModel();
}

int pepp::settings::PaletteManager::copy(int row) {
  if (row < 0 || row >= _palettes.size()) return -1;
  auto entry = _palettes[row];
  // TODO: if copy of copy, append - Copy 2, 3 instead.
  entry.name += " - Copy";
  QFileInfo targetFile;
  if (entry.isSystem) targetFile.setFile(userThemeDir(), entry.name + ".theme");
  else targetFile.setFile(targetFile.absolutePath(), entry.name + ".theme");
  entry.path = targetFile.absoluteFilePath();
  entry.isSystem = false;
  // Maye need to create the directory or copy will fail.
  QDir().mkpath(userThemeDir());
  if (!QFile::copy(_palettes[row].path, entry.path)) return -1;
  // Make sure we can write to this file later to update it.
  auto perms = targetFile.permissions();
  QFile(targetFile.absoluteFilePath()).setPermissions(perms | QFileDevice::WriteOwner);
#ifdef __EMSCRIPTEN__
  EM_ASM(FS.syncfs(function(){}););
#endif

  beginInsertRows({}, _palettes.size() - 1, _palettes.size() - 1);
  _palettes.append(entry);
  endInsertRows();
  return _palettes.size() - 1;
}

int pepp::settings::PaletteManager::importTheme(QString path) {
  QFileInfo src(path);
  if (!src.fileName().endsWith(".theme")) return -1;
  QFile jsonFile(src.absoluteFilePath());
  if (!jsonFile.open(QIODevice::ReadOnly)) return -1;
  auto ba = jsonFile.readAll();
  jsonFile.close();
  QJsonParseError parseError;
  auto doc = QJsonDocument::fromJson(ba, &parseError);

  QString name{};
  if (auto namePtr = doc["name"]; namePtr.isString()) name = namePtr.toString();
  else name = src.completeBaseName();

  QFileInfo dest(userThemeDir(), src.fileName());
  // Maye need to create the directory or copy will fail.
  QDir().mkpath(userThemeDir());
  if (!QFile::copy(path, dest.absoluteFilePath())) return -1;
  // Make sure we can write to this file later to update it.
  auto perms = dest.permissions();
  QFile(dest.absoluteFilePath()).setPermissions(perms | QFileDevice::WriteOwner);

#ifdef __EMSCRIPTEN__
  EM_ASM(FS.syncfs(function(){}););
#endif

  beginInsertRows({}, _palettes.size() - 1, _palettes.size() - 1);
  _palettes.append({.name = name, .path = dest.absoluteFilePath(), .isSystem = false});
  endInsertRows();
  return _palettes.size() - 1;
}

void pepp::settings::PaletteManager::deleteTheme(int row) {
  if (row < 0 || row >= _palettes.size()) return;
  auto entry = _palettes[row];
  if (!entry.isSystem) QFile::remove(entry.path);
#ifdef __EMSCRIPTEN__
  EM_ASM(FS.syncfs(function(){}););
#endif
  beginRemoveRows({}, row, row);
  _palettes.removeAt(row);
  endRemoveRows();
}

void pepp::settings::PaletteManager::loadFrom(QString directory) {
  QFileInfoList list = QDir(directory).entryInfoList(QDir::Files);
  // qDebug() << "Loading themes from " << directory << " found " << list.size();
  for (auto &file : list) {
    if (!file.fileName().endsWith(".theme")) continue;
    QFile jsonFile(file.absoluteFilePath());
    if (!jsonFile.open(QIODevice::ReadOnly)) continue;
    auto ba = jsonFile.readAll();
    jsonFile.close();
    QJsonParseError parseError;
    auto doc = QJsonDocument::fromJson(ba, &parseError);

    QString name{};
    if (auto namePtr = doc["name"]; namePtr.isString()) name = namePtr.toString();
    else name = file.completeBaseName();

    _palettes.append({.name = name, .path = file.absoluteFilePath(), .isSystem = !file.isWritable()});
  }
}
