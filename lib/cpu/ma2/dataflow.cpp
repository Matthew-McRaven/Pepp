#include "dataflow.hpp"
#include "settings/palette.hpp"

void pepp::connections_for(ConnectionArray &arr, const one_bye_mc &mc, MemoryState mem) {
  using enum Connections;
  using S = tc::arch::Pep9ByteBus::Signals;
  static const auto enabled = (int)pepp::settings::PaletteRole::BaseRole;
  static const auto disabled = (int)pepp::settings::PaletteRole::MidRole;
  static const auto primary = (int)pepp::settings::PaletteRole::CircuitPrimaryRole;
  static const auto secondary = (int)pepp::settings::PaletteRole::CircuitSecondaryRole;
  static const auto tertiary = (int)pepp::settings::PaletteRole::CircuitTertiaryRole;
  static const auto quaternary = (int)pepp::settings::PaletteRole::CircuitQuaternaryRole;
  arr.fill((int)enabled);
  arr[(int)Bus_A] = mc.enabled(S::A) ? primary : enabled;
  arr[(int)Bus_B] = mc.enabled(S::B) ? primary : enabled;
  arr[(int)Bus_NZVC2CMux] = tertiary;
  arr[(int)Clock_Load] = mc.enabled(S::LoadCk) ? enabled : disabled;
  arr[(int)Clock_MAR] = mc.enabled(S::MARCk) ? enabled : disabled;
  arr[(int)Clock_S] = mc.enabled(S::SCk) ? enabled : disabled;
  arr[(int)Clock_C] = mc.enabled(S::CCk) ? enabled : disabled;
  arr[(int)Clock_V] = mc.enabled(S::VCk) ? enabled : disabled;
  arr[(int)Clock_Z] = mc.enabled(S::ZCk) ? enabled : disabled;
  arr[(int)Clock_N] = mc.enabled(S::NCk) ? enabled : disabled;
  arr[(int)Clock_MDR] = mc.enabled(S::MDRCk) ? enabled : disabled;
  arr[(int)Sel_MemWrite] = mc.enabled(S::MemWrite) ? enabled : disabled;
  arr[(int)Sel_MemRead] = mc.enabled(S::MemRead) ? enabled : disabled;
  arr[(int)Sel_C] = mc.enabled(S::C) ? enabled : disabled;
  arr[(int)Sel_B] = mc.enabled(S::B) ? enabled : disabled;
  arr[(int)Sel_A] = mc.enabled(S::A) ? enabled : disabled;
  arr[(int)Sel_Mux_A] = mc.enabled(S::AMux) ? enabled : disabled;
  arr[(int)Sel_ALU] = mc.enabled(S::ALU) ? enabled : disabled;
  arr[(int)Sel_Mux_CS] = mc.enabled(S::CSMux) ? enabled : disabled;
  arr[(int)Sel_Andz] = mc.enabled(S::AndZ) ? enabled : disabled;
  arr[(int)Wire_AndZ2Z] = mc.enabled(S::AndZ) ? enabled : disabled;
  arr[(int)Sel_Mux_C] = mc.enabled(S::CMux) ? enabled : disabled;
  arr[(int)Sel_Mux_MDR] = mc.enabled(S::MDRMux) ? enabled : disabled;
  arr[(int)Bus_MDR2AMux] = mc.enabled(S::AMux) && mc.get(S::AMux) == 0 ? quaternary : enabled;
  arr[(int)Bus_MDR2Data] = tertiary;
  arr[(int)Bus_MAR2Address] = quaternary;
  switch (mem) {
  case MemoryState::Inactive:
    arr[(int)Bus_Address] = enabled;
    arr[(int)Bus_Data] = enabled;
    break;
  case MemoryState::Writing: [[fallthrough]];
  case MemoryState::Active: arr[(int)Bus_Address] = tertiary; arr[(int)Bus_Data] = primary;
  }
  if (mc.enabled(S::ALU)) arr[(int)Bus_ALU2CMux] = secondary, arr[(int)Wire_ALU_NZVC] = enabled;
  else arr[(int)Bus_ALU2CMux] = enabled, arr[(int)Wire_ALU_NZVC] = disabled;
  // Depends on Bus_ALU2CMux, Bus_NZVC2CMux
  if (mc.enabled(S::CMux)) {
    arr[(int)Bus_C] = mc.get(S::CMux) == 1 ? arr[(int)Bus_ALU2CMux] : arr[(int)Bus_NZVC2CMux];
  } else arr[(int)Bus_C] = enabled;
  // Depends on Bus_Data, Bus_C
  if (mc.enabled(S::MDRMux)) {
    if (mc.get(S::MDRMux) == 1) arr[(int)Bus_MDRMux2MDR] = arr[(int)Bus_C];
    else if (mem == MemoryState::Writing) arr[(int)Bus_MDRMux2MDR] = arr[(int)Bus_Data];
    else arr[(int)Bus_MDRMux2MDR] = enabled;
  } else arr[(int)Bus_MDRMux2MDR] = enabled;
  // Depends on Bus_A, Bus_MDR2AMux
  if (mc.enabled(S::AMux)) {
    arr[(int)Bus_AMux2ALU] = mc.get(S::AMux) == 0 ? arr[(int)Bus_MDR2AMux] : arr[(int)Bus_A];
  } else arr[(int)Bus_AMux2ALU] = enabled;
}

