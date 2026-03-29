#include "attr_identifier.hpp"

int pepp::tc::Identifier::type() const { return TYPE; }

pepp::tc::Identifier::Identifier(std::string v) : value(v) {}

std::string_view pepp::tc::Identifier::view() const { return value; }
