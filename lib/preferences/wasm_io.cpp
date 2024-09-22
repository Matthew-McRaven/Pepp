#include "wasm_io.hpp"
#include <QFileDialog>

WASMIO::WASMIO(QObject *parent) : QObject(parent) {}

void WASMIO::save(const QString &filename, const QString &data) {
  QFileInfo file(filename);
  QFileDialog::saveFileContent(data.toUtf8(), file.fileName());
}

void WASMIO::load(const QString &nameFilter) {
  auto ready = [this](const QString &fileName, const QByteArray &fileContent) {
    if (!fileName.isEmpty()) {
      _loadedName = fileName;
      _loadedData = QString::fromUtf8(fileContent);
      emit loaded();
    }
  };
  QFileDialog::getOpenFileContent(nameFilter, ready);
}

QString WASMIO::loadedData() const { return _loadedData; }

QString WASMIO::loadedName() const { return _loadedName; }
