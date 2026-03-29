#include "attr_argument.hpp"

int pepp::tc::Argument::type() const { return TYPE; }

pepp::tc::Argument::Argument(std::shared_ptr<ast::IRValue> value) : value(std::move(value)) {}
