/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include "core/libs/bitmanip/integers.h"
namespace pepp::bts {

// Will eventually exist in C++23, but it can be implemented easily enough by hand until it is widely supported.
template <class Enum> constexpr std::underlying_type_t<Enum> to_underlying(Enum e) noexcept {
  return static_cast<std::underlying_type_t<Enum>>(e);
}

template <typename T>
  requires(std::is_enum_v<T> && requires(T e) { is_bitflags(e); })
constexpr auto operator|(const T lhs, const T rhs) {
  return static_cast<T>(to_underlying(lhs) | to_underlying(rhs));
}

template <typename T>
  requires(std::is_enum_v<T> && requires(T e) { is_bitflags(e); })
constexpr auto operator|=(T &lhs, const T &rhs) {
  lhs = lhs | rhs;
  return lhs;
}

template <typename T>
  requires(std::is_enum_v<T> && requires(T e) { is_bitflags(e); })
constexpr auto operator&(const T lhs, const T rhs) {
  return static_cast<T>(to_underlying(lhs) & to_underlying(rhs));
}

template <typename T>
  requires(std::is_enum_v<T> && requires(T e) { is_bitflags(e); })
constexpr auto operator&=(T &lhs, const T &rhs) {
  lhs = lhs & rhs;
  return lhs;
}

template <typename T>
  requires(std::is_enum_v<T> && requires(T e) { is_bitflags(e); })
constexpr auto operator^(const T lhs, const T rhs) {
  return static_cast<T>(to_underlying(lhs) ^ to_underlying(rhs));
}

template <typename T>
  requires(std::is_enum_v<T> && requires(T e) { is_bitflags(e); })
constexpr auto operator^=(T &lhs, const T &rhs) {
  lhs = lhs ^ rhs;
  return lhs;
}

template <typename T>
  requires(std::is_enum_v<T> && requires(T e) { is_bitflags(e); })
constexpr auto operator~(const T lhs) {
  return static_cast<T>(~to_underlying(lhs));
}

// Can be hijacked to provide a bool conversion.
// !!e will yield a bool; we can't have a free operator bool() on enums
template <typename T>
  requires(std::is_enum_v<T> && requires(T e) { is_bitflags(e); })
constexpr auto operator!(const T lhs) {
  return static_cast<T>(!to_underlying(lhs));
}

// Return true if any bits are set.
// We can't have a free operator bool() on enums, so this is the next closest thing
template <class T>
  requires(std::is_enum_v<T> && requires(T e) { is_bitflags(e); })
constexpr bool any(T lhs) noexcept {
  return to_underlying(lhs) != 0;
}

template <class T>
  requires(std::is_enum_v<T> && requires(T e) { is_bitflags(e); })
constexpr bool none(T lhs) noexcept {
  return to_underlying(lhs) == 0;
}

// ELF file header enumerated constants
enum class ElfFileType : u16 {
  ET_NONE = 0,
  ET_REL = 1,
  ET_EXEC = 2,
  ET_DYN = 3,
  ET_CORE = 4,
  ET_LOOS = 0xFE00,
  ET_HIOS = 0xFEFF,
  ET_LOPROC = 0xFF00,
  ET_HIPROC = 0xFFFF
};

enum class ElfMachineType {
  EM_NONE = 0,                   // No machine
  EM_M32 = 1,                    // AT&T WE 32100
  EM_SPARC = 2,                  // SUN SPARC
  EM_386 = 3,                    // Intel 80386
  EM_68K = 4,                    // Motorola m68k family
  EM_88K = 5,                    // Motorola m88k family
  EM_486 = 6,                    // Intel 80486// Reserved for future use
  EM_860 = 7,                    // Intel 80860
  EM_MIPS = 8,                   // MIPS R3000 (officially, big-endian only)
  EM_S370 = 9,                   // IBM System/370
  EM_MIPS_RS3_LE = 10,           // MIPS R3000 little-endian (Deprecated)
  EM_res011 = 11,                // Reserved
  EM_res012 = 12,                // Reserved
  EM_res013 = 13,                // Reserved
  EM_res014 = 14,                // Reserved
  EM_PARISC = 15,                // HPPA
  EM_res016 = 16,                // Reserved
  EM_VPP550 = 17,                // Fujitsu VPP500
  EM_SPARC32PLUS = 18,           // Sun's "v8plus"
  EM_960 = 19,                   // Intel 80960
  EM_PPC = 20,                   // PowerPC
  EM_PPC64 = 21,                 // 64-bit PowerPC
  EM_S390 = 22,                  // IBM S/390
  EM_SPU = 23,                   // Sony/Toshiba/IBM SPU
  EM_res024 = 24,                // Reserved
  EM_res025 = 25,                // Reserved
  EM_res026 = 26,                // Reserved
  EM_res027 = 27,                // Reserved
  EM_res028 = 28,                // Reserved
  EM_res029 = 29,                // Reserved
  EM_res030 = 30,                // Reserved
  EM_res031 = 31,                // Reserved
  EM_res032 = 32,                // Reserved
  EM_res033 = 33,                // Reserved
  EM_res034 = 34,                // Reserved
  EM_res035 = 35,                // Reserved
  EM_V800 = 36,                  // NEC V800 series
  EM_FR20 = 37,                  // Fujitsu FR20
  EM_RH32 = 38,                  // TRW RH32
  EM_MCORE = 39,                 // Motorola M*Core // May also be taken by Fujitsu MMA
  EM_RCE = 39,                   // Old name for MCore
  EM_ARM = 40,                   // ARM
  EM_OLD_ALPHA = 41,             // Digital Alpha
  EM_SH = 42,                    // Renesas (formerly Hitachi) / SuperH SH
  EM_SPARCV9 = 43,               // SPARC v9 64-bit
  EM_TRICORE = 44,               // Siemens Tricore embedded processor
  EM_ARC = 45,                   // ARC Cores
  EM_H8_300 = 46,                // Renesas (formerly Hitachi) H8/300
  EM_H8_300H = 47,               // Renesas (formerly Hitachi) H8/300H
  EM_H8S = 48,                   // Renesas (formerly Hitachi) H8S
  EM_H8_500 = 49,                // Renesas (formerly Hitachi) H8/500
  EM_IA_64 = 50,                 // Intel IA-64 Processor
  EM_MIPS_X = 51,                // Stanford MIPS-X
  EM_COLDFIRE = 52,              // Motorola Coldfire
  EM_68HC12 = 53,                // Motorola M68HC12
  EM_MMA = 54,                   // Fujitsu Multimedia Accelerator
  EM_PCP = 55,                   // Siemens PCP
  EM_NCPU = 56,                  // Sony nCPU embedded RISC processor
  EM_NDR1 = 57,                  // Denso NDR1 microprocesspr
  EM_STARCORE = 58,              // Motorola Star*Core processor
  EM_ME16 = 59,                  // Toyota ME16 processor
  EM_ST100 = 60,                 // STMicroelectronics ST100 processor
  EM_TINYJ = 61,                 // Advanced Logic Corp. TinyJ embedded processor
  EM_X86_64 = 62,                // Advanced Micro Devices X86-64 processor
  EM_PDSP = 63,                  // Sony DSP Processor
  EM_PDP10 = 64,                 // Digital Equipment Corp. PDP-10
  EM_PDP11 = 65,                 // Digital Equipment Corp. PDP-11
  EM_FX66 = 66,                  // Siemens FX66 microcontroller
  EM_ST9PLUS = 67,               // STMicroelectronics ST9+ 8/16 bit microcontroller
  EM_ST7 = 68,                   // STMicroelectronics ST7 8-bit microcontroller
  EM_68HC16 = 69,                // Motorola MC68HC16 Microcontroller
  EM_68HC11 = 70,                // Motorola MC68HC11 Microcontroller
  EM_68HC08 = 71,                // Motorola MC68HC08 Microcontroller
  EM_68HC05 = 72,                // Motorola MC68HC05 Microcontroller
  EM_SVX = 73,                   // Silicon Graphics SVx
  EM_ST19 = 74,                  // STMicroelectronics ST19 8-bit cpu
  EM_VAX = 75,                   // Digital VAX
  EM_CRIS = 76,                  // Axis Communications 32-bit embedded processor
  EM_JAVELIN = 77,               // Infineon Technologies 32-bit embedded cpu
  EM_FIREPATH = 78,              // Element 14 64-bit DSP processor
  EM_ZSP = 79,                   // LSI Logic's 16-bit DSP processor
  EM_MMIX = 80,                  // Donald Knuth's educational 64-bit processor
  EM_HUANY = 81,                 // Harvard's machine-independent format
  EM_PRISM = 82,                 // SiTera Prism
  EM_AVR = 83,                   // Atmel AVR 8-bit microcontroller
  EM_FR30 = 84,                  // Fujitsu FR30
  EM_D10V = 85,                  // Mitsubishi D10V
  EM_D30V = 86,                  // Mitsubishi D30V
  EM_V850 = 87,                  // NEC v850
  EM_M32R = 88,                  // Renesas M32R (formerly Mitsubishi M32R)
  EM_MN10300 = 89,               // Matsushita MN10300
  EM_MN10200 = 90,               // Matsushita MN10200
  EM_PJ = 91,                    // picoJava
  EM_OPENRISC = 92,              // OpenRISC 32-bit embedded processor
  EM_ARC_A5 = 93,                // ARC Cores Tangent-A5
  EM_XTENSA = 94,                // Tensilica Xtensa Architecture
  EM_VIDEOCORE = 95,             // Alphamosaic VideoCore processor
  EM_TMM_GPP = 96,               // Thompson Multimedia General Purpose Processor
  EM_NS32K = 97,                 // National Semiconductor 32000 series
  EM_TPC = 98,                   // Tenor Network TPC processor
  EM_SNP1K = 99,                 // Trebia SNP 1000 processor
  EM_ST200 = 100,                // STMicroelectronics ST200 microcontroller
  EM_IP2K = 101,                 // Ubicom IP2022 micro controller
  EM_MAX = 102,                  // MAX Processor
  EM_CR = 103,                   // National Semiconductor CompactRISC
  EM_F2MC16 = 104,               // Fujitsu F2MC16
  EM_MSP430 = 105,               // TI msp430 micro controller
  EM_BLACKFIN = 106,             // ADI Blackfin
  EM_SE_C33 = 107,               // S1C33 Family of Seiko Epson processors
  EM_SEP = 108,                  // Sharp embedded microprocessor
  EM_ARCA = 109,                 // Arca RISC Microprocessor
  EM_UNICORE = 110,              // Microprocessor series from PKU-Unity Ltd.
  EM_EXCESS = 111,               // eXcess: 16/32/64-bit configurable embedded CPU
  EM_DXP = 112,                  // Icera Semiconductor Inc. Deep Execution Processor
  EM_ALTERA_NIOS2 = 113,         // Altera Nios II soft-core processor
  EM_CRX = 114,                  // National Semiconductor CRX
  EM_XGATE = 115,                // Motorola XGATE embedded processor
  EM_C166 = 116,                 // Infineon C16x/XC16x processor
  EM_M16C = 117,                 // Renesas M16C series microprocessors
  EM_DSPIC30F = 118,             // Microchip Technology dsPIC30F DSignal Controller
  EM_CE = 119,                   // Freescale Communication Engine RISC core
  EM_M32C = 120,                 // Renesas M32C series microprocessors
  EM_res121 = 121,               // Reserved
  EM_res122 = 122,               // Reserved
  EM_res123 = 123,               // Reserved
  EM_res124 = 124,               // Reserved
  EM_res125 = 125,               // Reserved
  EM_res126 = 126,               // Reserved
  EM_res127 = 127,               // Reserved
  EM_res128 = 128,               // Reserved
  EM_res129 = 129,               // Reserved
  EM_res130 = 130,               // Reserved
  EM_TSK3000 = 131,              // Altium TSK3000 core
  EM_RS08 = 132,                 // Freescale RS08 embedded processor
  EM_res133 = 133,               // Reserved
  EM_ECOG2 = 134,                // Cyan Technology eCOG2 microprocessor
  EM_SCORE = 135,                // Sunplus Score
  EM_SCORE7 = 135,               // Sunplus S+core7 RISC processor
  EM_DSP24 = 136,                // New Japan Radio (NJR) 24-bit DSP Processor
  EM_VIDEOCORE3 = 137,           // Broadcom VideoCore III processor
  EM_LATTICEMICO32 = 138,        // RISC processor for Lattice FPGA architecture
  EM_SE_C17 = 139,               // Seiko Epson C17 family
  EM_TI_C6000 = 140,             // Texas Instruments TMS320C6000 DSP family
  EM_TI_C2000 = 141,             // Texas Instruments TMS320C2000 DSP family
  EM_TI_C5500 = 142,             // Texas Instruments TMS320C55x DSP family
  EM_res143 = 143,               // Reserved
  EM_res144 = 144,               // Reserved
  EM_res145 = 145,               // Reserved
  EM_res146 = 146,               // Reserved
  EM_res147 = 147,               // Reserved
  EM_res148 = 148,               // Reserved
  EM_res149 = 149,               // Reserved
  EM_res150 = 150,               // Reserved
  EM_res151 = 151,               // Reserved
  EM_res152 = 152,               // Reserved
  EM_res153 = 153,               // Reserved
  EM_res154 = 154,               // Reserved
  EM_res155 = 155,               // Reserved
  EM_res156 = 156,               // Reserved
  EM_res157 = 157,               // Reserved
  EM_res158 = 158,               // Reserved
  EM_res159 = 159,               // Reserved
  EM_MMDSP_PLUS = 160,           // STMicroelectronics 64bit VLIW Data Signal Processor
  EM_CYPRESS_M8C = 161,          // Cypress M8C microprocessor
  EM_R32C = 162,                 // Renesas R32C series microprocessors
  EM_TRIMEDIA = 163,             // NXP Semiconductors TriMedia architecture family
  EM_QDSP6 = 164,                // QUALCOMM DSP6 Processor
  EM_8051 = 165,                 // Intel 8051 and variants
  EM_STXP7X = 166,               // STMicroelectronics STxP7x family
  EM_NDS32 = 167,                // Andes Technology embedded RISC processor family
  EM_ECOG1 = 168,                // Cyan Technology eCOG1X family
  EM_ECOG1X = 168,               // Cyan Technology eCOG1X family
  EM_MAXQ30 = 169,               // Dallas Semiconductor MAXQ30 Core Micro-controllers
  EM_XIMO16 = 170,               // New Japan Radio (NJR) 16-bit DSP Processor
  EM_MANIK = 171,                // M2000 Reconfigurable RISC Microprocessor
  EM_CRAYNV2 = 172,              // Cray Inc. NV2 vector architecture
  EM_RX = 173,                   // Renesas RX family
  EM_METAG = 174,                // Imagination Technologies META processor architecture
  EM_MCST_ELBRUS = 175,          // MCST Elbrus general purpose hardware architecture
  EM_ECOG16 = 176,               // Cyan Technology eCOG16 family
  EM_CR16 = 177,                 // National Semiconductor CompactRISC 16-bit processor
  EM_ETPU = 178,                 // Freescale Extended Time Processing Unit
  EM_SLE9X = 179,                // Infineon Technologies SLE9X core
  EM_L1OM = 180,                 // Intel L1OM
  EM_INTEL181 = 181,             // Reserved by Intel
  EM_INTEL182 = 182,             // Reserved by Intel
  EM_AARCH64 = 183,              // ARM AArch64
  EM_res184 = 184,               // Reserved by ARM
  EM_AVR32 = 185,                // Atmel Corporation 32-bit microprocessor family
  EM_STM8 = 186,                 // STMicroeletronics STM8 8-bit microcontroller
  EM_TILE64 = 187,               // Tilera TILE64 multicore architecture family
  EM_TILEPRO = 188,              // Tilera TILEPro multicore architecture family
  EM_MICROBLAZE = 189,           // Xilinx MicroBlaze 32-bit RISC soft processor core
  EM_CUDA = 190,                 // NVIDIA CUDA architecture
  EM_TILEGX = 191,               // Tilera TILE-Gx multicore architecture family
  EM_CLOUDSHIELD = 192,          // CloudShield architecture family
  EM_COREA_1ST = 193,            // KIPO-KAIST Core-A 1st generation processor family
  EM_COREA_2ND = 194,            // KIPO-KAIST Core-A 2nd generation processor family
  EM_ARC_COMPACT2 = 195,         // Synopsys ARCompact V2
  EM_OPEN8 = 196,                // Open8 8-bit RISC soft processor core
  EM_RL78 = 197,                 // Renesas RL78 family
  EM_VIDEOCORE5 = 198,           // Broadcom VideoCore V processor
  EM_78KOR = 199,                // Renesas 78KOR family
  EM_56800EX = 200,              // Freescale 56800EX Digital Signal Controller (DSC)
  EM_BA1 = 201,                  // Beyond BA1 CPU architecture
  EM_BA2 = 202,                  // Beyond BA2 CPU architecture
  EM_XCORE = 203,                // XMOS xCORE processor family
  EM_MCHP_PIC = 204,             // Microchip 8-bit PIC(r) family
  EM_INTEL205 = 205,             // Reserved by Intel
  EM_INTEL206 = 206,             // Reserved by Intel
  EM_INTEL207 = 207,             // Reserved by Intel
  EM_INTEL208 = 208,             // Reserved by Intel
  EM_INTEL209 = 209,             // Reserved by Intel
  EM_KM32 = 210,                 // KM211 KM32 32-bit processor
  EM_KMX32 = 211,                // KM211 KMX32 32-bit processor
  EM_KMX16 = 212,                // KM211 KMX16 16-bit processor
  EM_KMX8 = 213,                 // KM211 KMX8 8-bit processor
  EM_KVARC = 214,                // KM211 KVARC processor
  EM_CDP = 215,                  // Paneve CDP architecture family
  EM_COGE = 216,                 // Cognitive Smart Memory Processor
  EM_COOL = 217,                 // iCelero CoolEngine
  EM_NORC = 218,                 // Nanoradio Optimized RISC
  EM_CSR_KALIMBA = 219,          // CSR Kalimba architecture family
  EM_Z80 = 220,                  // Zilog Z80
  EM_VISIUM = 221,               // Controls and Data Services VISIUMcore processor
  EM_FT32 = 222,                 // FTDI Chip FT32 high performance 32-bit RISC architecture
  EM_MOXIE = 223,                // Moxie processor family
  EM_AMDGPU = 224,               // AMD GPU architecture
  EM_RISCV = 243,                // RISC-V
  EM_LANAI = 244,                // Lanai processor
  EM_CEVA = 245,                 // CEVA Processor Architecture Family
  EM_CEVA_X2 = 246,              // CEVA X2 Processor Family
  EM_BPF = 247,                  // Linux BPF – in-kernel virtual machine
  EM_GRAPHCORE_IPU = 248,        // Graphcore Intelligent Processing Unit
  EM_IMG1 = 249,                 // Imagination Technologies
  EM_NFP = 250,                  // Netronome Flow Processor (P)
  EM_CSKY = 252,                 // C-SKY processor family
  EM_ARC_COMPACT3_64 = 253,      // Synopsys ARCv2.3 64-bit
  EM_MCS6502 = 254,              // MOS Technology MCS 6502 processor
  EM_ARC_COMPACT3 = 255,         // Synopsys ARCv2.3 32-bit
  EM_KVX = 256,                  // Kalray VLIW core of the MPPA processor family
  EM_65816 = 257,                // WDC 65816/65C816
  EM_LOONGARCH = 258,            // Loongson Loongarch
  EM_KF32 = 259,                 // ChipON KungFu32
  EM_MT = 0x2530,                // Morpho Techologies MT processor
  EM_ALPHA = 0x9026,             // Alpha
  EM_WEBASSEMBLY = 0x4157,       // Web Assembly
  EM_DLX = 0x5aa5,               // OpenDL
  EM_PEP9 = 0x7038,              // Pep/8  "p8"
  EM_PEP8 = 0x7039,              // Pep/8  "p9"
  EM_PEP10 = 0x7078,             // Pep/10 "px"
  EM_XSTORMY16 = 0xad45,         // Sanyo XStormy16 CPU core
  EM_IQ2000 = 0xFEBA,            // Vitesse IQ2000
  EM_NIOS32 = 0xFEBB,            // Altera Nios
  EM_CYGNUS_MEP = 0xF00D,        // Toshiba MeP Media Engine
  EM_ADAPTEVA_EPIPHANY = 0x1223, // Adapteva EPIPHANY
  EM_CYGNUS_FRV = 0x5441,        // Fujitsu FR-V
  EM_S12Z = 0x4DEF,              // Freescale S12Z
};

enum class ElfVersion : u8 { EV_NONE = 0, EV_CURRENT = 1 };

enum class ElfIdentifierIndices {
  EI_MAG0 = 0,
  EI_MAG1 = 1,
  EI_MAG2 = 2,
  EI_MAG3 = 3,
  EI_CLASS = 4,
  EI_DATA = 5,
  EI_VERSION = 6,
  EI_OSABI = 7,
  EI_ABIVERSION = 8,
  EI_PAD = 9,
  EI_NIDENT = 16
};

enum class ElfMagic : u8 { ELFMAG0 = 0x7F, ELFMAG1 = 'E', ELFMAG2 = 'L', ELFMAG3 = 'F' };

enum class ElfEncoding : u8 { ELFDATANONE = 0, ELFDATA2LSB = 1, ELFDATA2MSB = 2 };

enum class ElfClass : u8 { ELFCLASSNONE = 0, ELFCLASS32 = 1, ELFCLASS64 = 2 };

enum class ElfABI : u8 {
  ELFOSABI_NONE = 0,         // No extensions or unspecified
  ELFOSABI_STANDALONE = 255, // Standalone (embedded) application
};

// ELF section header enumerated constants
enum class SectionIndices : u32 {
  SHN_UNDEF = 0,
  SHN_LORESERVE = 0xFF00,
  SHN_LOPROC = 0xFF00,
  SHN_HIPROC = 0xFF1F,
  SHN_LOOS = 0xFF20,
  SHN_HIOS = 0xFF3F,
  SHN_ABS = 0xFFF1,
  SHN_COMMON = 0xFFF2,
  SHN_XINDEX = 0xFFFF,
  SHN_HIRESERVE = 0xFFFF
};

enum class SectionTypes : u32 {
  SHT_NULL = 0,
  SHT_PROGBITS = 1,
  SHT_SYMTAB = 2,
  SHT_STRTAB = 3,
  SHT_RELA = 4,
  SHT_HASH = 5,
  SHT_DYNAMIC = 6,
  SHT_NOTE = 7,
  SHT_NOBITS = 8,
  SHT_REL = 9,
  SHT_SHLIB = 10,
  SHT_DYNSYM = 11,
  SHT_INIT_ARRAY = 14,
  SHT_FINI_ARRAY = 15,
  SHT_PREINIT_ARRAY = 16,
  SHT_GROUP = 17,
  SHT_SYMTAB_SHNDX = 18,
  SHT_GNU_ATTRIBUTES = 0x6ffffff5,
  SHT_GNU_HASH = 0x6ffffff6,
  SHT_GNU_LIBLIST = 0x6ffffff7,
  SHT_CHECKSUM = 0x6ffffff8,
  SHT_LOSUNW = 0x6ffffffa,
  SHT_SUNW_move = 0x6ffffffa,
  SHT_SUNW_COMDAT = 0x6ffffffb,
  SHT_SUNW_syminfo = 0x6ffffffc,
  SHT_GNU_verdef = 0x6ffffffd,
  SHT_GNU_verneed = 0x6ffffffe,
  SHT_GNU_versym = 0x6fffffff,
  SHT_LOOS = 0x60000000,
  SHT_HIOS = 0x6fffffff,
  SHT_LOPROC = 0x70000000,
  SHT_ARM_EXIDX = 0x70000001,
  SHT_ARM_PREEMPTMAP = 0x70000002,
  SHT_ARM_ATTRIBUTES = 0x70000003,
  SHT_ARM_DEBUGOVERLAY = 0x70000004,
  SHT_ARM_OVERLAYSECTION = 0x70000005,
  SHT_HIPROC = 0x7FFFFFFF,
  SHT_LOUSER = 0x80000000,
  SHT_HIUSER = 0xFFFFFFFF
};

// Can fit in 32 bits for 32bit targets, but use wider u64 to accomodate 64-bit targets.
enum class SectionFlags : u64 {
  SHF_WRITE = 0x1,
  SHF_ALLOC = 0x2,
  SHF_EXECINSTR = 0x4,
  SHF_MERGE = 0x10,
  SHF_STRINGS = 0x20,
  SHF_INFO_LINK = 0x40,
  SHF_LINK_ORDER = 0x80,
  SHF_OS_NONCONFORMING = 0x100,
  SHF_GROUP = 0x200,
  SHF_TLS = 0x400,
  SHF_COMPRESSED = 0x800,
  SHF_GNU_RETAIN = 0x200000,
  SHF_GNU_MBIND = 0x01000000,
  SHF_MASKOS = 0x0FF00000,
  SHF_MIPS_GPREL = 0x10000000,
  SHF_ORDERED = 0x40000000,
  SHF_EXCLUDE = 0x80000000,
  SHF_MASKPROC = 0xF0000000
};
consteval void is_bitflags(SectionFlags);

// Elf program header (i.e.,, segment) enumerated constants
enum class SegmentType : u32 {
  PT_NULL = 0,
  PT_LOAD = 1,
  PT_DYNAMIC = 2,
  PT_INTERP = 3,
  PT_NOTE = 4,
  PT_SHLIB = 5,
  PT_PHDR = 6,
  PT_TLS = 7,
  PT_GNU_EH_FRAME = 0x6474e550,
  PT_GNU_STACK = 0x6474e551,
  PT_GNU_RELRO = 0x6474e552,
  PT_GNU_PROPERTY = 0x6474e553,
  PT_OPENBSD_RANDOMIZE = 0x65a3dbe6,
  PT_ARM_EXIDX = 0x70000001,
  PT_RISCV_ATTRIBUTES = 0x70000003,
};

enum class SegmentFlags : u32 {
  PF_NONE = 0,
  PF_X = 1,
  PF_W = 2,
  PF_R = 4,
};
consteval void is_bitflags(SegmentFlags);

// ELF symbol enumerated constants
enum class SymbolBinding : u8 {
  STB_LOCAL = 0,
  STB_GLOBAL = 1,
  STB_WEAK = 2,
  STB_LOOS = 10,
  STB_HIOS = 12,
  STB_MULTIDEF = 13,
  STB_LOPROC = 13,
  STB_HIPROC = 15
};

enum class SymbolType : u8 {
  STT_NOTYPE = 0,
  STT_OBJECT = 1,
  STT_FUNC = 2,
  STT_SECTION = 3,
  STT_FILE = 4,
  STT_COMMON = 5,
  STT_TLS = 6,
  STT_LOOS = 10,
  STT_HIOS = 12,
  STT_LOPROC = 13,
  STT_HIPROC = 15
};

enum class SymbolVisibility : u8 {
  STV_DEFAULT = 0,
  STV_INTERNAL = 1,
  STV_HIDDEN = 2,
  STV_PROTECTED = 3,
};

// ELF Relocations
enum class Relocations : u32 {
  R_NONE = 0,
};

enum class Relocationsx86_64 : u32 {
  R_X86_64_NONE = 0,
  R_X86_64_64 = 1,
  R_X86_64_PC32 = 2,
  R_X86_64_GOT32 = 3,
  R_X86_64_PLT32 = 4,
  R_X86_64_COPY = 5,
  R_X86_64_GLOB_DAT = 6,
  R_X86_64_JUMP_SLOT = 7,
  R_X86_64_RELATIVE = 8,
  R_X86_64_GOTPCREL = 9,
  R_X86_64_32 = 10,
  R_X86_64_32S = 11,
  R_X86_64_16 = 12,
  R_X86_64_PC16 = 13,
  R_X86_64_8 = 14,
  R_X86_64_PC8 = 15,
  R_X86_64_DTPMOD64 = 16,
  R_X86_64_DTPOFF64 = 17,
  R_X86_64_TPOFF64 = 18,
  R_X86_64_TLSGD = 19,
  R_X86_64_TLSLD = 20,
  R_X86_64_DTPOFF32 = 21,
  R_X86_64_GOTTPOFF = 22,
  R_X86_64_TPOFF32 = 23,
  R_X86_64_PC64 = 24,
  R_X86_64_GOTOFF64 = 25,
  R_X86_64_GOTPC32 = 26,
  R_X86_64_GOT64 = 27,
  R_X86_64_GOTPCREL64 = 28,
  R_X86_64_GOTPC64 = 29,
  R_X86_64_GOTPLT64 = 30,
  R_X86_64_PLTOFF64 = 31,
  R_X86_64_SIZE32 = 32,
  R_X86_64_SIZE64 = 33,
  R_X86_64_GOTPC32_TLSDESC = 34,
  R_X86_64_TLSDESC_CALL = 35,
  R_X86_64_TLSDESC = 36,
  R_X86_64_IRELATIVE = 37,
  R_X86_64_GOTPCRELX = 41,
  R_X86_64_REX_GOTPCRELX = 42,
  R_X86_64_CODE_4_GOTPCRELX = 43,
  R_X86_64_CODE_4_GOTTPOFF = 44,
  R_X86_64_CODE_4_GOTPC32_TLSDESC = 45,
  R_X86_64_CODE_5_GOTPCRELX = 46,
  R_X86_64_CODE_5_GOTTPOFF = 47,
  R_X86_64_CODE_5_GOTPC32_TLSDESC = 48,
  R_X86_64_CODE_6_GOTPCRELX = 49,
  R_X86_64_CODE_6_GOTTPOFF = 50,
  R_X86_64_CODE_6_GOTPC32_TLSDESC = 51,
};

enum class Relocations386 : u32 {
  R_386_NONE = 0,
  R_386_32 = 1,
  R_386_PC32 = 2,
  R_386_GOT32 = 3,
  R_386_PLT32 = 4,
  R_386_COPY = 5,
  R_386_GLOB_DAT = 6,
  R_386_JUMP_SLOT = 7,
  R_386_RELATIVE = 8,
  R_386_GOTOFF = 9,
  R_386_GOTPC = 10,
  R_386_32PLT = 11,
  R_386_TLS_TPOFF = 14,
  R_386_TLS_IE = 15,
  R_386_TLS_GOTIE = 16,
  R_386_TLS_LE = 17,
  R_386_TLS_GD = 18,
  R_386_TLS_LDM = 19,
  R_386_16 = 20,
  R_386_PC16 = 21,
  R_386_8 = 22,
  R_386_PC8 = 23,
  R_386_TLS_GD_32 = 24,
  R_386_TLS_GD_PUSH = 25,
  R_386_TLS_GD_CALL = 26,
  R_386_TLS_GD_POP = 27,
  R_386_TLS_LDM_32 = 28,
  R_386_TLS_LDM_PUSH = 29,
  R_386_TLS_LDM_CALL = 30,
  R_386_TLS_LDM_POP = 31,
  R_386_TLS_LDO_32 = 32,
  R_386_TLS_IE_32 = 33,
  R_386_TLS_LE_32 = 34,
  R_386_TLS_DTPMOD32 = 35,
  R_386_TLS_DTPOFF32 = 36,
  R_386_TLS_TPOFF32 = 37,
  R_386_SIZE32 = 38,
  R_386_TLS_GOTDESC = 39,
  R_386_TLS_DESC_CALL = 40,
  R_386_TLS_DESC = 41,
  R_386_IRELATIVE = 42,
  R_386_GOT32X = 43,
};

enum class RelocationsAARCH64 : u32 {
  R_AARCH64_NONE = 0,
  R_AARCH64_ABS64 = 0x101,
  R_AARCH64_ABS32 = 0x102,
  R_AARCH64_ABS16 = 0x103,
  R_AARCH64_PREL64 = 0x104,
  R_AARCH64_PREL32 = 0x105,
  R_AARCH64_PREL16 = 0x106,
  R_AARCH64_MOVW_UABS_G0 = 0x107,
  R_AARCH64_MOVW_UABS_G0_NC = 0x108,
  R_AARCH64_MOVW_UABS_G1 = 0x109,
  R_AARCH64_MOVW_UABS_G1_NC = 0x10a,
  R_AARCH64_MOVW_UABS_G2 = 0x10b,
  R_AARCH64_MOVW_UABS_G2_NC = 0x10c,
  R_AARCH64_MOVW_UABS_G3 = 0x10d,
  R_AARCH64_MOVW_SABS_G0 = 0x10e,
  R_AARCH64_MOVW_SABS_G1 = 0x10f,
  R_AARCH64_MOVW_SABS_G2 = 0x110,
  R_AARCH64_LD_PREL_LO19 = 0x111,
  R_AARCH64_ADR_PREL_LO21 = 0x112,
  R_AARCH64_ADR_PREL_PG_HI21 = 0x113,
  R_AARCH64_ADR_PREL_PG_HI21_NC = 0x114,
  R_AARCH64_ADD_ABS_LO12_NC = 0x115,
  R_AARCH64_LDST8_ABS_LO12_NC = 0x116,
  R_AARCH64_TSTBR14 = 0x117,
  R_AARCH64_CONDBR19 = 0x118,
  R_AARCH64_JUMP26 = 0x11a,
  R_AARCH64_CALL26 = 0x11b,
  R_AARCH64_LDST16_ABS_LO12_NC = 0x11c,
  R_AARCH64_LDST32_ABS_LO12_NC = 0x11d,
  R_AARCH64_LDST64_ABS_LO12_NC = 0x11e,
  R_AARCH64_MOVW_PREL_G0 = 0x11f,
  R_AARCH64_MOVW_PREL_G0_NC = 0x120,
  R_AARCH64_MOVW_PREL_G1 = 0x121,
  R_AARCH64_MOVW_PREL_G1_NC = 0x122,
  R_AARCH64_MOVW_PREL_G2 = 0x123,
  R_AARCH64_MOVW_PREL_G2_NC = 0x124,
  R_AARCH64_MOVW_PREL_G3 = 0x125,
  R_AARCH64_LDST128_ABS_LO12_NC = 0x12b,
  R_AARCH64_ADR_GOT_PAGE = 0x137,
  R_AARCH64_LD64_GOT_LO12_NC = 0x138,
  R_AARCH64_LD64_GOTPAGE_LO15 = 0x139,
  R_AARCH64_PLT32 = 0x13a,
  R_AARCH64_TLSGD_ADR_PREL21 = 0x200,
  R_AARCH64_TLSGD_ADR_PAGE21 = 0x201,
  R_AARCH64_TLSGD_ADD_LO12_NC = 0x202,
  R_AARCH64_TLSGD_MOVW_G1 = 0x203,
  R_AARCH64_TLSGD_MOVW_G0_NC = 0x204,
  R_AARCH64_TLSLD_ADR_PREL21 = 0x205,
  R_AARCH64_TLSLD_ADR_PAGE21 = 0x206,
  R_AARCH64_TLSLD_ADD_LO12_NC = 0x207,
  R_AARCH64_TLSLD_MOVW_G1 = 0x208,
  R_AARCH64_TLSLD_MOVW_G0_NC = 0x209,
  R_AARCH64_TLSLD_LD_PREL19 = 0x20a,
  R_AARCH64_TLSLD_MOVW_DTPREL_G2 = 0x20b,
  R_AARCH64_TLSLD_MOVW_DTPREL_G1 = 0x20c,
  R_AARCH64_TLSLD_MOVW_DTPREL_G1_NC = 0x20d,
  R_AARCH64_TLSLD_MOVW_DTPREL_G0 = 0x20e,
  R_AARCH64_TLSLD_MOVW_DTPREL_G0_NC = 0x20f,
  R_AARCH64_TLSLD_ADD_DTPREL_HI12 = 0x210,
  R_AARCH64_TLSLD_ADD_DTPREL_LO12 = 0x211,
  R_AARCH64_TLSLD_ADD_DTPREL_LO12_NC = 0x212,
  R_AARCH64_TLSLD_LDST8_DTPREL_LO12 = 0x213,
  R_AARCH64_TLSLD_LDST8_DTPREL_LO12_NC = 0x214,
  R_AARCH64_TLSLD_LDST16_DTPREL_LO12 = 0x215,
  R_AARCH64_TLSLD_LDST16_DTPREL_LO12_NC = 0x216,
  R_AARCH64_TLSLD_LDST32_DTPREL_LO12 = 0x217,
  R_AARCH64_TLSLD_LDST32_DTPREL_LO12_NC = 0x218,
  R_AARCH64_TLSLD_LDST64_DTPREL_LO12 = 0x219,
  R_AARCH64_TLSLD_LDST64_DTPREL_LO12_NC = 0x21a,
  R_AARCH64_TLSIE_MOVW_GOTTPREL_G1 = 0x21b,
  R_AARCH64_TLSIE_MOVW_GOTTPREL_G0_NC = 0x21c,
  R_AARCH64_TLSIE_ADR_GOTTPREL_PAGE21 = 0x21d,
  R_AARCH64_TLSIE_LD64_GOTTPREL_LO12_NC = 0x21e,
  R_AARCH64_TLSIE_LD_GOTTPREL_PREL19 = 0x21f,
  R_AARCH64_TLSLE_MOVW_TPREL_G2 = 0x220,
  R_AARCH64_TLSLE_MOVW_TPREL_G1 = 0x221,
  R_AARCH64_TLSLE_MOVW_TPREL_G1_NC = 0x222,
  R_AARCH64_TLSLE_MOVW_TPREL_G0 = 0x223,
  R_AARCH64_TLSLE_MOVW_TPREL_G0_NC = 0x224,
  R_AARCH64_TLSLE_ADD_TPREL_HI12 = 0x225,
  R_AARCH64_TLSLE_ADD_TPREL_LO12 = 0x226,
  R_AARCH64_TLSLE_ADD_TPREL_LO12_NC = 0x227,
  R_AARCH64_TLSLE_LDST8_TPREL_LO12 = 0x228,
  R_AARCH64_TLSLE_LDST8_TPREL_LO12_NC = 0x229,
  R_AARCH64_TLSLE_LDST16_TPREL_LO12 = 0x22a,
  R_AARCH64_TLSLE_LDST16_TPREL_LO12_NC = 0x22b,
  R_AARCH64_TLSLE_LDST32_TPREL_LO12 = 0x22c,
  R_AARCH64_TLSLE_LDST32_TPREL_LO12_NC = 0x22d,
  R_AARCH64_TLSLE_LDST64_TPREL_LO12 = 0x22e,
  R_AARCH64_TLSLE_LDST64_TPREL_LO12_NC = 0x22f,
  R_AARCH64_TLSDESC_ADR_PAGE21 = 0x232,
  R_AARCH64_TLSDESC_LD64_LO12 = 0x233,
  R_AARCH64_TLSDESC_ADD_LO12 = 0x234,
  R_AARCH64_TLSDESC_CALL = 0x239,
  R_AARCH64_TLSLE_LDST128_TPREL_LO12_NC = 0x23b,
  R_AARCH64_COPY = 0x400,
  R_AARCH64_GLOB_DAT = 0x401,
  R_AARCH64_JUMP_SLOT = 0x402,
  R_AARCH64_RELATIVE = 0x403,
  R_AARCH64_TLS_DTPMOD64 = 0x404,
  R_AARCH64_TLS_DTPREL64 = 0x405,
  R_AARCH64_TLS_TPREL64 = 0x406,
  R_AARCH64_TLSDESC = 0x407,
  R_AARCH64_IRELATIVE = 0x408,
};

enum class RelocationsARM : u32 {
  R_ARM_NONE = 0x0,
  R_ARM_PC24 = 0x1,
  R_ARM_ABS32 = 0x2,
  R_ARM_REL32 = 0x3,
  R_ARM_LDR_PC_G0 = 0x4,
  R_ARM_ABS16 = 0x5,
  R_ARM_ABS12 = 0x6,
  R_ARM_THM_ABS5 = 0x7,
  R_ARM_ABS8 = 0x8,
  R_ARM_SBREL32 = 0x9,
  R_ARM_THM_CALL = 0xa,
  R_ARM_THM_PC8 = 0xb,
  R_ARM_BREL_ADJ = 0xc,
  R_ARM_TLS_DESC = 0xd,
  R_ARM_THM_SWI8 = 0xe,
  R_ARM_XPC25 = 0xf,
  R_ARM_THM_XPC22 = 0x10,
  R_ARM_TLS_DTPMOD32 = 0x11,
  R_ARM_TLS_DTPOFF32 = 0x12,
  R_ARM_TLS_TPOFF32 = 0x13,
  R_ARM_COPY = 0x14,
  R_ARM_GLOB_DAT = 0x15,
  R_ARM_JUMP_SLOT = 0x16,
  R_ARM_RELATIVE = 0x17,
  R_ARM_GOTOFF32 = 0x18,
  R_ARM_BASE_PREL = 0x19,
  R_ARM_GOT_BREL = 0x1a,
  R_ARM_PLT32 = 0x1b,
  R_ARM_CALL = 0x1c,
  R_ARM_JUMP24 = 0x1d,
  R_ARM_THM_JUMP24 = 0x1e,
  R_ARM_BASE_ABS = 0x1f,
  R_ARM_ALU_PCREL_7_0 = 0x20,
  R_ARM_ALU_PCREL_15_8 = 0x21,
  R_ARM_ALU_PCREL_23_15 = 0x22,
  R_ARM_LDR_SBREL_11_0_NC = 0x23,
  R_ARM_ALU_SBREL_19_12_NC = 0x24,
  R_ARM_ALU_SBREL_27_20_CK = 0x25,
  R_ARM_TARGET1 = 0x26,
  R_ARM_SBREL31 = 0x27,
  R_ARM_V4BX = 0x28,
  R_ARM_TARGET2 = 0x29,
  R_ARM_PREL31 = 0x2a,
  R_ARM_MOVW_ABS_NC = 0x2b,
  R_ARM_MOVT_ABS = 0x2c,
  R_ARM_MOVW_PREL_NC = 0x2d,
  R_ARM_MOVT_PREL = 0x2e,
  R_ARM_THM_MOVW_ABS_NC = 0x2f,
  R_ARM_THM_MOVT_ABS = 0x30,
  R_ARM_THM_MOVW_PREL_NC = 0x31,
  R_ARM_THM_MOVT_PREL = 0x32,
  R_ARM_THM_JUMP19 = 0x33,
  R_ARM_THM_JUMP6 = 0x34,
  R_ARM_THM_ALU_PREL_11_0 = 0x35,
  R_ARM_THM_PC12 = 0x36,
  R_ARM_ABS32_NOI = 0x37,
  R_ARM_REL32_NOI = 0x38,
  R_ARM_ALU_PC_G0_NC = 0x39,
  R_ARM_ALU_PC_G0 = 0x3a,
  R_ARM_ALU_PC_G1_NC = 0x3b,
  R_ARM_ALU_PC_G1 = 0x3c,
  R_ARM_ALU_PC_G2 = 0x3d,
  R_ARM_LDR_PC_G1 = 0x3e,
  R_ARM_LDR_PC_G2 = 0x3f,
  R_ARM_LDRS_PC_G0 = 0x40,
  R_ARM_LDRS_PC_G1 = 0x41,
  R_ARM_LDRS_PC_G2 = 0x42,
  R_ARM_LDC_PC_G0 = 0x43,
  R_ARM_LDC_PC_G1 = 0x44,
  R_ARM_LDC_PC_G2 = 0x45,
  R_ARM_ALU_SB_G0_NC = 0x46,
  R_ARM_ALU_SB_G0 = 0x47,
  R_ARM_ALU_SB_G1_NC = 0x48,
  R_ARM_ALU_SB_G1 = 0x49,
  R_ARM_ALU_SB_G2 = 0x4a,
  R_ARM_LDR_SB_G0 = 0x4b,
  R_ARM_LDR_SB_G1 = 0x4c,
  R_ARM_LDR_SB_G2 = 0x4d,
  R_ARM_LDRS_SB_G0 = 0x4e,
  R_ARM_LDRS_SB_G1 = 0x4f,
  R_ARM_LDRS_SB_G2 = 0x50,
  R_ARM_LDC_SB_G0 = 0x51,
  R_ARM_LDC_SB_G1 = 0x52,
  R_ARM_LDC_SB_G2 = 0x53,
  R_ARM_MOVW_BREL_NC = 0x54,
  R_ARM_MOVT_BREL = 0x55,
  R_ARM_MOVW_BREL = 0x56,
  R_ARM_THM_MOVW_BREL_NC = 0x57,
  R_ARM_THM_MOVT_BREL = 0x58,
  R_ARM_THM_MOVW_BREL = 0x59,
  R_ARM_TLS_GOTDESC = 0x5a,
  R_ARM_TLS_CALL = 0x5b,
  R_ARM_TLS_DESCSEQ = 0x5c,
  R_ARM_THM_TLS_CALL = 0x5d,
  R_ARM_PLT32_ABS = 0x5e,
  R_ARM_GOT_ABS = 0x5f,
  R_ARM_GOT_PREL = 0x60,
  R_ARM_GOT_BREL12 = 0x61,
  R_ARM_GOTOFF12 = 0x62,
  R_ARM_GOTRELAX = 0x63,
  R_ARM_GNU_VTENTRY = 0x64,
  R_ARM_GNU_VTINHERIT = 0x65,
  R_ARM_THM_JUMP11 = 0x66,
  R_ARM_THM_JUMP8 = 0x67,
  R_ARM_TLS_GD32 = 0x68,
  R_ARM_TLS_LDM32 = 0x69,
  R_ARM_TLS_LDO32 = 0x6a,
  R_ARM_TLS_IE32 = 0x6b,
  R_ARM_TLS_LE32 = 0x6c,
  R_ARM_TLS_LDO12 = 0x6d,
  R_ARM_TLS_LE12 = 0x6e,
  R_ARM_TLS_IE12GP = 0x6f,
  R_ARM_PRIVATE_0 = 0x70,
  R_ARM_PRIVATE_1 = 0x71,
  R_ARM_PRIVATE_2 = 0x72,
  R_ARM_PRIVATE_3 = 0x73,
  R_ARM_PRIVATE_4 = 0x74,
  R_ARM_PRIVATE_5 = 0x75,
  R_ARM_PRIVATE_6 = 0x76,
  R_ARM_PRIVATE_7 = 0x77,
  R_ARM_PRIVATE_8 = 0x78,
  R_ARM_PRIVATE_9 = 0x79,
  R_ARM_PRIVATE_10 = 0x7a,
  R_ARM_PRIVATE_11 = 0x7b,
  R_ARM_PRIVATE_12 = 0x7c,
  R_ARM_PRIVATE_13 = 0x7d,
  R_ARM_PRIVATE_14 = 0x7e,
  R_ARM_PRIVATE_15 = 0x7f,
  R_ARM_ME_TOO = 0x80,
  R_ARM_THM_TLS_DESCSEQ16 = 0x81,
  R_ARM_THM_TLS_DESCSEQ32 = 0x82,
  R_ARM_THM_BF16 = 0x88,
  R_ARM_THM_BF12 = 0x89,
  R_ARM_THM_BF18 = 0x8a,
  R_ARM_IRELATIVE = 0xa0,
};

enum class RelocationsRISCV : u32 {
  R_RISCV_NONE = 0,
  R_RISCV_32 = 1,
  R_RISCV_64 = 2,
  R_RISCV_RELATIVE = 3,
  R_RISCV_COPY = 4,
  R_RISCV_JUMP_SLOT = 5,
  R_RISCV_TLS_DTPMOD32 = 6,
  R_RISCV_TLS_DTPMOD64 = 7,
  R_RISCV_TLS_DTPREL32 = 8,
  R_RISCV_TLS_DTPREL64 = 9,
  R_RISCV_TLS_TPREL32 = 10,
  R_RISCV_TLS_TPREL64 = 11,
  R_RISCV_TLSDESC = 12,
  R_RISCV_BRANCH = 16,
  R_RISCV_JAL = 17,
  R_RISCV_CALL = 18,
  R_RISCV_CALL_PLT = 19,
  R_RISCV_GOT_HI20 = 20,
  R_RISCV_TLS_GOT_HI20 = 21,
  R_RISCV_TLS_GD_HI20 = 22,
  R_RISCV_PCREL_HI20 = 23,
  R_RISCV_PCREL_LO12_I = 24,
  R_RISCV_PCREL_LO12_S = 25,
  R_RISCV_HI20 = 26,
  R_RISCV_LO12_I = 27,
  R_RISCV_LO12_S = 28,
  R_RISCV_TPREL_HI20 = 29,
  R_RISCV_TPREL_LO12_I = 30,
  R_RISCV_TPREL_LO12_S = 31,
  R_RISCV_TPREL_ADD = 32,
  R_RISCV_ADD8 = 33,
  R_RISCV_ADD16 = 34,
  R_RISCV_ADD32 = 35,
  R_RISCV_ADD64 = 36,
  R_RISCV_SUB8 = 37,
  R_RISCV_SUB16 = 38,
  R_RISCV_SUB32 = 39,
  R_RISCV_SUB64 = 40,
  R_RISCV_ALIGN = 43,
  R_RISCV_RVC_BRANCH = 44,
  R_RISCV_RVC_JUMP = 45,
  R_RISCV_RELAX = 51,
  R_RISCV_SUB6 = 52,
  R_RISCV_SET6 = 53,
  R_RISCV_SET8 = 54,
  R_RISCV_SET16 = 55,
  R_RISCV_SET32 = 56,
  R_RISCV_32_PCREL = 57,
  R_RISCV_IRELATIVE = 58,
  R_RISCV_PLT32 = 59,
  R_RISCV_SET_ULEB128 = 60,
  R_RISCV_SUB_ULEB128 = 61,
  R_RISCV_TLSDESC_HI20 = 62,
  R_RISCV_TLSDESC_LOAD_LO12 = 63,
  R_RISCV_TLSDESC_ADD_LO12 = 64,
  R_RISCV_TLSDESC_CALL = 65,
};

// Dynamic Array Tag
enum class DynamicTags : u32 {
  DT_NULL = 0,
  DT_NEEDED = 1,
  DT_PLTRELSZ = 2,
  DT_PLTGOT = 3,
  DT_HASH = 4,
  DT_STRTAB = 5,
  DT_SYMTAB = 6,
  DT_RELA = 7,
  DT_RELASZ = 8,
  DT_RELAENT = 9,
  DT_STRSZ = 10,
  DT_SYMENT = 11,
  DT_INIT = 12,
  DT_FINI = 13,
  DT_SONAME = 14,
  DT_RPATH = 15,
  DT_SYMBOLIC = 16,
  DT_REL = 17,
  DT_RELSZ = 18,
  DT_RELENT = 19,
  DT_PLTREL = 20,
  DT_DEBUG = 21,
  DT_TEXTREL = 22,
  DT_JMPREL = 23,
  DT_BIND_NOW = 24,
  DT_INIT_ARRAY = 25,
  DT_FINI_ARRAY = 26,
  DT_INIT_ARRAYSZ = 27,
  DT_FINI_ARRAYSZ = 28,
  DT_RUNPATH = 29,
  DT_FLAGS = 30,
  DT_ENCODING = 32,
  DT_PREINIT_ARRAY = 32,
  DT_PREINIT_ARRAYSZ = 33,
  DT_MAXPOSTAGS = 34,
  DT_LOOS = 0x6000000D,
  DT_HIOS = 0x6ffff000,
  DT_GNU_HASH = 0x6ffffef5,
  DT_TLSDESC_PLT = 0x6ffffef6,
  DT_TLSDESC_GOT = 0x6ffffef7,
  DT_GNU_CONFLICT = 0x6ffffef8,
  DT_GNU_LIBLIST = 0x6ffffef9,
  DT_CONFIG = 0x6ffffefa,
  DT_DEPAUDIT = 0x6ffffefb,
  DT_AUDIT = 0x6ffffefc,
  DT_PLTPAD = 0x6ffffefd,
  DT_MOVETAB = 0x6ffffefe,
  DT_SYMINFO = 0x6ffffeff,
  DT_ADDRRNGHI = 0x6ffffeff,
  DT_VERSYM = 0x6ffffff0,
  DT_RELACOUNT = 0x6ffffff9,
  DT_RELCOUNT = 0x6ffffffa,
  DT_FLAGS_1 = 0x6ffffffb,
  DT_VERDEF = 0x6ffffffc,
  DT_VERDEFNUM = 0x6ffffffd,
  DT_VERNEED = 0x6ffffffe,
  DT_VERNEEDNUM = 0x6fffffff,
  DT_LOPROC = 0x70000000,
  DT_HIPROC = 0x7FFFFFFF
};
} // namespace pepp::core
