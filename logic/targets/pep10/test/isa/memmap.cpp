#include <array>

#include "catch.hpp"
#include "isa/pep10.hpp"
using im = isa::pep10::instruction_mnemonic;

using namespace isa::pep10;

std::array<instruction_mnemonic, 256> instr_array = {
    im::RET, im::SRET, im::MOVSPA, im::MOVASP, im::MOVFLGA, im::MOVAFLG, im::MOVTA, im::USCALL, // 0b0000'0000
    im::NOP, im::UNIMPL, im::UNIMPL, im::UNIMPL, im::UNIMPL, im::UNIMPL, im::UNIMPL, im::UNIMPL, // 0b0000'1000
    im::NOTA, im::NOTX, im::NEGA, im::NEGX, im::ASLA, im::ASLX, im::ASRA, im::ASRX,   // 0b0001'0000
    im::ROLA, im::ROLX, im::RORA, im::RORX, im::BR, im::BR, im::BRLE, im::BRLE,   // 0b0001'1000

    im::BRLT, im::BRLT, im::BREQ, im::BREQ, im::BRNE, im::BRNE, im::BRGE, im::BRGE,  // 0b0010'0000
    im::BRGT, im::BRGT, im::BRV, im::BRV, im::BRC, im::BRC, im::CALL, im::CALL,  // 0b0010'1000
    im::SCALL, im::SCALL, im::SCALL, im::SCALL, im::SCALL, im::SCALL, im::SCALL, im::SCALL, // 0b0011'0000
    im::LDWT, im::LDWT, im::LDWT, im::LDWT, im::LDWT, im::LDWT, im::LDWT, im::LDWT,  // 0b0011'1000

    im::LDWA, im::LDWA, im::LDWA, im::LDWA, im::LDWA, im::LDWA, im::LDWA, im::LDWA, // 0b0100'0000
    im::LDWX, im::LDWX, im::LDWX, im::LDWX, im::LDWX, im::LDWX, im::LDWX, im::LDWX, // 0b0100'1000
    im::LDBA, im::LDBA, im::LDBA, im::LDBA, im::LDBA, im::LDBA, im::LDBA, im::LDBA, // 0b0101'0000
    im::LDBX, im::LDBX, im::LDBX, im::LDBX, im::LDBX, im::LDBX, im::LDBX, im::LDBX, // 0b0101'1000

    im::STWA, im::STWA, im::STWA, im::STWA, im::STWA, im::STWA, im::STWA, im::STWA, // 0b0110'0000
    im::STWX, im::STWX, im::STWX, im::STWX, im::STWX, im::STWX, im::STWX, im::STWX, // 0b0110'1000
    im::STBA, im::STBA, im::STBA, im::STBA, im::STBA, im::STBA, im::STBA, im::STBA, // 0b0111'0000
    im::STBX, im::STBX, im::STBX, im::STBX, im::STBX, im::STBX, im::STBX, im::STBX, // 0b0111'1000

    im::CPWA, im::CPWA, im::CPWA, im::CPWA, im::CPWA, im::CPWA, im::CPWA, im::CPWA, // 0b1000'0000
    im::CPWX, im::CPWX, im::CPWX, im::CPWX, im::CPWX, im::CPWX, im::CPWX, im::CPWX, // 0b1000'1000
    im::CPBA, im::CPBA, im::CPBA, im::CPBA, im::CPBA, im::CPBA, im::CPBA, im::CPBA, // 0b1001'0000
    im::CPBX, im::CPBX, im::CPBX, im::CPBX, im::CPBX, im::CPBX, im::CPBX, im::CPBX, // 0b1001'1000

    im::ADDA, im::ADDA, im::ADDA, im::ADDA, im::ADDA, im::ADDA, im::ADDA, im::ADDA, // 0b1010'0000
    im::ADDX, im::ADDX, im::ADDX, im::ADDX, im::ADDX, im::ADDX, im::ADDX, im::ADDX, // 0b1010'1000
    im::SUBA, im::SUBA, im::SUBA, im::SUBA, im::SUBA, im::SUBA, im::SUBA, im::SUBA, // 0b1011'0000
    im::SUBX, im::SUBX, im::SUBX, im::SUBX, im::SUBX, im::SUBX, im::SUBX, im::SUBX, // 0b1011'1000

    im::ANDA, im::ANDA, im::ANDA, im::ANDA, im::ANDA, im::ANDA, im::ANDA, im::ANDA, // 0b1100'0000
    im::ANDX, im::ANDX, im::ANDX, im::ANDX, im::ANDX, im::ANDX, im::ANDX, im::ANDX, // 0b1100'1000
    im::ORA, im::ORA, im::ORA, im::ORA, im::ORA, im::ORA, im::ORA, im::ORA,  // 0b1101'0000
    im::ORX, im::ORX, im::ORX, im::ORX, im::ORX, im::ORX, im::ORX, im::ORX,  // 0b1101'1000
    im::XORA, im::XORA, im::XORA, im::XORA, im::XORA, im::XORA, im::XORA, im::XORA, // 0b1110'0000
    im::XORX, im::XORX, im::XORX, im::XORX, im::XORX, im::XORX, im::XORX, im::XORX, // 0b1110'1000

    im::ADDSP, im::ADDSP, im::ADDSP, im::ADDSP, im::ADDSP, im::ADDSP, im::ADDSP, im::ADDSP, // 0b1111'0000
    im::SUBSP, im::SUBSP, im::SUBSP, im::SUBSP, im::SUBSP, im::SUBSP, im::SUBSP, im::SUBSP  // 0b1111'1000
};

