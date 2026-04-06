#pragma once

#include <memory>
#include <optional>
#include <unordered_map>
#include "common_types.hpp"

class CircuitSchematic;
class BlueprintLibrary;

class CircuitProject {
public:
  CircuitProject();
  //  Returns nullptr if absent.
  schematic::ImageFileKey track_file(const std::string &file_path);
  std::optional<const std::string *> find_file(schematic::ImageFileKey key) const;
  std::optional<schematic::ImageFileKey> find_file(const std::string &file_path) const;
  bool contains_file(schematic::ImageFileKey key) const;
  bool contains_file(const std::string &file_path) const;
  std::size_t size() const;

  u32 add_builtin_blueprints();
  void add_test_data();

  inline std::shared_ptr<CircuitSchematic> schematic() const { return _schematic; }

private:
  std::shared_ptr<CircuitSchematic> _schematic;
  std::shared_ptr<BlueprintLibrary> _library;

  // Members used to track files used by the project.
  // Needed to serialize custom images.
  std::unordered_map<schematic::ImageFileKey, std::string> _files;
  std::unordered_map<std::string, schematic::ImageFileKey> _inverse_files;
  u32 _next_file_index = 1;
};
