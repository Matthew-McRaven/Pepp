#pragma once
#include <exception>
#include <functional>
#include <map>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include "core/sim/devicetree.hpp"

class System;

struct ParsingError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class DeviceSerializer {
public:
  std::function<Device *(const nlohmann::json &, System *, Device * /*parent*/)> parser;
  std::function<void(nlohmann::json &)> prefill;
  std::function<void(nlohmann::json &, const System *, const Device * /*self*/)> serialize;
  std::string compatible;
};

// Helper which parses the common fields of Device (like compatible).
void parse_standard_fields(const nlohmann::json &node, Device::Configuration &cfg);
// Helper method which prefills the JSON object with type-correct keys for a given compatible value.
// Optional values will have non-null defaults. Required fields are null. Array fields will be empty array.
void prefill_keys(nlohmann::json &obj, std::string_view compatible);
// Parse a system description file and return the corresponding system. Throws an error if parsing fails.
// Each node is allowed a "magic" field, called "children", which is an array of objects nested inside this one.
// All other fields / params are parsed with a per-device-type parser method.
std::unique_ptr<System> parse_system(std::string_view body);

// Does not recurse! It only create the system object.
std::unique_ptr<System> create_system(const nlohmann::json &obj);

// if parent is nullptr, then system will be used as the parent. Throws parsing error if system is nullptr.
// Returns non-owning pointer. Owner is the system (and its device tree).
// Identifiies correct device parser based on the compatible field.
Device *create_device(const nlohmann::json &obj, System *sys, Device *parent);

void serialize_system(const System *sys, nlohmann::json &obj);

// Helpers to extract integer values from a JSON node.
// If the field is a string, attempt to parse that field.
// Throws ParsingError if the value is not present not convertible to an integer.
i8 as_i8(const nlohmann::json &node);
u8 as_u8(const nlohmann::json &node);
i16 as_i16(const nlohmann::json &node);
u16 as_u16(const nlohmann::json &node);
i32 as_i32(const nlohmann::json &node);
u32 as_u32(const nlohmann::json &node);
