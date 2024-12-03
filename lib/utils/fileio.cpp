#include "fileio.hpp"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

FileIO::FileIO(QObject *parent) : QObject(parent) {}

void FileIO::save(const QString &filename, const QString &data) {
  static const auto uri_start = QRegularExpression("file:///");
  auto modified = filename;
  modified.replace(uri_start, "");
  QFile file(modified);
  if (file.open(QIODevice::WriteOnly)) {
    file.write(data.toUtf8());
    file.close();
  } else qWarning() << "Could not open file for writing";
#ifdef __EMSCRIPTEN__
  EM_ASM(FS.syncfs(function(){}););
#endif
}
