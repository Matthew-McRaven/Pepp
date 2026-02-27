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
  Bus_A,           // xxyy
  Bus_AMux2ALU,    // xxyy
  Bus_B,           // xxyy
  Bus_ALU2CMux,    // xxyy
  Bus_C,           // xxyy
  Bus_NZVC2CMux,   // xxyy
  Bus_Address,     // xxyy
  Bus_Data,        // xxyy
  Bus_MAR2Address, // xxyy
  Clock_Load,      // xxyy
  Clock_MAR,       // xxyy
  Clock_S,         // xxyy
  Clock_C,         // xxyy
  Clock_V,         // xxyy
  Clock_Z,         // xxyy
  Clock_N,         // xxyy
  Sel_MemWrite,    // xxyy
  Sel_MemRead,     // xxyy
  Sel_C,           // xxyy
  Sel_B,           // xxyy
  Sel_A,           // xxyy
  Sel_Mux_A,       // xxyy
  Sel_Mux_C,       // xxyy
  Sel_ALU,         // xxyy
  Sel_Mux_CS,      // xxyy
  Sel_Andz,        // xxyy
  Wire_ALU_NZVC,   // xxy
  Wire_AndZ2Z,     // xxy
  // 1-byte only
  Bus_MDRMux2MDR, // xy
  Bus_MDR2AMux,   // xy
  Bus_MDR2Data,   // xy
  Clock_MDR,      // xy
  Sel_Mux_MDR,    // xy
  // 2-byte only
  Bus_MARMux2MARA,  // xy
  Bus_MARMux2MARB,  // xy
  Bus_MDREMux2MDRE, // xy
  Bus_MDRE2Data,    // xy
  Bus_MDRE2EOMux,   // xy
  Bus_MDROMux2MDRO, // xy
  Bus_MDRO2Data,    // xy
  Bus_MDRO2EOMux,   // xy
  Bus_EOMux2AMux,   // xy
  Clock_MDRE,       // xy
  Clock_MDRO,       // xy
  Sel_Mux_MAR,      // xy
  Sel_Mux_MDRO,     // xy
  Sel_Mux_MDRE,     // xy
  Sel_Mux_EO,       // xy
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
