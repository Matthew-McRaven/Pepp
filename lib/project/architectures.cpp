#include "architectures.hpp"
pepp::ArchitectureHelper::ArchitectureHelper(QObject *parent) : QObject(parent) {}

QString pepp::ArchitectureHelper::string(Architecture architecture) {
  QMetaEnum metaEnum = QMetaEnum::fromType<Architecture>();
  return QString(metaEnum.valueToKey(static_cast<int>(architecture)));
}

pepp::ArchitectureUtils::ArchitectureUtils(QObject *parent) : QObject(parent) {}

QString pepp::ArchitectureUtils::archAsString(Architecture architecture) {
  return ArchitectureHelper::string(architecture);
}

QString pepp::archAsPrettyString(ArchitectureHelper::Architecture architecture) {
  switch (architecture) {
  case ArchitectureHelper::Architecture::PEP8: return "Pep/8";
  case ArchitectureHelper::Architecture::PEP9: return "Pep/9";
  case ArchitectureHelper::Architecture::PEP10: return "Pep/10";
  case ArchitectureHelper::Architecture::RISCV: return "RISC-V";
  default: qDebug() << "Unknown architecture in archAsPrettyString"; return "Unknown";
  }
}
