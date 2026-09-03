#include "as.hpp"
#include <elfio/elfio.hpp>
#include <fstream>
#include <iostream>

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
} // namespace

AsTask::AsTask(Options &opts, QObject *parent) : Task(parent), _opts(opts) {}

void AsTask::run() {
  pepp::tc::DriverConfig cfg;
  switch (_opts.arch) {
  case pepp::Architecture::NO_ARCH:
    std::cerr << "Error: No architecture specified. Use -march to specify an architecture.\n";
    return emit finished(1);
  case pepp::Architecture::PEP8: cfg = prepare_pep(); break;
  case pepp::Architecture::PEP9: cfg = prepare_pep(); break;
  case pepp::Architecture::PEP10: cfg = prepare_pep(); break;
  case pepp::Architecture::RISCV: cfg = prepare_riscv(); break;
  }
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
  } else if (result.elf.elf) result.elf.elf->save(_opts.file_elf);

  return emit finished(0);
}

pepp::tc::DriverConfig AsTask::prepare_riscv() {
  return pepp::tc::RISCVDriverConfig{};
}

pepp::tc::DriverConfig AsTask::prepare_pep() {
  return pepp::tc::Pep10DriverConfig{};
}
