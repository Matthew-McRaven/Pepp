#include "adevice.hpp"

std::string Device::Descriptor::child_name(std::string_view child_basename) const {
  auto fullprefix = this->fullname + (this->fullname.ends_with("/") ? "" : "/");
  fullprefix.append(child_basename);
  return fullprefix;
}
