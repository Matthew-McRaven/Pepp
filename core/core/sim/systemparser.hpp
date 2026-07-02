#pragma once
#include <map>
#include <string>
#include "core/sim/api/configs.hpp"
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
