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

schematic::BlueprintGroupID BlueprintLibrary::add_group(std::shared_ptr<BlueprintGroup> group) {
  auto id = schematic::BlueprintGroupID(_next_group++);
  _groups[id] = group;
  _name_to_group.insert({group->name, id});
  return id;
}

void BlueprintLibrary::add_blueprint_to_group(schematic::BlueprintGroupID group_id,
                                              schematic::BlueprintID blueprint_id) {
  if (_groups.find(group_id) == _groups.end())
    throw std::runtime_error("Blueprint group with id '" + std::to_string(group_id.value) + "' does not exist");
  else if (_blueprints.find(blueprint_id) == _blueprints.end())
    throw std::runtime_error("Blueprint with id '" + std::to_string(blueprint_id.value) + "' does not exist");
  _groups[group_id]->members.insert(blueprint_id);
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

