#include "as.hpp"
#include <fstream>
#include <iostream>
#include <regex>
#include "core/resources/figures/book.hpp"
#include "toolchain/helpers/assemblerregistry.hpp"

namespace {
// Writes each line followed by a newline to `path`, or to `default_out` when path == "-".
void write_lines(const std::string &path, const std::vector<std::string> &lines,
                  std::ostream &default_out = std::cout) {
  if (path == "-") {
    for (const auto &line : lines) default_out << line << "\n";
    return;
  }
  std::ofstream out(path, std::ios::binary);
  if (!out.is_open()) {
    std::cerr << "Error: Could not open output file: " << path << "\n";
    return;
  }
  for (const auto &line : lines) out << line << "\n";
}

// Book macros still use the old assembler's $N arguments, which we rename to \argN for the new parser.
void add_book_macros(pepp::tc::MacroRegistry &registry) {
  static const std::regex positional(R"(\$([0-9]+))");
  const auto books = helpers::builtins_registry(false);
  const auto book = helpers::book(6, &*books);
  if (book == nullptr) return;
  for (const auto &file : book->macros()) {
    auto def = std::make_shared<pepp::tc::MacroDefinition>();
    if (file->name.starts_with("@")) def->name = file->name;
    else def->name = std::string("@") + file->name;
    for (int it = 1; it <= file->argcount; it++) def->arguments.push_back({.name = "arg" + std::to_string(it)});
    def->body = std::regex_replace(file->body, positional, R"(\arg$1)");
    registry.insert(def);
  }
}

// To avoid having to parse the OS, we hardcode the system call macros here.
// This is a temporary workaround until we have a more general way of extracting macros from a file.
void add_os_macros(pepp::tc::MacroRegistry &registry) {
  for (const auto name : {"DECI", "DECO", "STRO", "HEXO", "SNOP"}) {
    auto def = std::make_shared<pepp::tc::MacroDefinition>();
    def->name = std::string("@") + name;
    def->arguments = {{.name = "arg1"}, {.name = "arg2"}};
    def->body = std::string("LDWA ") + name + ",i\nSCALL \\arg1,\\arg2";
    registry.insert(def);
  }
}
} // namespace

AsTask::AsTask(Options &opts, ArchOptions arch_opts, QObject *parent)
    : Task(parent), _opts(opts), _arch_opts(std::move(arch_opts)) {}

void AsTask::run() {
  if (_opts.arch == pepp::Architecture::NO_ARCH) {
    std::cerr << "Error: No architecture specified. Use -march to specify an architecture.\n";
    return emit finished(1);
  }
  const auto cfg = std::visit([this](const auto &arch_opts) { return prepare(arch_opts); }, _arch_opts);
  pepp::tc::FormattingConfig fmt_cfg;
  if (_opts.listing_enable) {
    fmt_cfg.listing_format = [&](std::vector<std::string> &&lines) { write_lines(_opts.file_listing, lines); };
    fmt_cfg.listing_config = _opts.listing_config;
  }
  if (_opts.format_source_enable) {
    fmt_cfg.source_format = [&](std::vector<std::string> &&lines) { write_lines(_opts.file_fmt_source, lines); };
  }

  // Extract text for all input files...
  std::vector<std::string> sources;
  for (const auto &file_source : _opts.file_sources) {
    std::string tmp;
    if (file_source == "-") {
      tmp.append(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
    } else {
      std::ifstream source_file(file_source, std::ios::binary);
      if (!source_file.is_open()) {
        std::cerr << "Error: Could not open source file: " << file_source << "\n";
        return emit finished(1);
      }
      // Read the entire file into a string via a single read operation.
      source_file.seekg(0, std::ios::end);
      tmp.resize(source_file.tellg());
      source_file.seekg(0);
      source_file.read(tmp.data(), tmp.size());
    }
    sources.push_back(std::move(tmp));
  }

  // ... and concatenate those file contents into a single newline-delimited string.
  std::string source_contents = "";
  {
    int total_size = 0;
    for (const auto &s : sources) total_size += s.size();
    total_size += sources.size() - 1; // for newlines between files

    source_contents.reserve(total_size);
    for (const auto &s : sources) {
      source_contents.append(s);
      source_contents.append("\n");
    }
  }

  auto result = pepp::tc::assemble(cfg, fmt_cfg, std::move(source_contents));
  if (!result.ok()) {
    // Write error messages either to the designated error file, which is cerr by default.
    std::vector<std::string> diag_lines;
    diag_lines.reserve(result.diagnostics.count() + 1);
    diag_lines.push_back("Assembly failed with " + std::to_string(result.diagnostics.count()) + " error(s):");
    for (const auto &diag : result.diagnostics) diag_lines.push_back(diag.second);
    write_lines(_opts.file_errs, diag_lines, std::cerr);
    return emit finished(1);
  } else {
    std::ofstream out(_opts.file_elf, std::ios::binary);
    if (!out) {
      write_lines(_opts.file_errs, {"Could not open " + _opts.file_elf + " for writing"}, std::cerr);
      return emit finished(1);
    }
    pepp::tc::write_elf(result.elf, out);
    if (!out.flush()) {
      write_lines(_opts.file_errs, {"Could not write " + _opts.file_elf}, std::cerr);
      return emit finished(1);
    }
  }

  return emit finished(0);
}

pepp::tc::DriverConfig AsTask::prepare(const RISCVOptions &) {
  return pepp::tc::RISCVDriverConfig{.symdefs = _opts.symdefs};
}

pepp::tc::DriverConfig AsTask::prepare(const PEP10Options &arch) {
  std::shared_ptr<pepp::tc::MacroRegistry> macros = nullptr;
  if (arch.default_macros || arch.os_macros) {
    macros = std::make_shared<pepp::tc::MacroRegistry>();
    if (arch.default_macros) add_book_macros(*macros);
    if (arch.os_macros) add_os_macros(*macros);
  }
  return pepp::tc::Pep10DriverConfig{.symdefs = _opts.symdefs, .macros = macros};
}
