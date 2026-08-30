#include "rv_instruction_list.hpp"

// Row i must describe static_cast<RvOp>(i)
inline constexpr bool rv_op_info_is_ordered() noexcept {
  for (std::size_t i = 0; i < riscv::RV_OP_INFO.size(); ++i)
    if (static_cast<std::size_t>(riscv::RV_OP_INFO[i].op) != i) return false;
  return true;
}
static_assert(rv_op_info_is_ordered(), "RV_OP_INFO rows must be in RvOp order");
static_assert(riscv::RV_OP_INFO.size() == static_cast<std::size_t>(RvOp::COUNT),
              "every RvOp needs a row in RV_OP_INFO");