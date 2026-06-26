#include "adevice.hpp"

std::string Device::Descriptor::child_name(std::string_view child_basename) const {
  auto fullprefix = this->fullname + (this->fullname.ends_with("/") ? "" : "/");
  fullprefix.append(child_basename);
  return fullprefix;
}

DeviceTree *DeviceTree::append_child(Device *dev) {
  auto ptr = std::make_unique<DeviceTree>(dev, this);
  auto ret = ptr.get();
  children.emplace_back(std::move(ptr));
  return ret;
}

// I want to use  std::views::filter on a device tree, which requires fulfulling the following concepts.
// Some concepts are duplicates, but
static_assert(std::input_or_output_iterator<DeviceTree::Iterator<true>>);
static_assert(std::input_iterator<DeviceTree::Iterator<true>>);
static_assert(std::indirectly_readable<DeviceTree::Iterator<true>>);
static_assert(std::input_iterator<DeviceTree::Iterator<true>>);
static_assert(std::sentinel_for<DeviceTree::Iterator<true>, DeviceTree::Iterator<true>>);
static_assert(std::input_or_output_iterator<DeviceTree::Iterator<false>>);
static_assert(std::input_iterator<DeviceTree::Iterator<false>>);
static_assert(std::indirectly_readable<DeviceTree::Iterator<false>>);
static_assert(std::input_iterator<DeviceTree::Iterator<false>>);
static_assert(std::sentinel_for<DeviceTree::Iterator<false>, DeviceTree::Iterator<false>>);
static_assert(std::ranges::range<DeviceTree>);
