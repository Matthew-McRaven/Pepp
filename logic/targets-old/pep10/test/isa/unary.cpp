#include "catch.hpp"

#include "isa/pep10.hpp"

using namespace isa::pep10;

TEST_CASE("Sanity checks on Unary ISA instructions", "[isa-def]") {
  auto isa_def = isa::pep10::isa_definition::get_definition();

  SECTION("RET") {
    const auto def = isa_def.isa.at(instruction_mnemonic::RET);
    REQUIRE(def->bit_pattern == 0b0000'0000);
    REQUIRE(def->iformat == addressing_class::U_none);
    bool CSR_modified[magic_enum::enum_count<isa::pep10::CSR>()] = {false, false, false, false};
    for (int it = 0; it < magic_enum::enum_count<isa::pep10::CSR>(); it++) {
      REQUIRE(def->CSR_modified[it] == CSR_modified[it]);
    }
    REQUIRE(def->mnemonic == instruction_mnemonic::RET);
    REQUIRE(as_string(def->mnemonic) == "RET");
  }

  SECTION("SRET") {
    const auto def = isa_def.isa.at(instruction_mnemonic::SRET);
    REQUIRE(def->bit_pattern == 0b0000'0001);
    REQUIRE(def->iformat == addressing_class::U_none);
    bool CSR_modified[magic_enum::enum_count<isa::pep10::CSR>()] = {false, false, false, false};
    for (int it = 0; it < magic_enum::enum_count<isa::pep10::CSR>(); it++) {
      REQUIRE(def->CSR_modified[it] == CSR_modified[it]);
    }
    REQUIRE(def->mnemonic == instruction_mnemonic::SRET);
    REQUIRE(as_string(def->mnemonic) == "SRET");
  }

  SECTION("MOVSPA") {
    const auto def = isa_def.isa.at(instruction_mnemonic::MOVSPA);
    REQUIRE(def->bit_pattern == 0b0000'0010);
    REQUIRE(def->iformat == addressing_class::U_none);
    bool CSR_modified[magic_enum::enum_count<isa::pep10::CSR>()] = {false, false, false, false};
    for (int it = 0; it < magic_enum::enum_count<isa::pep10::CSR>(); it++) {
      REQUIRE(def->CSR_modified[it] == CSR_modified[it]);
    }
    REQUIRE(def->mnemonic == instruction_mnemonic::MOVSPA);
    REQUIRE(as_string(def->mnemonic) == "MOVSPA");
  }

  SECTION("MOVASP") {
    const auto def = isa_def.isa.at(instruction_mnemonic::MOVASP);
    REQUIRE(def->bit_pattern == 0b0000'0011);
    REQUIRE(def->iformat == addressing_class::U_none);
    bool CSR_modified[magic_enum::enum_count<isa::pep10::CSR>()] = {false, false, false, false};
    for (int it = 0; it < magic_enum::enum_count<isa::pep10::CSR>(); it++) {
      REQUIRE(def->CSR_modified[it] == CSR_modified[it]);
    }
    REQUIRE(def->mnemonic == instruction_mnemonic::MOVASP);
    REQUIRE(as_string(def->mnemonic) == "MOVASP");
  }

  SECTION("MOVFLGA") {
    const auto def = isa_def.isa.at(instruction_mnemonic::MOVFLGA);
    REQUIRE(def->bit_pattern == 0b0000'0100);
    REQUIRE(def->iformat == addressing_class::U_none);
    bool CSR_modified[magic_enum::enum_count<isa::pep10::CSR>()] = {false, false, false, false};
    for (int it = 0; it < magic_enum::enum_count<isa::pep10::CSR>(); it++) {
      REQUIRE(def->CSR_modified[it] == CSR_modified[it]);
    }
    REQUIRE(def->mnemonic == instruction_mnemonic::MOVFLGA);
    REQUIRE(as_string(def->mnemonic) == "MOVFLGA");
  }

  SECTION("MOVAFLG") {
    const auto def = isa_def.isa.at(instruction_mnemonic::MOVAFLG);
    REQUIRE(def->bit_pattern == 0b0000'0101);
    REQUIRE(def->iformat == addressing_class::U_none);
    bool CSR_modified[magic_enum::enum_count<isa::pep10::CSR>()] = {true, true, true, true};
    for (int it = 0; it < magic_enum::enum_count<isa::pep10::CSR>(); it++) {
      REQUIRE(def->CSR_modified[it] == CSR_modified[it]);
    }
    REQUIRE(def->mnemonic == instruction_mnemonic::MOVAFLG);
    REQUIRE(as_string(def->mnemonic) == "MOVAFLG");
  }

  SECTION("MOVTA") {
    const auto def = isa_def.isa.at(instruction_mnemonic::MOVTA);
    REQUIRE(def->bit_pattern == 0b0000'0110);
    REQUIRE(def->iformat == addressing_class::U_none);
    bool CSR_modified[magic_enum::enum_count<isa::pep10::CSR>()] = {false, false, false, false};
    for (int it = 0; it < magic_enum::enum_count<isa::pep10::CSR>(); it++) {
      REQUIRE(def->CSR_modified[it] == CSR_modified[it]);
    }
    REQUIRE(def->mnemonic == instruction_mnemonic::MOVTA);
    REQUIRE(as_string(def->mnemonic) == "MOVTA");
  }

  SECTION("USCALL") {
    const auto def = isa_def.isa.at(instruction_mnemonic::USCALL);
    REQUIRE(def->bit_pattern == 0b0000'0111);
    REQUIRE(def->iformat == addressing_class::U_none);
    bool CSR_modified[magic_enum::enum_count<isa::pep10::CSR>()] = {false, false, false, false};
    for (int it = 0; it < magic_enum::enum_count<isa::pep10::CSR>(); it++) {
      REQUIRE(def->CSR_modified[it] == CSR_modified[it]);
    }
    REQUIRE(def->mnemonic == instruction_mnemonic::USCALL);
    REQUIRE(as_string(def->mnemonic) == "USCALL");
  }

  SECTION("NOP") {
    const auto def = isa_def.isa.at(instruction_mnemonic::NOP);
    REQUIRE(def->bit_pattern == 0b0000'1000);
    REQUIRE(def->iformat == addressing_class::U_none);
    bool CSR_modified[magic_enum::enum_count<isa::pep10::CSR>()] = {false, false, false, false};
    for (int it = 0; it < magic_enum::enum_count<isa::pep10::CSR>(); it++) {
      REQUIRE(def->CSR_modified[it] == CSR_modified[it]);
    }
    REQUIRE(def->mnemonic == instruction_mnemonic::NOP);
    REQUIRE(as_string(def->mnemonic) == "NOP");
  }
}

TEST_CASE("Sanity checks on Unary `R`-type ISA instructions", "[isa-def]") {
  auto isa_def = isa::pep10::isa_definition::get_definition();

  SECTION("NOTA") {
    const auto def = isa_def.isa.at(instruction_mnemonic::NOTA);
    REQUIRE(def->bit_pattern == 0b0001'0000);
    REQUIRE(def->iformat == addressing_class::R_none);
    bool CSR_modified[magic_enum::enum_count<isa::pep10::CSR>()] = {true, true, false, false};
    for (int it = 0; it < magic_enum::enum_count<isa::pep10::CSR>(); it++) {
      REQUIRE(def->CSR_modified[it] == CSR_modified[it]);
    }
    REQUIRE(def->mnemonic == instruction_mnemonic::NOTA);
    REQUIRE(as_string(def->mnemonic) == "NOTA");
  }

  SECTION("NOTX") {
    const auto def = isa_def.isa.at(instruction_mnemonic::NOTX);
    REQUIRE(def->bit_pattern == 0b0001'0001);
    REQUIRE(def->iformat == addressing_class::R_none);
    bool CSR_modified[magic_enum::enum_count<isa::pep10::CSR>()] = {true, true, false, false};
    for (int it = 0; it < magic_enum::enum_count<isa::pep10::CSR>(); it++) {
      REQUIRE(def->CSR_modified[it] == CSR_modified[it]);
    }
    REQUIRE(def->mnemonic == instruction_mnemonic::NOTX);
    REQUIRE(as_string(def->mnemonic) == "NOTX");
  }

  SECTION("NEGA") {
    const auto def = isa_def.isa.at(instruction_mnemonic::NEGA);
    REQUIRE(def->bit_pattern == 0b0001'0010);
    REQUIRE(def->iformat == addressing_class::R_none);
    bool CSR_modified[magic_enum::enum_count<isa::pep10::CSR>()] = {true, true, true, false};
    for (int it = 0; it < magic_enum::enum_count<isa::pep10::CSR>(); it++) {
      REQUIRE(def->CSR_modified[it] == CSR_modified[it]);
    }
    REQUIRE(def->mnemonic == instruction_mnemonic::NEGA);
    REQUIRE(as_string(def->mnemonic) == "NEGA");
  }

  SECTION("NEGX") {
    const auto def = isa_def.isa.at(instruction_mnemonic::NEGX);
    REQUIRE(def->bit_pattern == 0b0001'0011);
    REQUIRE(def->iformat == addressing_class::R_none);
    bool CSR_modified[magic_enum::enum_count<isa::pep10::CSR>()] = {true, true, true, false};
    for (int it = 0; it < magic_enum::enum_count<isa::pep10::CSR>(); it++) {
      REQUIRE(def->CSR_modified[it] == CSR_modified[it]);
    }
    REQUIRE(def->mnemonic == instruction_mnemonic::NEGX);
    REQUIRE(as_string(def->mnemonic) == "NEGX");
  }

  SECTION("ASLA") {
    const auto def = isa_def.isa.at(instruction_mnemonic::ASLA);
    REQUIRE(def->bit_pattern == 0b0001'0100);
    REQUIRE(def->iformat == addressing_class::R_none);
    bool CSR_modified[magic_enum::enum_count<isa::pep10::CSR>()] = {true, true, true, true};
    for (int it = 0; it < magic_enum::enum_count<isa::pep10::CSR>(); it++) {
      REQUIRE(def->CSR_modified[it] == CSR_modified[it]);
    }
    REQUIRE(def->mnemonic == instruction_mnemonic::ASLA);
    REQUIRE(as_string(def->mnemonic) == "ASLA");
  }

  SECTION("ASLX") {
    const auto def = isa_def.isa.at(instruction_mnemonic::ASLX);
    REQUIRE(def->bit_pattern == 0b0001'0101);
    REQUIRE(def->iformat == addressing_class::R_none);
    bool CSR_modified[magic_enum::enum_count<isa::pep10::CSR>()] = {true, true, true, true};
    for (int it = 0; it < magic_enum::enum_count<isa::pep10::CSR>(); it++) {
      REQUIRE(def->CSR_modified[it] == CSR_modified[it]);
    }
    REQUIRE(def->mnemonic == instruction_mnemonic::ASLX);
    REQUIRE(as_string(def->mnemonic) == "ASLX");
  }

  SECTION("ASRA") {
    const auto def = isa_def.isa.at(instruction_mnemonic::ASRA);
    REQUIRE(def->bit_pattern == 0b0001'0110);
    REQUIRE(def->iformat == addressing_class::R_none);
    bool CSR_modified[magic_enum::enum_count<isa::pep10::CSR>()] = {true, true, false, true};
    for (int it = 0; it < magic_enum::enum_count<isa::pep10::CSR>(); it++) {
      REQUIRE(def->CSR_modified[it] == CSR_modified[it]);
    }
    REQUIRE(def->mnemonic == instruction_mnemonic::ASRA);
    REQUIRE(as_string(def->mnemonic) == "ASRA");
  }

  SECTION("ASRX") {
    const auto def = isa_def.isa.at(instruction_mnemonic::ASRX);
    REQUIRE(def->bit_pattern == 0b0001'0111);
    REQUIRE(def->iformat == addressing_class::R_none);
    bool CSR_modified[magic_enum::enum_count<isa::pep10::CSR>()] = {true, true, false, true};
    for (int it = 0; it < magic_enum::enum_count<isa::pep10::CSR>(); it++) {
      REQUIRE(def->CSR_modified[it] == CSR_modified[it]);
    }
    REQUIRE(def->mnemonic == instruction_mnemonic::ASRX);
    REQUIRE(as_string(def->mnemonic) == "ASRX");
  }

  SECTION("ROLA") {
    const auto def = isa_def.isa.at(instruction_mnemonic::ROLA);
    REQUIRE(def->bit_pattern == 0b0001'1000);
    REQUIRE(def->iformat == addressing_class::R_none);
    bool CSR_modified[magic_enum::enum_count<isa::pep10::CSR>()] = {false, false, false, true};
    for (int it = 0; it < magic_enum::enum_count<isa::pep10::CSR>(); it++) {
      REQUIRE(def->CSR_modified[it] == CSR_modified[it]);
    }
    REQUIRE(def->mnemonic == instruction_mnemonic::ROLA);
    REQUIRE(as_string(def->mnemonic) == "ROLA");
  }SECTION("ROLX") {
    const auto def = isa_def.isa.at(instruction_mnemonic::ROLX);
    REQUIRE(def->bit_pattern == 0b0001'1001);
    REQUIRE(def->iformat == addressing_class::R_none);
    bool CSR_modified[magic_enum::enum_count<isa::pep10::CSR>()] = {false, false, false, true};
    for (int it = 0; it < magic_enum::enum_count<isa::pep10::CSR>(); it++) {
      REQUIRE(def->CSR_modified[it] == CSR_modified[it]);
    }
    REQUIRE(def->mnemonic == instruction_mnemonic::ROLX);
    REQUIRE(as_string(def->mnemonic) == "ROLX");
  }SECTION("RORA") {
    const auto def = isa_def.isa.at(instruction_mnemonic::RORA);
    REQUIRE(def->bit_pattern == 0b0001'1010);
    REQUIRE(def->iformat == addressing_class::R_none);
    bool CSR_modified[magic_enum::enum_count<isa::pep10::CSR>()] = {false, false, false, true};
    for (int it = 0; it < magic_enum::enum_count<isa::pep10::CSR>(); it++) {
      REQUIRE(def->CSR_modified[it] == CSR_modified[it]);
    }
    REQUIRE(def->mnemonic == instruction_mnemonic::RORA);
    REQUIRE(as_string(def->mnemonic) == "RORA");
  }SECTION("RORX") {
    const auto def = isa_def.isa.at(instruction_mnemonic::RORX);
    REQUIRE(def->bit_pattern == 0b0001'1011);
    REQUIRE(def->iformat == addressing_class::R_none);
    bool CSR_modified[magic_enum::enum_count<isa::pep10::CSR>()] = {false, false, false, true};
    for (int it = 0; it < magic_enum::enum_count<isa::pep10::CSR>(); it++) {
      REQUIRE(def->CSR_modified[it] == CSR_modified[it]);
    }
    REQUIRE(def->mnemonic == instruction_mnemonic::RORX);
    REQUIRE(as_string(def->mnemonic) == "RORX");
  }
}
