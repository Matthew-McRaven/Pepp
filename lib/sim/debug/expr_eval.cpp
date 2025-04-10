#include "expr_eval.hpp"

bool pepp::debug::is_unsigned(ExpressionType t) {
  switch (t) {
  case pepp::debug::ExpressionType::i8: [[fallthrough]];
  case pepp::debug::ExpressionType::i16: [[fallthrough]];
  case pepp::debug::ExpressionType::i32: return false;
  case pepp::debug::ExpressionType::u8: [[fallthrough]];
  case pepp::debug::ExpressionType::u16: [[fallthrough]];
  case pepp::debug::ExpressionType::u32: return true;
  }
}

