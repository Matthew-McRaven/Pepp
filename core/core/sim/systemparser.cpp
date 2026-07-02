#include "systemparser.hpp"
#include <nlohmann/json.hpp>
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"

void parse_node_ram_dense(const nlohmann::json &node, System *sys, Device *parent, ParsingContext &ctx);
void parse_node_ram_sparse(const nlohmann::json &node, System *sys, Device *parent, ParsingContext &ctx);
void parse_node_ram(const nlohmann::json &node, System *sys, Device *parent, ParsingContext &ctx);

using Parser = std::function<void(const nlohmann::json &, System *, Device *, ParsingContext &)>;
static const std::map<std::string, Parser> parsers = {
    {"ram,dense", Parser{parse_node_ram_dense}},
    {"ram,sparse", Parser{parse_node_ram_sparse}},
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
    if (as_json.contains("compatible") && !as_json["compatible"].is_null())
      cfg.compatible = as_json["compatible"].get<std::string>();
    else cfg.compatible = System::compatible;
    if (cfg.compatible != System::compatible)
      throw ParsingError("System description must have compatible: " + std::string(System::compatible));
    if (as_json.contains("basename") && !as_json["basename"].is_null())
      cfg.basename = as_json["basename"].get<std::string>();
  } catch (const nlohmann::json::type_error &e) {
    throw ParsingError("Failed to parse system description: " + std::string(e.what()));
  }
  std::unique_ptr<System> system = std::make_unique<System>(cfg);
  dispatch_children(as_json, system.get(), system.get(), context);
  return system;
}
