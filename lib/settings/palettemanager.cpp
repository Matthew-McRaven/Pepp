#include "palettemanager.hpp"

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

void pepp::settings::PaletteManager::reload()
{
  emit beginResetModel();
  _palettes.clear();
  loadFrom(":/themes");
  loadFrom(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/themes");
  emit endResetModel();
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
    else name = file.fileName();

    _palettes.append({.name = name, .path = file.absoluteFilePath(), .isSystem = !file.isWritable()});
  }
}
