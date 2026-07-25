#include "hwdebug.hpp"
#include "core/sim/system.hpp"

static const Operation rw{.type = Operation::Type::Application, .kind = Operation::Kind::data};

void HWDebug::expose(const Named &n) {
  Device::ID target = std::visit([](const auto &x) { return x.target; }, n);
  _exposed[target].push_back(n);
}

u16 HWDebug::read(const Named &n) {

  // If NamedConstant
  if (std::holds_alternative<NamedConstant>(n)) {
    return std::get<NamedConstant>(n).value;
  } else if (std::holds_alternative<NamedLocation>(n)) {
    Device::ID id = std::visit([](const auto &x) { return x.target; }, n);
    auto dev = _sys->find_by_id(id);
    if (!dev) throw std::runtime_error("Device not found");
    auto &reg = std::get<NamedLocation>(n);
    auto target = dev->capability<Target>();
    if (!target) throw std::runtime_error("Device is not a Target");
    return target->read<u16, bits::host_is_le>(reg.offset, rw).second;
  }
  throw std::runtime_error("Unknown Named type");
}

std::optional<Named> HWDebug::find(std::string_view name) {
  for (const auto &[id, regs] : _exposed) {
    for (const auto &n : regs)
      if (std::visit([](const auto &x) { return x.name; }, n) == name) return n;
  }
  return std::nullopt;
}
