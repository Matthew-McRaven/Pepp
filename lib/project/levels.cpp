#include "levels.hpp"
pepp::AbstractionHelper::AbstractionHelper(QObject *parent) : QObject(parent) {}

QString pepp::AbstractionHelper::string(Abstractionss abstraction) const {
  return QString::fromStdString(pepp::level_as_string(static_cast<pepp::AbstractionEnu>(abstraction)));
}
