#pragma once
#include <memory>
#include <set>
#include <string>
#include "common_types.hpp"
#include "flat/flat_map.hpp"

class Blueprint;

struct BlueprintGroup {
  std::string name;
  schematic::ImageFileKey image;
  std::set<schematic::BlueprintID> members;
};

class BlueprintLibrary {
public:
  BlueprintLibrary();
  schematic::BlueprintID add_blueprint(const std::string &name, std::shared_ptr<Blueprint> blueprint);
  schematic::BlueprintGroupID add_group(std::shared_ptr<BlueprintGroup> group);
  void add_blueprint_to_group(schematic::BlueprintGroupID group_id, schematic::BlueprintID blueprint_id);
  std::shared_ptr<const Blueprint> get_blueprint(schematic::BlueprintID id) const;
  std::shared_ptr<Blueprint> get_blueprint(schematic::BlueprintID id);
  std::shared_ptr<const Blueprint> get_blueprint(const std::string &name) const;
  std::shared_ptr<Blueprint> get_blueprint(const std::string &name);

private:
  u32 _next_id = 1, _next_group = 1;
  fc::vector_map<schematic::BlueprintID, std::shared_ptr<Blueprint>> _blueprints = {};
  std::unordered_map<std::string, schematic::BlueprintID> _name_to_id = {};
  fc::vector_map<schematic::BlueprintGroupID, std::shared_ptr<BlueprintGroup>> _groups;
  std::unordered_multimap<std::string, schematic::BlueprintGroupID> _name_to_group = {};
};

