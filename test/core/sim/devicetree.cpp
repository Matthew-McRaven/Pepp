/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "core/sim/devicetree.hpp"
#include <catch.hpp>
#include <ranges>
#include "core/math/bitmanip/enums.hpp"
#include "core/sim/api/clock.hpp"
#include "core/sim/api/device.hpp"

struct DeviceWithType : public Device {
  DeviceWithType(Configuration config, Device::ID id, Type type) : Device(), _config(config), _id(id), _type(type) {}
  Type type() const override { return _type; }
  const Configuration &config() const override { return _config; }
  const Device::ID id() const override { return _id; }

private:
  Configuration _config;
  Device::ID _id;
  Type _type;
};

struct SubclassingDevice : public Device, ClockSource {
  SubclassingDevice(Configuration config, Device::ID id) : Device(), _config(config), _id(id) {}
  Type type() const override { return Type::ClockSource; }
  PulseSchedule schedule() const override { return {.period = 100, .jitter = 10, .seed = 0}; }
  const Configuration &config() const override { return _config; }
  const Device::ID id() const override { return _id; }

private:
  Configuration _config;
  Device::ID _id;
};

TEST_CASE("New devices test", "[scope:core][scope:core.sim][kind:unit][arch:*]") {
  using namespace bits;
  static const Device::Configuration desc{"test", "/test", "test-compatible"};
  SubclassingDevice dev(desc, Device::ID{0});
  auto tree = DeviceTree(&dev, nullptr);
  CHECK(tree.device == &dev);
  auto erased = tree.device;
  CHECK(any(erased->type() & Device::Type::ClockSource));
  CHECK(erased->capability<ClockSource>() == &dev);
  CHECK(erased->capability<ClockSink>() == nullptr);
}
TEST_CASE("DeviceTree", "[scope:core][scope:core.sim][kind:unit][arch:*]") {
  using namespace bits;
  static const Device::Configuration root_desc{"/", "/", "root"};
  static const Device::Configuration alpha_desc{"alpha", "/alpha", "alpha-compatible"};
  static const Device::Configuration beta_desc{"beta", "/beta", "beta-compatible"};
  static const Device::Configuration delta_desc{"delta", alpha_desc.child_name("delta"), "delta-compatible"};
  static const Device::Configuration gamma_desc{"gamma", alpha_desc.child_name("gamma"), "gamma-compatible"};
  using T = Device::Type;
  static DeviceWithType root(root_desc, Device::ID{0}, T::Root),
      alpha(alpha_desc, Device::ID{1}, T::ClockSink | T::MemoryInitiator),
      beta(beta_desc, Device::ID{2}, T::MemoryTarget), delta(delta_desc, Device::ID{4}, T::MemoryTarget),
      gamma(gamma_desc, Device::ID{3}, T::MemoryTarget);
  DeviceTree root_tree(&root, nullptr);
  auto alpha_tree = root_tree.append_child(&alpha);
  auto beta_tree = root_tree.append_child(&beta);
  auto delta_tree = alpha_tree->append_child(&delta);
  auto gamma_tree = alpha_tree->append_child(&gamma);

  SECTION("Count all elements") {
    CHECK(0 == std::distance(delta_tree->end(), delta_tree->end()));
    CHECK(1 == std::distance(gamma_tree->begin(), gamma_tree->end()));
    CHECK(5 == std::distance(root_tree.begin(), root_tree.end()));
    CHECK(3 == std::distance(alpha_tree->begin(), alpha_tree->end()));
    CHECK(1 == std::distance(beta_tree->begin(), beta_tree->end()));
    CHECK(1 == std::distance(delta_tree->begin(), delta_tree->end()));
  }
  SECTION("No-op filter") {
    auto view = root_tree | std::views::filter([](Device *dt) { return true; });
    CHECK(std::distance(view.begin(), view.end()) == 5);
  }
  SECTION("Filter on basename") {
    auto view = root_tree | std::views::filter([](Device *dt) { return dt->config().basename == "alpha"; });
    CHECK(std::distance(view.begin(), view.end()) == 1);
  }
  SECTION("Filter for ID") {
    auto view = root_tree | std::views::filter([](Device *dt) { return dt->id() == Device::ID{1}; });
    CHECK(std::distance(view.begin(), view.end()) == 1);
  }
  SECTION("Filter for type") {
    auto view = root_tree | std::views::filter([](Device *dt) { return any(dt->type() & T::MemoryTarget); });
    CHECK(std::distance(view.begin(), view.end()) == 3);
  }
}