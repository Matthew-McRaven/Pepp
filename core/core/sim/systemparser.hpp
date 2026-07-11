#pragma once
#include <map>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include "core/sim/devicetree.hpp"

class System;

struct ParsingContext {
  std::map<std::string, std::string> substitutions;
};

struct ParsingError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};
// Parse a system description file and return the corresponding system. Throws an error if parsing fails.
// Each node is allowed a "magic" field, called "children", which is an array of objects nested inside this one.
// All other fields / params are parsed with a per-device-type parser method.
std::unique_ptr<System> parse_system(std::string_view body, ParsingContext &context);

// Does not recurse! It only create the system.
std::unique_ptr<System> create_system(const nlohmann::json &obj, ParsingContext &ctx);

// if parent is nullptr, then system will be used as the parent. Throws parsing error if system is nullptr.
// Returns non-owning pointer. Owner is the system (and its device tree).
Device *parse_device(std::string_view body, ParsingContext &ctx, System *sys, Device *parent);

Device *create_device(const nlohmann::json &obj, ParsingContext &ctx, System *sys, Device *parent);

void prefill_keys(nlohmann::json &obj, std::string_view compatible);