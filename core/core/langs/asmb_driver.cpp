#include "asmb_driver.hpp"
#include <type_traits>
#include <utility>
#include "core/compile/symbol/value.hpp"

namespace pepp::tc {

DriverResult assemble(const DriverConfig &config, const FormattingConfig &fmt, std::string source) {
  const auto disp = [&](auto &&arch_config) -> DriverResult {
    using Config = std::decay_t<decltype(arch_config)>;
    if constexpr (std::is_same_v<Config, RISCVDriverConfig>) return assemble_riscv(arch_config, fmt, std::move(source));
    else if constexpr (std::is_same_v<Config, Pep10DriverConfig>)
      return assemble_pep10(arch_config, fmt, std::move(source));
  };
  return std::visit(disp, config);
}

void inject_symdefs(std::vector<std::pair<std::string, u32>> symdefs, core::symbol::LeafTable &symtab, u8 byte_width) {
  for (const auto &[name, value] : symdefs) {
    auto s = symtab.define(name);
    auto _bits = bits::MaskedBits{.byteCount = byte_width, .bitPattern = value, .mask = bits::mask(byte_width)};
    s->value = std::make_shared<core::symbol::ConstantValue>(_bits);
    s->binding = core::symbol::Binding::Weak;
  }
}

} // namespace pepp::tc
