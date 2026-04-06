#include "circuitschematic.hpp"

CircuitSchematic::CircuitSchematic() {}

std::shared_ptr<Component> CircuitSchematic::component(schematic::ComponentID id) {
  if (auto it = _components.find(id); it == _components.end()) return nullptr;
  else return it->second;
}

const std::shared_ptr<Component> CircuitSchematic::component(schematic::ComponentID id) const {
  if (auto it = _components.find(id); it == _components.end()) return nullptr;
  else return it->second;
}

bool CircuitSchematic::empty() const { return _components.empty(); }

std::optional<schematic::ComponentID> CircuitSchematic::component_at(schematic::Point location) const {
  if (const auto id = _floorplan.at(location); id) return schematic::ComponentID{id.value()};
  else return std::nullopt;
}

bool CircuitSchematic::can_move_component(schematic::ComponentID id, schematic::Point location) const {
  if (const auto &comp = component(id); comp == nullptr) return false;
  else return _floorplan.can_move_absolute(id.value, location);
}

bool CircuitSchematic::move_component(schematic::ComponentID id, schematic::Point location) {
  // Only components can be moved by this method; nets are ignored.
  if (auto comp = component(id); comp == nullptr) return false;
  else if (!can_move_component(id, location)) return false;
  else if (_floorplan.move_absolute(id.value, location)) {
    comp->set_position(location);
    return true;
  } else return false;
}

bool CircuitSchematic::can_rotate_component(schematic::ComponentID id, Direction dir) const {
  using D = Direction;
  if (const auto &comp = component(id); comp == nullptr) return false;
  // Reject no-op rotation
  else if (const auto cd = comp->direction(); cd == dir) return false;
  // Rotating by 180 degrees does not change footprint.
  else if (parallel(cd, dir)) return true;
  // Else rotation is perpenedicular and requires a transpose in floorplan.
  else return _floorplan.can_move_relative(id.value, {0, 0}, true);
}

bool CircuitSchematic::rotate_component(schematic::ComponentID id, Direction dir) {
  using D = Direction;
  if (const auto &comp = component(id); comp == nullptr) return false;
  else if (!can_rotate_component(id, dir)) return false;
  else if (const auto cd = comp->direction();
           parallel(cd, dir)) { // Rotate pins by 180 degress without updating floorplan
    comp->set_direction(dir);
    return true;
  } else { // Direction is perpendicular, so both pin oritentation and floorplan need updates.
    comp->set_direction(dir);
    return _floorplan.can_move_relative(id.value, {0, 0}, true);
  }
}

std::optional<schematic::ComponentID> CircuitSchematic::place_component(std::shared_ptr<Blueprint> blueprint,
                                                                        schematic::Point location, Direction dir) {
  std::shared_ptr<Component> comp = std::make_shared<Component>(blueprint, location, dir);
  auto maybe_id = _floorplan.try_add(comp->geometry());
  if (maybe_id) {
    schematic::ComponentID id{maybe_id.value()};
    // Update Component's ID (which is initially invalid) with value returned by floorplan before inserting into map.
    comp->set_id(id);
    _components.emplace(id, comp);
    return id;
  }
  return std::nullopt;
}

bool CircuitSchematic::remove_component(schematic::ComponentID id) {
  if (auto it = _components.find(id); it == _components.end()) return false;
  _floorplan.remove(id.value);
  _components.erase(id);
  // Iterate over all connections and remove any connections to/from this component.
  // Adding/removing components should be rare, so can afford the O(N) scan.
  connections().erase(std::remove_if(connections().begin(), connections().end(),
                                     [id](const Connection &conn) {
                                       return conn.src.component_id == id || conn.dst.component_id == id;
                                     }),
                      connections().end());
  return true;
}

bool CircuitSchematic::has_pin(schematic::GlobalPinID pin_id) const {
  const auto comp = component(pin_id.component_id);
  return comp != nullptr && comp->pin_count() > pin_id.local_pin_id.value;
}

schematic::Rectangle CircuitSchematic::pin_geometry(schematic::GlobalPinID pin_id) const {
  const auto comp = component(pin_id.component_id);
  if (comp == nullptr) throw std::runtime_error("Component does not exist");
  else if (comp->pin_count() <= pin_id.local_pin_id.value) throw std::runtime_error("Pin does not exist");
  const auto &pin = comp->pin(pin_id.local_pin_id.value);
  return pin.geometry;
}

bool CircuitSchematic::add_connection(schematic::GlobalPinID src, schematic::GlobalPinID dst) {
  // TODO: Check that src is an output
  // TODO: Check that dst is an input or a clock.
  // TODO: Check that dst is not already driven by another connection
  // TODO: return a status code rather than true/false. An output pin driven by multiple inputs is valid to draw, just
  // not simulate.
  Connection conn{src, dst};
  if (!has_pin(src)) {
    std::cerr << "Source pin does not exist";
    return false;
  } else if (!has_pin(dst)) {
    std::cerr << "Destination pin does not exist";
    return false;
  }
  const auto &c = connections();
  auto it = std::find(c.cbegin(), c.cend(), conn);
  if (it != c.cend()) {
    std::cerr << "Connection already exists";
    return false;
  } else {
    connections().push_back(conn);
    return true;
  }
}
