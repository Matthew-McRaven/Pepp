#include "constants.hpp"

pepp::settings::PaletteRoleHelper::PaletteRoleHelper(QObject *parent) : QObject(parent) {}

QString pepp::settings::PaletteRoleHelper::string(Role role) {
  QMetaEnum metaEnum = QMetaEnum::fromType<Role>();
  return metaEnum.valueToKey(static_cast<int>(role));
}

pepp::settings::CategoryHelper::CategoryHelper(QObject *parent) : QObject(parent) {}

QString pepp::settings::CategoryHelper::string(PaletteCategory cat) const {
  QMetaEnum metaEnum = QMetaEnum::fromType<PaletteCategory>();
  return metaEnum.valueToKey(static_cast<int>(cat));
}

enum Ranges : uint32_t {
  GeneralCategoryStart = (uint32_t)pepp::settings::PaletteRole::BaseRole,
  GeneralCategoryEnd = (uint32_t)pepp::settings::PaletteRole::SymbolRole,
  EditorCategoryStart = GeneralCategoryEnd,
  EditorCategoryEnd = (uint32_t)pepp::settings::PaletteRole::SeqCircuitRole,
  CircuitCategoryStart = EditorCategoryEnd,
  CircuitCategoryEnd = (uint32_t)pepp::settings::PaletteRole::Total,
};
pepp::settings::PaletteCategory pepp::settings::categoryForRole(PaletteRole role) {
  uint32_t asInt = static_cast<uint32_t>(role);
  if (asInt < GeneralCategoryEnd) return PaletteCategory::General;
  else if (asInt < EditorCategoryEnd) return PaletteCategory::Editor;
  else return PaletteCategory::Circuit;
}
