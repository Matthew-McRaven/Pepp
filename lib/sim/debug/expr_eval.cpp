#include "expr_eval.hpp"

pepp::debug::EvaluationMode pepp::debug::mode_for_child(EvaluationMode current) {
  switch (current) {
  case EvaluationMode::UseCache: [[fallthrough]];
  case EvaluationMode::RecomputeSelf: return EvaluationMode::UseCache;
  case EvaluationMode::RecomputeTree: return EvaluationMode::RecomputeTree;
  }
}
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

