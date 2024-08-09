/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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
#pragma once
#include <QtCore>
#include <algorithm>
#include <vector>

/*
 * This class assigns a unique identifier to each data path. Embedding that identifier in a trace packet allows us to
 * reconstruct the entire dataflow without tracing all intermediate devices.
 *
 * In a system, a transaction may cross multiple device to reach its destination. For example, the SimpleBus transforms
 * an address in the bus space to an address in a child device's space. Recovering that path (e.g., finding the
 * initiator and address it accessed) is generally impossible without auxillary data. Consider the following topology:
 * // clang-format off
 * SimpleBus A   SimpleBus B
 *         \        /
 *          Device D
 * // clang-format on
 * Since A & B's address translation remains constant during simulation, we ought to avoid tracing them.
 * If tracing only D and given a trace packet one cannot determine the initiator nor the address that was sent to the
 * bus. On a real bus, the device would have an address in the bus space and would perform any address translation on
 * the internal side of the bus interface, but the above logic still stands.
 *
 * This issue is relevant to minimizing the set of addresses we update in the memory dump pane after a simulation step.
 * Only devices like RAM or MM IO ports are traced, and traces are addressed relative to the interal address space of
 * the traced device. We must reconstruct the path the transaction took from the CPU to the device to identify the
 * correct address to re-render.
 */
namespace sim::api2 {
class Paths {
  // Must declare prior to first usage of <.
  // Helper struct that lets me store a pair while only sorting on one value.
  // This avoids additional indirection from a std::set (which is usually a balanced tree).
  // We ensure that for each device, each parent_context appears in at most one node.
  struct H {
    quint16 previous_step = EMPTY;
    quint16 step_index = 0; // Index into _steps containing {EMPTY, EMPTY, 0}.
    // Ignore nodeIndex as there will never be duplicate parents.
    auto operator<=>(const H &other) const { return previous_step <=> other.previous_step; }
  };

public:
  // Helper which holds a step. It is a home-rolled linked-list backed by a vector.
  struct Step {
    // Pointers to nodes stored as indices into _steps.
    // I don't want to waste 4-12 bytes for pointers.
    quint16 previous_step = EMPTY, current_step = EMPTY;
    // Device which is traversed during this step.
    quint16 device = 0;
  };

  static const quint16 EMPTY = 0;
  quint16 add(quint16 parent, quint16 device) {
    // Prevent adding a path that exists.
    if (auto tmp = find(parent, device); tmp != EMPTY)
      return tmp;
    // Use the current size as a unique identifier.
    quint16 current = _steps.size();

    _steps.emplace_back(Step{parent, current, device});
    // Ensure we don't perform OOB access.
    if (_device_to_paths.size() <= device)
      _device_to_paths.resize(device + 1, {});

    auto &cont = _device_to_paths[device];
    cont.emplace_back(H{parent, current});
    std::sort(cont.begin(), cont.end());
    return current;
  }

  quint16 find(quint16 parent, quint16 device) const {
    if (device >= _device_to_paths.size())
      return 0;
    auto paths = _device_to_paths[device];
    auto it = std::lower_bound(paths.cbegin(), paths.cend(), H{parent, 0});
    if (it != paths.cend())
      return _steps[it->step_index].current_step;
    return EMPTY;
  }

  // Allows chasing a path to the root.
  const Step &operator[](quint16 step_index) const { return _steps.at(step_index); }

  void clear() {
    _steps.clear();
    _device_to_paths.clear();
    _steps.emplace_back(Step{EMPTY, EMPTY, 0});
    _device_to_paths.emplace_back(std::vector<H>({H{EMPTY, 0}}));
  }

private:
  std::vector<Step> _steps = {Step{EMPTY, EMPTY, 0}};
  // Index into outer vector is device ID. Inner vector is paths that traverse that device ID.
  std::vector<std::vector<H>> _device_to_paths = {{{EMPTY, 0}}};
};
} // namespace sim::api2
