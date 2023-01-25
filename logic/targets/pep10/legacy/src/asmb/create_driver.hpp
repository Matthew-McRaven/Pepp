#pragma once

#include "asmdr/backend/assign_addr.hpp"
#include "asmdr/driver.hpp"
#include "asmdr/elf/pack.hpp"
#include "asmdr/frontend/tokenizer.hpp"
#include "asmdr/frontend/tokens.hpp"
#include "asmdr/project/init_project.hpp"
#include "asmdr/project/project.hpp"
#include "ir.hpp"
#include "ir/directives.hpp"
#include "macro/registry.hpp"
#include "parser.hpp"
#include "tokenizer.hpp"

namespace asmb::pep10::driver {
using tokenizer_t = asmb::pep10::tokenizer<masm::frontend::lexer_t>;
using stage_t = masm::project::toolchain_stage;
using driver_t = masm::driver<uint16_t, stage_t>;
using target_t = driver_t::target_t;
using group_t = driver_t::group_t;
using section_t = driver_t::section_t;
using result_t = driver_t::result_t;
using transform_t = driver_t::transform_t;
std::shared_ptr<driver_t> make_driver();

std::tuple<bool, group_t> assemble(std::string user_text, std::string os_text);
}; // namespace asmb::pep10::driver