void pepp::connections_for(ConnectionArray &arr, const two_bye_mc &mc, MemoryState mem) {
  using enum Connections;
  using S = tc::arch::Pep9WordBus::Signals;
  static const auto enabled = (int)pepp::settings::PaletteRole::BaseRole;
  static const auto disabled = (int)pepp::settings::PaletteRole::MidRole;
  static const auto primary = (int)pepp::settings::PaletteRole::CircuitPrimaryRole;
  static const auto secondary = (int)pepp::settings::PaletteRole::CircuitSecondaryRole;
  static const auto tertiary = (int)pepp::settings::PaletteRole::CircuitTertiaryRole;
  static const auto quaternary = (int)pepp::settings::PaletteRole::CircuitQuaternaryRole;
  arr.fill((int)enabled);
  arr[(int)Bus_A] = mc.enabled(S::A) ? primary : enabled;
  arr[(int)Bus_B] = mc.enabled(S::B) ? primary : enabled;
  arr[(int)Bus_NZVC2CMux] = tertiary;
  arr[(int)Clock_Load] = mc.enabled(S::LoadCk) ? enabled : disabled;
  arr[(int)Clock_MAR] = mc.enabled(S::MARCk) ? enabled : disabled;
  arr[(int)Clock_S] = mc.enabled(S::SCk) ? enabled : disabled;
  arr[(int)Clock_C] = mc.enabled(S::CCk) ? enabled : disabled;
  arr[(int)Clock_V] = mc.enabled(S::VCk) ? enabled : disabled;
  arr[(int)Clock_Z] = mc.enabled(S::ZCk) ? enabled : disabled;
  arr[(int)Clock_N] = mc.enabled(S::NCk) ? enabled : disabled;
  arr[(int)Clock_MDRE] = mc.enabled(S::MDRECk) ? enabled : disabled;
  arr[(int)Clock_MDRO] = mc.enabled(S::MDROCk) ? enabled : disabled;
  arr[(int)Sel_MemWrite] = mc.enabled(S::MemWrite) ? enabled : disabled;
  arr[(int)Sel_MemRead] = mc.enabled(S::MemRead) ? enabled : disabled;
  arr[(int)Sel_C] = mc.enabled(S::C) ? enabled : disabled;
  arr[(int)Sel_B] = mc.enabled(S::B) ? enabled : disabled;
  arr[(int)Sel_A] = mc.enabled(S::A) ? enabled : disabled;
  arr[(int)Sel_Mux_A] = mc.enabled(S::AMux) ? enabled : disabled;
  arr[(int)Sel_ALU] = mc.enabled(S::ALU) ? enabled : disabled;
  arr[(int)Sel_Mux_CS] = mc.enabled(S::CSMux) ? enabled : disabled;
  arr[(int)Sel_Andz] = mc.enabled(S::AndZ) ? enabled : disabled;
  arr[(int)Wire_AndZ2Z] = mc.enabled(S::AndZ) ? enabled : disabled;
  arr[(int)Sel_Mux_C] = mc.enabled(S::CMux) ? enabled : disabled;
  arr[(int)Sel_Mux_MDRE] = mc.enabled(S::MDREMux) ? enabled : disabled;
  arr[(int)Sel_Mux_MDRO] = mc.enabled(S::MDROMux) ? enabled : disabled;
  arr[(int)Sel_Mux_EO] = mc.enabled(S::EOMux) ? enabled : disabled;
  arr[(int)Sel_Mux_MAR] = mc.enabled(S::MARMux) ? enabled : disabled;
  arr[(int)Bus_MDRE2Data] = tertiary;
  arr[(int)Bus_MDRO2Data] = tertiary;
  arr[(int)Bus_MAR2Address] = quaternary;
  switch (mem) {
  case MemoryState::Inactive:
    arr[(int)Bus_Address] = enabled;
    arr[(int)Bus_Data] = enabled;
    break;
  case MemoryState::Writing: [[fallthrough]];
  case MemoryState::Active: arr[(int)Bus_Address] = tertiary; arr[(int)Bus_Data] = primary;
  }
  if (mc.enabled(S::ALU)) arr[(int)Bus_ALU2CMux] = secondary, arr[(int)Wire_ALU_NZVC] = enabled;
  else arr[(int)Bus_ALU2CMux] = enabled, arr[(int)Wire_ALU_NZVC] = disabled;

  if ((mc.enabled(S::MARMux) && mc.get(S::MARMux) == 0 && mc.get(S::MARCk)) ||
      (mc.enabled(S::EOMux) && mc.get(S::EOMux) == 1)) {
    arr[(int)Bus_MDRO2EOMux] = secondary;
  } else arr[(int)Bus_MDRO2EOMux] = enabled;
  if ((mc.enabled(S::MARMux) && mc.get(S::MARMux) == 0 && mc.get(S::MARCk)) ||
      (mc.enabled(S::EOMux) && mc.get(S::EOMux) == 0)) {
    arr[(int)Bus_MDRE2EOMux] = tertiary;
  } else arr[(int)Bus_MDRE2EOMux] = enabled;

  // Depends on Bus_MDRO2EOMux, Bus_MDRE2EOMux, Bus_A, Bus_B
  if (mc.enabled(S::MARMux)) {
    if (mc.get(S::MARMux) == 0) {
      arr[(int)Bus_MARMux2MARA] = arr[(int)Bus_MDRO2EOMux];
      arr[(int)Bus_MARMux2MARB] = arr[(int)Bus_MDRE2EOMux];
    } else {
      arr[(int)Bus_MARMux2MARA] = arr[(int)Bus_A];
      arr[(int)Bus_MARMux2MARB] = arr[(int)Bus_B];
    }
  }
  // Depends on Bus_MDRO2EOMux, Bus_MDRE2EOMux
  if (mc.enabled(S::EOMux)) {
    arr[(int)Bus_EOMux2AMux] = mc.get(S::EOMux) == 1 ? arr[(int)Bus_MDRO2EOMux] : arr[(int)Bus_MDRE2EOMux];
  } else arr[(int)Bus_EOMux2AMux] = enabled;
  // Depends on Bus_ALU2CMux, Bus_NZVC2CMux
  if (mc.enabled(S::CMux)) {
    arr[(int)Bus_C] = mc.get(S::CMux) == 1 ? arr[(int)Bus_ALU2CMux] : arr[(int)Bus_NZVC2CMux];
  } else arr[(int)Bus_C] = enabled;
  // Depends on Bus_Data, Bus_C
  if (mc.enabled(S::MDREMux)) {
    if (mc.get(S::MDREMux) == 1) arr[(int)Bus_MDREMux2MDRE] = arr[(int)Bus_C];
    else if (mem == MemoryState::Writing) arr[(int)Bus_MDREMux2MDRE] = arr[(int)Bus_Data];
    else arr[(int)Bus_MDREMux2MDRE] = enabled;
  } else arr[(int)Bus_MDREMux2MDRE] = enabled;
  if (mc.enabled(S::MDROMux)) {
    if (mc.get(S::MDROMux) == 1) arr[(int)Bus_MDROMux2MDRO] = arr[(int)Bus_C];
    else if (mem == MemoryState::Writing) arr[(int)Bus_MDROMux2MDRO] = arr[(int)Bus_Data];
    else arr[(int)Bus_MDROMux2MDRO] = enabled;
  } else arr[(int)Bus_MDROMux2MDRO] = enabled;
  // Depends on Bus_A, Bus_EOMux2AMux
  if (mc.enabled(S::AMux)) {
    if (mc.get(S::AMux) == 1) arr[(int)Bus_AMux2ALU] = arr[(int)Bus_A];
    else if (mc.enabled(S::EOMux)) arr[(int)Bus_AMux2ALU] = arr[(int)Bus_EOMux2AMux];
    else arr[(int)Bus_AMux2ALU] = enabled;
  } else arr[(int)Bus_AMux2ALU] = enabled;
}

pepp::ConnectionsHolder::ConnectionsHolder(QObject *parent) : QObject(parent) {}
