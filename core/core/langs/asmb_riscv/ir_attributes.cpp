#include "ir_attributes.hpp"

int pepp::tc::RISCVMnemonicAttribute::type() const { return TYPE; }

pepp::tc::RISCVMnemonicAttribute::RISCVMnemonicAttribute(riscv::MnemonicDescriptor mn) : mn(mn) {}
