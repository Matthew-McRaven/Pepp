#pragma once
#include <memory>
#include <string>
#include "common_types.hpp"
#include "flat/flat_map.hpp"

class Blueprint;

class BlueprintLibrary {
public:
  BlueprintLibrary();
  schematic::BlueprintID add_blueprint(const std::string &name, std::shared_ptr<Blueprint> blueprint);
  std::shared_ptr<const Blueprint> get_blueprint(schematic::BlueprintID id) const;
  std::shared_ptr<Blueprint> get_blueprint(schematic::BlueprintID id);
  std::shared_ptr<const Blueprint> get_blueprint(const std::string &name) const;
  std::shared_ptr<Blueprint> get_blueprint(const std::string &name);

private:
  u32 _next_id = 1;
  fc::vector_map<schematic::BlueprintID, std::shared_ptr<Blueprint>> _blueprints = {};
  std::unordered_map<std::string, schematic::BlueprintID> _name_to_id = {};
};

u32 add_builtin_blueprints(BlueprintLibrary &library);
