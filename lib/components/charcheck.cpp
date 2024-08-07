#include "charcheck.hpp"

#include <QFontMetrics>

CharCheck::CharCheck(QObject *parent) : QObject{parent} {}

bool CharCheck::isCharSupported(const QString &character, const QFont &font) {
  if (character.isEmpty())
    return true;
  return QFontMetrics(font).inFont(character.at(0));
}

QFont CharCheck::noMerge(const QFont &font) {
  QFont ret = font;
  ret.setStyleStrategy(QFont::StyleStrategy::NoFontMerging);
  return ret;
}
