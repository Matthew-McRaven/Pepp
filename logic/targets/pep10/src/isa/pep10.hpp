#pragma once

#include <array>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "magic_enum.hpp"

namespace isa::pep10 {
enum class instruction_mnemonic {

    RET,
    SRET,
    MOVSPA,
    MOVASP,
    MOVFLGA,
    MOVAFLG,
    MOVTA,
    USCALL,
    NOP,

    // FAULTS
    UNIMPL,

    NOTA,
    NOTX,
    NEGA,
    NEGX,
    ASLA,
    ASLX,
    ASRA,
    ASRX,
    ROLA,
    ROLX,
    RORA,
    RORX,
    // STOP,
    BR,
    BRLE,
    BRLT,
    BREQ,
    BRNE,
    BRGE,
    BRGT,
    BRV,
    BRC,
    CALL,
    SCALL,
    LDWT,
    LDWA,
    LDWX,
    LDBA,
    LDBX,
    STWA,
    STWX,
    STBA,
    STBX,
    CPWA,
    CPWX,
    CPBA,
    CPBX,
    ADDA,
    ADDX,
    SUBA,
    SUBX,
    ANDA,
    ANDX,
    ORA,
    ORX,
    XORA,
    XORX,
    ADDSP,
    SUBSP,
};

enum class Register {
    A = 0,
    X = 1,
    SP = 2,
    PC = 3,
    IS = 4,
    OS = 5,
    TR = 6,
};

enum class addressing_mode {
    NONE = 0,
    I = 1,
    D = 2,
    N = 4,
    S = 8,
    SF = 16,
    X = 32,
    SX = 64,
    SFX = 128,
    ALL = 255,
    INVALID
};

enum class addressing_class {
    Invalid,
    U_none,   //?
    R_none,   //?
    A_ix,     //?
    AAA_all,  //?
    AAA_i,    //?
    RAAA_all, //?
    RAAA_noi

};
enum class MemoryVector {
    kUser_Stack,
    kSystem_Stack,
    kPower_Off_Port,
    kDispatcher,
    kLoader,
    kTrap_Handler,
};

enum class CSR {
    N,
    Z,
    V,
    C,
};

struct instruction_definition {
    uint8_t bit_pattern = 0;
    addressing_class iformat = addressing_class::Invalid;
    std::array<bool, magic_enum::enum_count<CSR>()> CSR_modified = {
        false}; // Flag which CSR bits are changed by this instruction
    instruction_mnemonic mnemonic = instruction_mnemonic::RET;
    bool is_unary = false;
    std::string comment = {};
};
struct addr_map {
    std::shared_ptr<instruction_definition> inst;
    addressing_mode addr;
};

struct isa_definition {
    const std::map<instruction_mnemonic, std::shared_ptr<instruction_definition>> isa;
    const std::array<addr_map, 256> riproll;
    // TODO: Make this constructor private since we have a singleton.
    isa_definition();
    // Returns a static copy of a pep/10 isa definition.
    // Required for static initialization in a static library.
    static const isa_definition &get_definition();
};

std::string as_string(instruction_mnemonic);
std::string as_string(addressing_mode);
bool is_mnemonic_unary(instruction_mnemonic);
bool is_mnemonic_unary(uint8_t);
bool is_opcode_unary(instruction_mnemonic);
bool is_opcode_unary(uint8_t);
bool is_store(instruction_mnemonic);
bool is_store(uint8_t);
// Convert unary instruction definition to its opcode.
uint8_t opcode(instruction_mnemonic);
// Convert nonunary instruction definition to its opcode.
uint8_t opcode(instruction_mnemonic, addressing_mode);

}; // namespace isa::pep10