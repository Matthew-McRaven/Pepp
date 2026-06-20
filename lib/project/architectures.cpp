#include "architectures.hpp"

pepp::ArchitectureHelper::ArchitectureHelper(QObject *parent) : QObject(parent) {}

QString pepp::ArchitectureHelper::string(pepp::ArchitectureHelper::Architecture arch) {
  const auto casted = to_cpp_type(arch);
  return QString::fromStdString(pepp::arch_as_string(casted));
}

pepp::ArchitectureUtils::ArchitectureUtils(QObject *parent) : QObject(parent) {}

QString pepp::ArchitectureUtils::archAsString(pepp::ArchitectureHelper::Architecture arch) {
  return ArchitectureHelper::string(arch);
}
