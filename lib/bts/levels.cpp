#include "levels.hpp"
pepp::AbstractionHelper::AbstractionHelper(QObject *parent) : QObject(parent) {}

QString pepp::AbstractionHelper::string(Abstraction abstraction) const {
  QMetaEnum metaEnum = QMetaEnum::fromType<Abstraction>();
  return metaEnum.valueToKey(static_cast<int>(abstraction));
}
QString pepp::abstractionAsPrettyString(AbstractionHelper::Abstraction abstraction) {
  switch (abstraction) {
  case AbstractionHelper::Abstraction::MA2: return "MA2";
  case AbstractionHelper::Abstraction::ISA3: return "ISA3";
  case AbstractionHelper::Abstraction::ASMB3: return "Asmb3";
  case AbstractionHelper::Abstraction::OS4: return "OS4";
  case AbstractionHelper::Abstraction::ASMB5: return "Asmb5";
  default: qDebug() << "Unknown abstraction in abstractionAsPrettyString"; return "Unknown";
  }
}
