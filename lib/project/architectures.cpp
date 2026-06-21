#include "architectures.hpp"

pepp::ArchitectureHelper::ArchitectureHelper(QObject *parent) : QObject(parent) {}

QString pepp::ArchitectureHelper::string(int arch) {
  const auto casted = static_cast<pepp::Architecture>(arch);
  return QString::fromStdString(pepp::arch_as_string(casted));
}