using ad = addressing_mode;
std::array<addressing_mode, 256> addr_array = {
    ad::NONE, ad::NONE, ad::NONE, ad::NONE,
    ad::NONE, ad::NONE, ad::NONE, ad::NONE, // 0b0000'0000
    ad::NONE, ad::INVALID, ad::INVALID, ad::INVALID,
    ad::INVALID, ad::INVALID, ad::INVALID, ad::INVALID, // 0b0000'1000
    ad::NONE, ad::NONE, ad::NONE, ad::NONE,
    ad::NONE, ad::NONE, ad::NONE, ad::NONE, // 0b0001'0000
    ad::NONE, ad::NONE, ad::NONE, ad::NONE,
    ad::I, ad::X, ad::I, ad::X, // 0b0001'1000

    ad::I, ad::X, ad::I, ad::X,
    ad::I, ad::X, ad::I, ad::X, // 0b0010'0000
    ad::I, ad::X, ad::I, ad::X,
    ad::I, ad::X, ad::I, ad::X, // 0b0010'1000
    ad::I, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b0011'0000
    ad::I, ad::INVALID, ad::INVALID, ad::INVALID,
    ad::INVALID, ad::INVALID, ad::INVALID, ad::INVALID, // 0b0011'1000

    ad::I, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b0100'0000
    ad::I, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b0100'1000
    ad::I, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b0101'0000
    ad::I, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b0101'1000`

    ad::INVALID, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b0110'0000
    ad::INVALID, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b0110'1000
    ad::INVALID, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b0111'0000
    ad::INVALID, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b0111'1000

    ad::I, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b1000'0000
    ad::I, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b1000'1000
    ad::I, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b1001'0000
    ad::I, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b1001'1000

    ad::I, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b1010'0000
    ad::I, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b1010'1000
    ad::I, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b1011'0000
    ad::I, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b1011'1000

    ad::I, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b1100'0000
    ad::I, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b1100'1000
    ad::I, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b1101'0000
    ad::I, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b1101'1000
    ad::I, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b1110'0000
    ad::I, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b1110'1000

    ad::I, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b1111'0000
    ad::I, ad::D, ad::N, ad::S,
    ad::SF, ad::X, ad::SX, ad::SFX, // 0b1111'1000
};

// Require that clever instruction definitions matches manually enumerated table.
// These tables confirm that addressing modes, instruction specs are correct for all
// instructions.
TEST_CASE("Validate instruction memory map", "[isa-def]") {
  auto def = isa::pep10::isa_definition::get_definition();

  SECTION("Instructions") {
    for (int it = 0; it < 255; it++) {
      auto mnemon = def.riproll[it].inst->mnemonic;
      auto assert_mnemon = instr_array[it];
      REQUIRE(mnemon == assert_mnemon);
    }
  }

  SECTION("Addressing Modes", "[isa-def]") {
    for (int it = 0; it < 255; it++) {
      auto addr_mode = def.riproll[it].addr;
      auto assert_addr = addr_array[it];
      REQUIRE(addr_mode == assert_addr);
    }
  }
}
