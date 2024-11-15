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
      QFileInfo fileInfo(fileName);
      if (!QDir("/tmp/").exists()) QDir().mkdir("/tmp/");
      QString dest = "/tmp/" + fileInfo.fileName();
      _loadedName = "file:////tmp/" + fileInfo.fileName();
      QFile file(dest);
      if (!file.open(QIODevice::WriteOnly)) throw std::runtime_error("Could not open file for writing");
      file.write(fileContent);
      file.close();
      emit loaded();
    }
  };
  QFileDialog::getOpenFileContent(nameFilter, ready);
}


QString WASMIO::loadedName() const { return _loadedName; }
