#pragma once
#include <concepts>
#include <optional>
#include <string>

/*
 * With our 2-phase parser, configurations have 3 kinds of values:
 * - Immediate, which the parser is able to resolve in the first phase directly to a value.
 *   Device constructors may use these values at any point in their lifetime.
 * - Deferred, which cannot be resolved until after the whole device tree has been parsed.
 *   This is usually for cross-device links, e.g., the name of the memory module that the CPU connects to.
 *   These values should not be used by the device until initialize() is called, at which point the Device
 *   is likely to store the resolved value internally.
 * - Computed values are those generated "for free" during the parsing process, like a Device::ID and a full name.
 *   Comments on the fields will indicate in which phase they are computed.
 *   If serializing a config, compute fields should be dropped, as they will be re-computed on the next parse.
 *
 * We don't have a mechanism to enforce this policy, but my hope is that encoding this info in the type system rather
 * than variable names and comments can be a first-step towards preventing missuse.
 *
 */

enum class ConfigType { Immediate, Deferred, Computed };
template <ConfigType t, typename T> class ConfigField {
public:
  template <typename A> ConfigField(A &&value) : _value(value) {}
  // default copy/move/assignment
  ConfigField() = default;
  ConfigField(const ConfigField &other) = default;
  ConfigField(ConfigField &&other) noexcept = default;
  // copy-assign
  ConfigField &operator=(const ConfigField &other) = default;
  ConfigField &operator=(ConfigField &&other) noexcept = default;
  template <typename A> ConfigField &operator=(A &&value) {
    _value = value;
    return *this;
  }

  T operator*() const { return _value; }

private:
  T _value;
};
template <typename T> using Immediate = ConfigField<ConfigType::Immediate, T>;
template <typename T> using Deferred = ConfigField<ConfigType::Deferred, T>;
template <typename T> using Computed = ConfigField<ConfigType::Computed, T>;
