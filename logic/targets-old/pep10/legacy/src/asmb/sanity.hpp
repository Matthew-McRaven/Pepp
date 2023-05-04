#pragma once
#include "asmdr/project/project.hpp"
#include "ir/args.hpp"
#include "ir/directives.hpp"
#include "ir/macro.hpp"
#include "symbol/value.hpp"
//#include "masm/ir.macro.hpp"

template <typename address_size_t>
bool whole_program_sanity_fixup(std::shared_ptr<masm::project::target<uint16_t>> &target,
                                std::shared_ptr<ir::section::code_section<uint16_t>> &section);

#include "sanity.tpp"