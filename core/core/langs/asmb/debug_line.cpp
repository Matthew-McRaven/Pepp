#include "core/langs/asmb/debug_line.hpp"
#include "core/compile/ir_linear/attr_comment.hpp"

pepp::bts::LineProgramHeader pepp::tc::detail::debug_line_header(u8 address_size, std::string_view directory,
                                                                  std::string_view file) {
  pepp::bts::LineProgramHeader ret;
  ret.address_size = address_size;
  // Instructions and data directives may be any number of bytes long.
  ret.minimum_instruction_length = 1;
  ret.maximum_operations_per_instruction = 1;
  ret.default_is_stmt = true;
  ret.line_base = -5;
  ret.line_range = 14;
  ret.standard_opcode_lengths = {0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1};
  // Section 6.2.4 item 16: entry 0 is the current directory of the compilation.
  ret.directories = {std::string(directory)};
  // Section 6.2.4 item 20 makes entry 0 the primary source file. Repeating it as entry 1, as GCC does, matches the
  // file register's initial value of 1 (Table 6.4), so rows need no DW_LNS_set_file and binutils finds file 1.
  ret.files = {{.path = std::string(file), .directory_index = 0}, {.path = std::string(file), .directory_index = 0}};
  return ret;
}

std::string pepp::tc::detail::define_string(const MacroDefinition &definition) {
  std::vector<std::string> parameters;
  for (const auto &[name, default_value] : definition.arguments) {
    // A name containing = would read back as a default.
    if (name.find('=') != name.npos) throw std::logic_error("define_string: parameter name contains =");
    parameters.push_back(default_value ? name + "=" + *default_value : name);
  }
  // TODO: avoid extra copy to vector by make format_define smarter.
  const std::vector<std::string_view> views(parameters.begin(), parameters.end());
  return pepp::bts::format_define(definition.name, views, definition.body);
}

std::expected<std::shared_ptr<pepp::tc::MacroDefinition>, std::string>
pepp::tc::detail::parse_define_string(std::string_view string) {
  const auto parsed = pepp::bts::parse_define(string);
  if (!parsed) return std::unexpected("unterminated parameter list in define string");
  // define_string always writes a parameter list.
  else if (!parsed->parameters) return std::unexpected("define string has no parameter list");
  auto ret = std::make_shared<MacroDefinition>();
  ret->name = parsed->name;
  for (const auto &parameter : *parsed->parameters) {
    const auto equals = parameter.find('=');
    if (equals == parameter.npos) ret->arguments.push_back({.name = std::string(parameter)});
    else
      ret->arguments.push_back({.name = std::string(parameter.substr(0, equals)),
                                .default_value = std::string(parameter.substr(equals + 1))});
  }
  ret->body = parsed->definition;
  return ret;
}
