#include "systemparser.hpp"
#include <nlohmann/json.hpp>
#include "core/ds/string_compare.hpp"
#include "core/sim/memory/bus/simplebus.hpp"
#include "core/sim/memory/io/mm.hpp"
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

i8 as_i8(const nlohmann::json &node) { return as_int<i8>(node); }

u8 as_u8(const nlohmann::json &node) { return as_int<u8>(node); }

i16 as_i16(const nlohmann::json &node) { return as_int<i16>(node); }

u16 as_u16(const nlohmann::json &node) { return as_int<u16>(node); }

i32 as_i32(const nlohmann::json &node) { return as_int<i32>(node); }

u32 as_u32(const nlohmann::json &node) { return as_int<u32>(node); }

void parse_standard_fields(const nlohmann::json &node, Device::Configuration &cfg) {
  if (node.contains("compatible") && !node["compatible"].is_null())
    cfg.compatible = node["compatible"].get<std::string>();
  if (node.contains("basename") && !node["basename"].is_null()) cfg.basename = node["basename"].get<std::string>();
}

// Helper which can choose between dense and sparse RAM based on the provided properties.
// e.g., choose dense when size is small and sparse when large.
Device *parse_node_ram(const nlohmann::json &node, System *sys, Device *parent);
void prefill_node_ram(nlohmann::json &obj);
std::unique_ptr<DeviceSerializer> make_serializer_ram() {
  DeviceSerializer s{
      .parser = parse_node_ram,
      .prefill = prefill_node_ram,
      .serialize = nullptr,
      .compatible = "ram",
  };
  return std::make_unique<DeviceSerializer>(std::move(s));
}

// Use immediately-invoked function expression to avoid the fact that intiailizer-list elements are copied, which breaks
// with unique_ptr.
static const std::unordered_map<std::string, std::unique_ptr<DeviceSerializer>, pepp::bts::cs_hash, pepp::bts::cs_eq>
    parsers = [] {
      std::unordered_map<std::string, std::unique_ptr<DeviceSerializer>, pepp::bts::cs_hash, pepp::bts::cs_eq> m;
      m.emplace(Dense::compatible, Dense::make_serializer());
      m.emplace(Sparse::compatible, Sparse::make_serializer());
      m.emplace("ram", make_serializer_ram());
      m.emplace(SimpleBus::compatible, SimpleBus::make_serializer());
      m.emplace(MemoryMappedRegister::compatible, MemoryMappedRegister::make_serializer());
      return m;
    }();

// Helpers which choose between available RAM modules depending on the configuration properties.
Device *parse_node_ram(const nlohmann::json &self, System *sys, Device *parent) {
  if (true) return parsers.find("ram,dense")->second->parser(self, sys, parent);
  else return parsers.find("ram,sparse")->second->parser(self, sys, parent);
}
void prefill_node_ram(nlohmann::json &obj) {
  obj["compatible"] = "ram";
  obj["basename"];
  obj["min_offset"];
  obj["max_offset"];
  obj["fill"] = 0;
}

// Helper to identify the right parser based on compatible value and invoke it.
Device *dispatch_parser(const nlohmann::json &node, System *sys, Device *parent) {
  auto compatible = node["compatible"].get<std::string>();
  auto it = parsers.find(compatible);
  if (it == parsers.end()) throw ParsingError("Unknown compatible type: " + compatible);
  return it->second->parser(node, sys, parent);
}

// Handle recursion into children nodes. Parse each child, then recurse into that node's children.
void dispatch_children(const nlohmann::json &node, System *sys, Device *parent) {
  if (!node.contains("children")) return;
  auto children = node["children"];
  if (!children.is_array()) throw ParsingError("Children must be an array");
  for (const auto &child : children) {
    auto dev = dispatch_parser(child, sys, parent);
    dispatch_children(child, sys, dev);
  }
}

std::unique_ptr<System> parse_system(std::string_view body) {
  nlohmann::json as_json;
  try {
    as_json = nlohmann::json::parse(body);
  } catch (const nlohmann::json::parse_error &e) {
    throw ParsingError("Failed to parse system description: " + std::string(e.what()));
  }
  if (!as_json.is_object()) throw ParsingError("System description must be a JSON object");
  auto system = create_system(as_json);
  // Handle the recursive step where all children are recursively constructed.
  dispatch_children(as_json, system.get(), system.get());
  return system;
}

Device *create_device(const nlohmann::json &obj, System *sys, Device *parent) {
  if (sys == nullptr) throw ParsingError("System must be non-null");
  if (parent == nullptr) parent = sys;
  return dispatch_parser(obj, sys, parent);
}

void prefill_keys(nlohmann::json &obj, std::string_view compatible) {
  if (auto it = parsers.find(compatible); it != parsers.end()) it->second->prefill(obj);
}

std::unique_ptr<System> create_system(const nlohmann::json &obj) {
  System::Configuration cfg;
  try {
    parse_standard_fields(obj, cfg);
    if (cfg.compatible.empty()) cfg.compatible = System::compatible;
    else if (cfg.compatible != System::compatible)
      throw ParsingError("System description must have compatible: " + std::string(System::compatible));
    if (cfg.basename.empty()) cfg.basename = "/";
  } catch (const nlohmann::json::type_error &e) {
    throw ParsingError("Failed to parse system description: " + std::string(e.what()));
  }
  return std::make_unique<System>(cfg);
}

void serialize_children(const DeviceTree *parent, const System *sys, nlohmann::json &obj) {
  nlohmann::json children = nlohmann::json::array();
  for (const auto &child : parent->children) {
    nlohmann::json child_obj;
    child->device->serializer()->serialize(child_obj, sys, child->device);
    serialize_children(child.get(), sys, child_obj);
    children.push_back(std::move(child_obj));
  }
  obj["children"] = std::move(children);
}

void serialize_system(const System *sys, nlohmann::json &obj) {
  obj["compatible"] = sys->config().compatible;
  obj["basename"] = sys->config().basename;
  const auto dt = sys->root();
  serialize_children(dt, sys, obj);
}
