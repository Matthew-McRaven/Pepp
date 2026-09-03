/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include <CLI11.hpp>
#include <variant>
#include <vector>
#include "../../shared.hpp"
#include "../../task.hpp"
#include "core/architectures.hpp"
#include "core/langs/asmb_driver.hpp"
namespace ELFIO {
class elfio;
}

class AsTask : public Task {
public:
  struct Options {
    // At least one file_sources entry is required.
    std::vector<std::string> file_sources;
    std::string file_elf = "a.out", file_listing = "-", file_fmt_source = "", file_errs = "-";
    bool listing_enable = false;
    bool format_source_enable = false;
    pepp::tc::ListingConfig listing_config = {};
    pepp::Architecture arch = pepp::Architecture::NO_ARCH;
    // The -march part of  after the family prefix, e.g. "imc" for "rv32imc"; empty for Pep
    std::string arch_variant = "";
  };
  struct RISCVOptions : public Options {};
  struct PEP10Options : public Options {};

  AsTask(Options &opts, QObject *parent = nullptr);
  void run() override;

private:
  pepp::tc::DriverConfig prepare_riscv();
  pepp::tc::DriverConfig prepare_pep();
  Options &_opts;
};

void registerAs(auto &app, task_factory_t &task, detail::SharedFlags &flags) {
  static AsTask::Options opts;
  static std::string a_text;
  static std::string march_text;
  static auto as_clone = app.add_subcommand("as", "GNU as-compatible assembler");
  as_clone->allow_non_standard_option_names();
  static const auto march_opt =
      as_clone->add_option("-march", march_text, "Specify target architecture, e.g. rv32imc or pep10")
          ->option_text("<arch>");
  static const auto a_opts = as_clone
                                 ->add_option("-a", a_text,
                                              "Turn on listings, sub-options:\n"
                                              "c      omit false conditionals\n"
                                              "d      omit debugging directives\n"
                                              "m      include macro expansions\n"
                                              "=file  set listing file name (must be last sub-option)")
                                 ->expected(0, 1)
                                 ->default_val("")
                                 ->option_text("[suboption...]");
  as_clone->add_option("files", opts.file_sources, "Source file(s) to assemble")->required();
  as_clone->add_option("-o", opts.file_elf, "Output ELF file name")->option_text("<file>");
  static const auto fmt_opts =
      as_clone->add_option("--format", opts.file_fmt_source, "Output formatted source file name")
          ->option_text("<file>");

  as_clone->add_option("-e,--errors", opts.file_errs, "Output errors file name. Defaults to cerr")
      ->option_text("<file>");

  as_clone->callback([&]() {
    opts.format_source_enable = fmt_opts->count() > 0;
    // Use count() rather than a_text.empty(), since a bare "-a"  and an absent "-a" both leave a_text == "".
    opts.listing_enable = a_opts->count() > 0;
    // Handle listing options.
    if (!a_text.empty()) {
      const auto eq_pos = a_text.find('=');
      const std::string sub_flags = a_text.substr(0, eq_pos);
      if (sub_flags.find('c') != std::string::npos) opts.listing_config.omit_false_conditionals = true;
      if (sub_flags.find('d') != std::string::npos) opts.listing_config.omit_debugging_directives = true;
      if (sub_flags.find('m') != std::string::npos) opts.listing_config.include_macro_expansions = true;
      if (eq_pos != std::string::npos) opts.file_listing = a_text.substr(eq_pos + 1);
    }

    // Handle -march
    using PA = pepp::Architecture;
    if (march_text.rfind("rv32", 0) == 0) {
      opts.arch = PA::RISCV;
      opts.arch_variant = march_text.substr(4);
    } else if (march_text.rfind("pep10", 0) == 0) opts.arch = PA::PEP10;
    else if (march_text.rfind("pep9", 0) == 0) opts.arch = PA::PEP9;
    else if (march_text.rfind("pep8", 0) == 0) opts.arch = PA::PEP8;

    flags.kind = detail::SharedFlags::Kind::TERM;
    task = [&](QObject *parent) { return new AsTask(opts, parent); };
  });
}
