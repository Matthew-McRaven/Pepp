#include "systemparser.hpp"
#include <nlohmann/json.hpp>
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/memory/ram/sparse.hpp"
#include "core/sim/system.hpp"

void parse_standard_fields(const nlohmann::json &node, Device::Configuration &cfg) {
  if (node.contains("compatible") && !node["compatible"].is_null())
    cfg.compatible = node["compatible"].get<std::string>();
  if (node.contains("basename") && !node["basename"].is_null()) cfg.basename = node["basename"].get<std::string>();
}
void parse_node_ram_dense(const nlohmann::json &node, System *sys, Device *parent, ParsingContext &ctx);
void parse_node_ram_sparse(const nlohmann::json &node, System *sys, Device *parent, ParsingContext &ctx);
void parse_node_ram(const nlohmann::json &node, System *sys, Device *parent, ParsingContext &ctx);

using Parser = std::function<void(const nlohmann::json &, System *, Device *, ParsingContext &)>;
static const std::map<std::string, Parser> parsers = {
    {Dense::compatible, Parser{parse_node_ram_dense}},
    {Sparse::compatible, Parser{parse_node_ram_sparse}},
    {"ram", Parser{parse_node_ram}},
};
void dispatch_parser(const nlohmann::json &node, System *sys, Device *parent, ParsingContext &ctx) {
  auto compatible = node["compatible"].get<std::string>();
  auto it = parsers.find(compatible);
  if (it == parsers.end()) throw ParsingError("Unknown compatible type: " + compatible);
  it->second(node, sys, parent, ctx);
}
void dispatch_children(const nlohmann::json &node, System *sys, Device *parent, ParsingContext &ctx) {
  if (!node.contains("children")) return;
  auto children = node["children"];
  if (!children.is_array()) throw ParsingError("Children must be an array");
  for (const auto &child : children) dispatch_parser(child, sys, parent, ctx);
}

void parse_node_ram_dense(const nlohmann::json &self, System *sys, Device *parent, ParsingContext &ctx) {
  Dense::Configuration cfg;
  try {
    parse_standard_fields(self, cfg);
    if (cfg.basename->empty()) throw ParsingError("RAM must have a basename");
    if (!self.contains("min_offset") || self["min_offset"].is_null()) throw ParsingError("RAM must have a min_offset");
    auto min = self["min_offset"].get<u32>();
    if (!self.contains("max_offset") || self["max_offset"].is_null()) throw ParsingError("RAM must have a max_offset");
    auto max = self["max_offset"].get<u32>();
    cfg.span = AddressSpan{min, max};
    if (self.contains("fill") && !self["fill"].is_null()) cfg.fill = self["fill"].get<u8>();
  } catch (const nlohmann::json::type_error &e) {
    throw ParsingError("Failed to parse dense RAM: " + std::string(e.what()));
  }

  auto dense = sys->make_device<Dense>(parent, cfg);
  dispatch_children(self, sys, dense, ctx);
}
void parse_node_ram_sparse(const nlohmann::json &self, System *sys, Device *parent, ParsingContext &ctx) {
  dispatch_children(self, sys, parent, ctx);
}
void parse_node_ram(const nlohmann::json &self, System *sys, Device *parent, ParsingContext &ctx) {
  if (false) parse_node_ram_dense(self, sys, parent, ctx);
  else parse_node_ram_sparse(self, sys, parent, ctx);
}

std::unique_ptr<System> parse_system(std::string_view body, ParsingContext &context) {
  nlohmann::json as_json;
  try {
    as_json = nlohmann::json::parse(body);
  } catch (const nlohmann::json::parse_error &e) {
    throw ParsingError("Failed to parse system description: " + std::string(e.what()));
  }
  if (!as_json.is_object()) throw ParsingError("System description must be a JSON object");
  System::Configuration cfg;
  try {
    parse_standard_fields(as_json, cfg);
    if (cfg.compatible->empty()) cfg.compatible = System::compatible;
    else if (cfg.compatible != System::compatible)
      throw ParsingError("System description must have compatible: " + std::string(System::compatible));
    if (cfg.basename->empty()) cfg.basename = "/";
  } catch (const nlohmann::json::type_error &e) {
    throw ParsingError("Failed to parse system description: " + std::string(e.what()));
  }
  std::unique_ptr<System> system = std::make_unique<System>(cfg);
  dispatch_children(as_json, system.get(), system.get(), context);
  return system;
}
