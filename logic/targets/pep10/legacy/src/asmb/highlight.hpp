#pragma once

#include "api/highlight.hpp"
#include <map>
#include <string>

namespace asmb::pep10 {
std::map<asmb::highlight_type, std::string> construct_rules();
}