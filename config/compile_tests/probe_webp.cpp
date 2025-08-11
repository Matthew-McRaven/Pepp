#include <QByteArrayList>
#include <QImageReader>
#include <QStringList>
#include <iostream>
int main() {
  const QByteArrayList fmts = QImageReader::supportedImageFormats();
  std::cout << "Formats are:";
  for (const auto &ba : fmts) std::cout << " " << ba.constData();
  std::cout.flush();
  return fmts.contains("webp") ? 0 : 1;
}
