#pragma once

#include <functional>
#include <string>
#include "core/ds/opaque_handle.hpp"
#include "core/integers.h"

struct Device {
  using ID = pepp::OpaqueHandle<struct DeviceID, u8>;
  using IDGenerator = std::function<Device::ID()>;
  struct Descriptor {
    ID id = Device::ID{0};
    std::string basename = "", fullname = "", compatible = "";
    std::string child_name(std::string_view child_basename) const;
  };

  Device(Descriptor desc) : _desc(desc) {}
  virtual ~Device() = default;
  const Descriptor &descriptor() const { return _desc; }

private:
  Descriptor _desc;
};

struct DeviceTree {
  DeviceTree(Device *device, DeviceTree *parent) : device(device), parent(parent), owned(nullptr) {}
  DeviceTree(std::unique_ptr<Device> device, DeviceTree *parent)
      : device(device.get()), parent(parent), owned(std::move(device)) {}
  // Non-owning pointer. If owned is not-null, both point to the same object.
  Device *device;
  // Non-owning pointer to parent tree element. If nullptr, this is the root node.
  DeviceTree *parent;
  std::vector<std::unique_ptr<DeviceTree>> children;
  template <bool Const> struct Iterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Device *;
    using reference = std::conditional_t<Const, Device *const &, Device *&>;
    using pointer = std::conditional_t<Const, Device *const *, Device **>;
    using node_ptr = std::conditional_t<Const, const DeviceTree *, DeviceTree *>;
    Iterator() = default;
    // Use this overload when root==cur
    explicit Iterator(node_ptr node) : _root(node), _cur(node) {}
    // Use this overload when root!=cur, (e.g., cur is nullptr, but root must still be set to limit recursion)
    explicit Iterator(node_ptr node, node_ptr root) : _root(root), _cur(node) {}
    value_type operator*() const;
    Iterator &operator++();
    Iterator operator++(int);
    template <bool Other> bool operator==(const Iterator<Other> &o) const;
    template <bool Other> bool operator!=(const Iterator<Other> &o) const;

  private:
    static node_ptr preorder(const DeviceTree *root, DeviceTree *current);
    // Store the root so that we can avoid ascending above our starting level.
    node_ptr _root = nullptr, _cur = nullptr;
  };

  auto begin() { return Iterator<false>(this); }
  auto end() { return Iterator<false>(nullptr, this); }
  auto begin() const { return Iterator<true>(this); }
  auto end() const { return Iterator<true>(nullptr, this); }
  auto cbegin() const { return Iterator<true>(this); }
  auto cend() const { return Iterator<true>(nullptr, this); }

  DeviceTree *append_child(Device *dev);

private:
  // If present, this device owns device and needs to clean it up on destruction.
  // It exists to accomodate the fact that System is a device, but System owns the DeviceTree root.
  std::unique_ptr<Device> owned;
};

template <bool Const> DeviceTree::Iterator<Const>::value_type DeviceTree::Iterator<Const>::operator*() const {
  return _cur->device;
}
template <bool Const> DeviceTree::Iterator<Const> &DeviceTree::Iterator<Const>::operator++() {
  _cur = preorder(_root, _cur);
  return *this;
}
template <bool Const> DeviceTree::Iterator<Const> DeviceTree::Iterator<Const>::operator++(int) {
  auto t = *this;
  ++*this;
  return t;
}

template <bool Const>
template <bool Other>
bool DeviceTree::Iterator<Const>::operator==(const Iterator<Other> &o) const {
  return _root == o._root && _cur == o._cur;
}

template <bool Const>
template <bool Other>
bool DeviceTree::Iterator<Const>::operator!=(const Iterator<Other> &o) const {
  return !(*this == o);
}

// Roughly scales O(N*load_factor), where load factor is the average number of children per node.
// Scanning could be improved/cached, but I opted to reduce memory footprint of the iterator at the cost of iteration
// time. Querying the tree should be a rare occurence.
template <bool Const>
DeviceTree::Iterator<Const>::node_ptr DeviceTree::Iterator<Const>::preorder(const DeviceTree *root,
                                                                            DeviceTree *current) {
  if (!current) return nullptr;
  using node = DeviceTree::Iterator<Const>::node_ptr;
  // First child if present, otherwise next sibling, which requires climbing into parent
  if (!current->children.empty()) return current->children.front().get();
  // Otherwise climb until we find an unvisited next sibling.
  while (current != root && current->parent) {
    // Find the index of the current node in the parent, and return it's right sibling.
    node p = current->parent;
    auto &sibs = p->children;
    for (size_t i = 0; i < sibs.size(); ++i) {
      if (sibs[i].get() == current) {
        if (i + 1 < sibs.size()) return sibs[i + 1].get();
        else break;
      }
    }
    current = p; // no next sibling here; climb.
  }
  return nullptr; // Root has no next siblings.
}

// Ensure that
