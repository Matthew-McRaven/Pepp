#include "sanity.hpp"

#include <boost/algorithm/string.hpp>

template <typename address_size_t>
bool whole_program_sanity_fixup(std::shared_ptr<masm::project::target<uint16_t>> &target,
                                std::shared_ptr<ir::section::code_section<uint16_t>> section) {
    bool success = true;
    int end_count = 0, burn_count = 0;
    auto &ir_lines = section->ir_lines;

    // TODO: Make these checks make sense with linker...
    for (auto &line : ir_lines) {
        if (auto ptr = std::dynamic_pointer_cast<ir::dot_burn<uint16_t>>(line); ptr)
            burn_count++;
        if (auto ptr = std::dynamic_pointer_cast<ir::dot_end<uint16_t>>(line); ptr)
            end_count++;
    }
    /*
     * Sanity checks on assembled program.
     */
    // Require that a .END be present.
    /*auto last_line_index = ir_lines.back()->source_line;
    if (end_count == 0) {
        project->message_resolver->log_message(section, last_line_index,
                                               {masm::message_type::kError, ";ERROR: Program must end with .END"});
        success = false;
    } else if (end_count > 1) {
        success = false;
        for (auto &line : ir_lines) {
            if (auto ptr = std::dynamic_pointer_cast<ir::dot_end<uint16_t>>(line); ptr) {
                project->message_resolver->log_message(
                    section, line->source_line, {masm::message_type::kError, ";ERROR: Duplicate .END directives."});
            }
        }
    } else if (burn_count > 0 && section->headers.section_type == masm::elf::program_type::kUserProgram) {
        success = false;
        for (auto &line : ir_lines) {
            if (auto ptr = std::dynamic_pointer_cast<ir::dot_burn<uint16_t>>(line); ptr) {
                project->message_resolver->log_message(
                    section, line->source_line,
                    {masm::message_type::kError, ";ERROR: .BURN is illegal in user programs."});
            } else if (auto ptr = std::dynamic_pointer_cast<ir::dot_input<uint16_t>>(line); ptr) {
                project->message_resolver->log_message(
                    section, line->source_line,
                    {masm::message_type::kError, ";ERROR: .INPUT is illegal in user programs."});
            } else if (auto ptr = std::dynamic_pointer_cast<ir::dot_output<uint16_t>>(line); ptr) {
                project->message_resolver->log_message(
                    section, line->source_line,
                    {masm::message_type::kError, ";ERROR: .OUTPUT is illegal in user programs."});
            }
        }
    }
    // Check that a USER program doesn't have a BURN.
    else if (burn_count == 0 && section->header.section_type == masm::elf::program_type::kOperatingSystem) {
        success = false;
        project->message_resolver->log_message(section, 0,
                                               {masm::message_type::kError, ";ERROR: .BURN must be present in OS."});
    }
    // Check that OS has exactly 1 BURN.
    else if (burn_count > 1 && section->header.section_type == masm::elf::program_type::kOperatingSystem) {
        success = false;
        for (auto &line : ir_lines) {
            if (auto ptr = std::dynamic_pointer_cast<ir::dot_burn<uint16_t>>(line); ptr) {
                project->message_resolver->log_message(
                    section, line->source_line,
                    {masm::message_type::kError, ";ERROR: .BURN is illegal in user programs."});
            }
        }
    }

    // Find the address, source line of a .BURN, and suppress all object code
    // from being generated on lines above the .BURN.
    auto burn_source_index = 0;
    if (section->header.section_type == masm::elf::program_type::kOperatingSystem) {
        for (auto &line : ir_lines) {
            if (auto ptr = std::dynamic_pointer_cast<ir::dot_burn<uint16_t>>(line); ptr) {
                section->body_ir->BURN_address = ptr->argument->value();
                burn_source_index = ptr->source_line;
                break;
            }
        }

        std::list<std::shared_ptr<ir::macro_invocation<uint16_t>>> macro_fixups;
        // Flag any line of code prior to a BURN as not generating object code.
        for (auto &line : ir_lines) {
            if (line->source_line < burn_source_index) {
                if (auto ptr = std::dynamic_pointer_cast<ir::macro_invocation<uint16_t>>(line); ptr)
                    macro_fixups.push_back(ptr);
                line->emits_object_code = false;
            }
        }

        // Recurse into each macro definition to prevent object code generation.
        while (!macro_fixups.empty()) {
            auto macro = macro_fixups.front();
            macro_fixups.pop_front();

            for (auto &line : macro->macro->ir_lines) {
                if (auto ptr = std::dynamic_pointer_cast<ir::macro_invocation<uint16_t>>(line); ptr)
                    macro_fixups.push_back(ptr);
                line->emits_object_code = false;
            }
        }
    }*/
    return success;
}