#pragma once
#include "pas/ast/pepp/attr_addr.hpp"
#include "pas/ast/pepp/attr_instruction.hpp"
#include <QtCore>
#include <isa/pep10.hpp>
Q_DECLARE_METATYPE(pas::ast::pepp::Instruction<isa::Pep10>);
Q_DECLARE_METATYPE(pas::ast::pepp::AddressingMode<isa::Pep10>);
