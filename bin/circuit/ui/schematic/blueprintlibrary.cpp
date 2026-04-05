#include "blueprintlibrary.hpp"
#include "schematic/blueprint.hpp"

BlueprintLibrary::BlueprintLibrary() {}

schematic::BlueprintID BlueprintLibrary::add_blueprint(const std::string &name, std::shared_ptr<Blueprint> blueprint) {
  if (_name_to_id.contains(name)) throw std::runtime_error("Blueprint with name '" + name + "' already exists");
  auto id = schematic::BlueprintID(_next_id++);
  _blueprints[id] = blueprint;
  _name_to_id[name] = id;
  return id;
}

std::shared_ptr<Blueprint> BlueprintLibrary::get_blueprint(schematic::BlueprintID id) {
  auto it = _blueprints.find(id);
  if (it == _blueprints.end()) return nullptr;
  return it->second;
}

std::shared_ptr<const Blueprint> BlueprintLibrary::get_blueprint(schematic::BlueprintID id) const {
  const auto it = _blueprints.find(id);
  if (it == _blueprints.end()) return nullptr;
  return it->second;
}

std::shared_ptr<Blueprint> BlueprintLibrary::get_blueprint(const std::string &name) {
  if (const auto it = _name_to_id.find(name); it == _name_to_id.cend()) return nullptr;
  else return get_blueprint(it->second);
}

std::shared_ptr<const Blueprint> BlueprintLibrary::get_blueprint(const std::string &name) const {
  if (const auto it = _name_to_id.find(name); it == _name_to_id.cend()) return nullptr;
  else return get_blueprint(it->second);
}

namespace {
u32 add_and_blueprints(BlueprintLibrary &library) {
  auto and2 = std::make_shared<BuiltinBlueprint>();
  and2->type = BuiltinBlueprint::Type::AND;
  and2->size = schematic::Size{2, 3};
  and2->pins = {
      Blueprint::Pin{schematic::Rectangle::from_point_size(2, 0, 1, 3), PinType::Output},
      Blueprint::Pin{schematic::Rectangle::from_point_size(0, 0, 1, 1), PinType::Input},
      Blueprint::Pin{schematic::Rectangle::from_point_size(0, 2, 1, 1), PinType::Input},
  };
  library.add_blueprint("AND2", and2);

  return 1;
}
} // namespace
u32 add_builtin_blueprints(BlueprintLibrary &library) {
  u32 ret = 0;
  ret += add_and_blueprints(library);
  return ret;
}
