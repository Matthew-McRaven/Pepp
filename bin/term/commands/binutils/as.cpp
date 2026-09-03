#include "as.hpp"
#include <elfio/elfio.hpp>
#include <fstream>
#include <iostream>

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
    fmt_cfg.listing_format = [&](std::vector<std::string> &&lines) {
      for (const auto &line : lines) std::cout << line << "\n";
    };
    fmt_cfg.listing_config = _opts.listing_config;
  }

  // Extract text for all input files.
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
  // Concatenate all source files into a newline-delimited string.
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
    std::cerr << "Assembly failed with " << result.diagnostics.count() << " error(s):\n";
    for (const auto &diag : result.diagnostics) std::cerr << diag.second << "\n";
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
