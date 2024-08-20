#include "platformdetector.hpp"
#include <QOperatingSystemVersion>

PlatformDetector::PlatformDetector(QObject *parent) : QObject{parent} {}

bool PlatformDetector::isWindows() const {
  return QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Windows;
}

bool PlatformDetector::isMac() const {
  return QOperatingSystemVersion::currentType() == QOperatingSystemVersion::MacOS;
}

bool PlatformDetector::isLinux() const {
#ifdef Q_OS_LINUX
  return true;
#else
  return false;
#endif
}

bool PlatformDetector::isWASM() const {
#ifdef Q_OS_WASM
  return true;
#else
  return false;
#endif
}
