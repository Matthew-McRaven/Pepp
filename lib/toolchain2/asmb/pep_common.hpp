#pragma once
#include "./pep_ir.hpp"
namespace pepp::tc {
using PepIRProgram = std::vector<std::shared_ptr<tc::ir::LinearIR>>;
}
