#include "ir_attributes.hpp"

int pepp::tc::RISCVMnemonicAttribute::type() const { return TYPE; }

pepp::tc::RISCVMnemonicAttribute::RISCVMnemonicAttribute(std::string_view name, riscv::MnemonicDescriptor mn)
    : name(name), mn(mn) {}
