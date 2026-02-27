#pragma once
#include <QtCore/qobject.h>
#include <array>
#include <qqmlintegration.h>
#include <stdint.h>
#include "core/arch/pep/uarch/pep.hpp"

namespace pepp {
enum class Connections {
  None = 0,
  // Shared between 1- and 2-byte data busses
  Bus_A,
  Bus_AMux2ALU,
  Bus_B,
  Bus_ALU2CMux,
  Bus_C,
  Bus_NZVC2CMux,
  Bus_Address,
  Bus_Data,
  Bus_MAR2Address,
  Clock_Load,
  Clock_MAR,
  Clock_S,
  Clock_C,
  Clock_V,
  Clock_Z,
  Clock_N,
  Sel_MemWrite,
  Sel_MemRead,
  Sel_C,
  Sel_B,
  Sel_A,
  Sel_Mux_A,
  Sel_Mux_C,
  Sel_ALU,
  Sel_Mux_CS,
  Sel_Andz,
  Wire_ALU_NZVC,
  Wire_AndZ2Z,
  // 1-byte only
  Bus_MDRMux2MDR,
  Bus_MDR2AMux,
  Bus_MDR2Data,
  Clock_MDR,
  Sel_Mux_MDR,
  // 2-byte only
  Bus_MARMux2MARA,
  Bus_MARMux2MARB,
  Bus_MDREMux2MDRE,
  Bus_MDRE2Data,
  Bus_MDRE2EOMux,
  Bus_MDROMux2MDRO,
  Bus_MDRO2Data,
  Bus_MDRO2EOMux,
  Bus_EOMux2AMux,
  Clock_MDRE,
  Clock_MDRO,
  Sel_Mux_MAR,
  Sel_Mux_MDRO,
  Sel_Mux_MDRE,
  Sel_Mux_EO,
  Total
};
using ConnectionArray = std::array<int, static_cast<size_t>(Connections::Total)>;
using one_bye_mc = tc::arch::Pep9ByteBus::CodeWithEnables;
using two_bye_mc = tc::arch::Pep9WordBus::CodeWithEnables;
enum class MemoryState {
  Inactive = 0,
  Active = 1,
  Writing = 2,
};

void connections_for(ConnectionArray &arr, const one_bye_mc &mc, MemoryState mem);
void connections_for(ConnectionArray &arr, const two_bye_mc &mc, MemoryState mem);

class ConnectionsHolder : public QObject {
  Q_OBJECT
  QML_NAMED_ELEMENT(ConnectionsHolder)
public:
  ConnectionsHolder(QObject *parent = nullptr);
  ConnectionArray c;
};

} // namespace pepp
