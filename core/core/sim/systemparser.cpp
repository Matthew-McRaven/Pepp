#include "systemparser.hpp"
#include <nlohmann/json.hpp>
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/memory/ram/sparse.hpp"
#include "core/sim/system.hpp"

template <std::integral T> T as_int(const nlohmann::json &value) {
  using l = std::numeric_limits<T>;
  if constexpr (std::is_unsigned_v<T>) {
    if (value.is_number_unsigned()) {
      u64 v = value.get<u64>();
      if (v > l::max()) throw ParsingError("Unsigned integer field too large");
      return static_cast<T>(v);
    } else if (value.is_number_integer()) {
      i64 v = value.get<i64>();
      if (v < 0) throw ParsingError("Unsigned integer field cannot be negative");
      return static_cast<T>(v);
    }
  } else {
    if (value.is_number_integer()) {
      i64 v = value.get<i64>();
      if (v < l::min() || v > l::max()) throw ParsingError("Signed integer field out of range");
      return static_cast<T>(v);
    } else if (value.is_number_unsigned()) {
      u64 v = value.get<u64>();
      if (v > static_cast<u64>(l::max())) throw ParsingError("Signed integer field out of range");
      return static_cast<T>(v);
    }
  }
  // If we've made it to here, it's a string or not an integer
  if (!value.is_string()) throw ParsingError("Field must be an integer or string containing an integer");
  auto str = value.get<std::string>();
  int base = 10, prefix_size = 0, sign = 1;
  // Extract base from prefix, and handle explict signs for decimals.
  if (str.starts_with("0b") || str.starts_with("0B")) base = 2, prefix_size = 2;
  else if (str.starts_with("0o") || str.starts_with("0O")) base = 8, prefix_size = 2;
  else if (str.starts_with("0x") || str.starts_with("0X")) base = 16, prefix_size = 2;
  else if (str.starts_with("-")) prefix_size = 1, sign = -1;
  else if (str.starts_with("+")) prefix_size = 1;
  // Throw erorr if - and unsigned type.
  if (std::is_unsigned_v<T> && sign < 0) throw ParsingError("Unsigned integer field cannot be negative");

  auto sub = str.substr(prefix_size);
  T ret;
  const auto* sub_end = sub.data() + sub.size();
  auto [ptr, ec] = std::from_chars(sub.data(), sub_end, ret, base);
  if (ec != std::errc() || ptr != sub_end) throw ParsingError("Failed to parse integer from string");
  return static_cast<T>(sign) * ret;
}
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
    auto min = as_int<u32>(self["min_offset"]);
    if (!self.contains("max_offset") || self["max_offset"].is_null()) throw ParsingError("RAM must have a max_offset");
    auto max = as_int<u32>(self["max_offset"]);
    cfg.span = AddressSpan{min, max};
    if (self.contains("fill") && !self["fill"].is_null()) cfg.fill = as_int<u8>(self["fill"]);
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
