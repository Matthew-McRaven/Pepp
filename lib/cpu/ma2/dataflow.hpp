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
  Bus_A,           // xxy
  Bus_AMux2ALU,    // xxy
  Bus_B,           // xxy
  Bus_ALU2CMux,    // xxy
  Bus_C,           // xxy
  Bus_NZVC2CMux,   // xxy
  Bus_Address,     // xxy
  Bus_Data,        // xxy
  Bus_MAR2Address, // xxy
  Clock_Load,      // xxy
  Clock_MAR,       // xxy
  Clock_S,         // xxy
  Clock_C,         // xxy
  Clock_V,         // xxy
  Clock_Z,         // xxy
  Clock_N,         // xxy
  Sel_MemWrite,    // xxy
  Sel_MemRead,     // xxy
  Sel_C,           // xxy
  Sel_B,           // xxy
  Sel_A,           // xxy
  Sel_Mux_A,       // xxy
  Sel_Mux_C,       // xxy
  Sel_ALU,         // xxy
  Sel_Mux_CS,      // xxy
  Sel_Andz,        // xxy
  Wire_ALU_NZVC,   // xxy
  Wire_AndZ2Z,     // xxy
  // 1-byte only
  Bus_MDRMux2MDR, // xy
  Bus_MDR2AMux,   // xy
  Bus_MDR2Data,   // xy
  Clock_MDR,      // xy
  Sel_Mux_MDR,    // xy
  // 2-byte only
  Bus_MARMux2MARA,  // x
  Bus_MARMux2MARB,  // x
  Bus_MDREMux2MDRE, // x
  Bus_MDRE2Data,    // x
  Bus_MDRE2EOMux,   // x
  Bus_MDROMux2MDRO, // x
  Bus_MDRO2Data,    // x
  Bus_MDRO2EOMux,   // x
  Bus_EOMux2AMux,   // x
  Clock_MDRE,       // x
  Clock_MDRO,       // x
  Sel_Mux_MAR,      // x
  Sel_Mux_MDRO,     // x
  Sel_Mux_MDRE,     // x
  Sel_Mux_EO,       // x
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
