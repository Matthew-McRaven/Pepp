#include "asmb_driver.hpp"
#include <type_traits>
#include <utility>

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

} // namespace pepp::tc
