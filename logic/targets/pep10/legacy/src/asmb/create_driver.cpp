#include "create_driver.hpp"

#include <memory>

#include "./sanity.hpp"
#include "asmdr/project/project.hpp"
#include "ex_registry.hpp"

std::shared_ptr<asmb::pep10::driver::driver_t> asmb::pep10::driver::make_driver() {
  using namespace asmb::pep10::driver;
  auto driver = std::make_shared<driver_t>();

  auto tokenizer = std::make_shared<masm::frontend::tokenizer<uint16_t, tokenizer_t>>();
  transform_t tx_tokenizer = [=](target_t &target, std::list<driver_t::work_t> &work) {
    bool success = true;
    driver_t::work_iterable_t result_work;
    std::transform(work.begin(), work.end(), std::back_inserter(result_work), [&](auto &value) {
      success &= tokenizer->tokenize(target, std::get<driver_t::section_t>(value));
      return driver_t::work_iterable_t::value_type{stage_t::TOKEN, value};
    });
    return driver_t::result_t{success, result_work};
  };
  driver->register_transform(tx_tokenizer, stage_t::RAW);

  auto preproc = std::make_shared<masm::frontend::preprocessor<uint16_t, tokenizer_t>>();
  transform_t tx_preproc = [=](target_t &target, std::list<driver_t::work_t> &work) {
    bool success = true;
    driver_t::work_iterable_t result_work;
    for (auto value : work) {
      auto [local_success, local_children] = preproc->preprocess(target, std::get<driver_t::section_t>(value));
      for (const auto &child : local_children) {
        result_work.emplace_back(driver_t::work_iterable_t::value_type{
            stage_t::RAW, std::static_pointer_cast<ir::section::code_section<uint16_t>>(child)});
      }
      result_work.emplace_back(driver_t::work_iterable_t::value_type{stage_t::PREPROCESS, value});
      success &= local_success;
    }
    return driver_t::result_t{success, result_work};
  };
  driver->register_transform(tx_preproc, stage_t::TOKEN);

  auto parser = std::make_shared<asmb::pep10::parser>();
  transform_t tx_parser = [=](target_t &target, std::list<driver_t::work_t> &work) {
    bool success = true;
    driver_t::work_iterable_t result_work;
    std::transform(work.begin(), work.end(), std::back_inserter(result_work), [&](auto &value) {
      success &= parser->parse(target, std::get<driver_t::section_t>(value));
      return driver_t::work_iterable_t::value_type{stage_t::SYMANTIC, value};
    });
    return driver_t::result_t{success, result_work};
  };
  driver->register_transform(tx_parser, stage_t::PREPROCESS);

  transform_t tx_sanity1 = [=](target_t &target, std::list<driver_t::work_t> &work) {
    bool success = true;
    driver_t::work_iterable_t result_work;
    using tls_t = std::shared_ptr<ir::section::code_section<uint16_t>>;
    std::map<tls_t, bool> parsed_tls;
    std::transform(work.begin(), work.end(), std::back_inserter(result_work), [&](auto &value) {
      auto container = ir::section::find_container(std::get<driver_t::section_t>(value));
      for (auto tls : container->sections) {
        if (parsed_tls.find(tls) == parsed_tls.end()) {
          bool val = whole_program_sanity_fixup<uint16_t>(target, tls);
          success &= parsed_tls[tls] = val;
        }
      }
      return driver_t::work_iterable_t::value_type{stage_t::WHOLE_PROGRAM_SANITY, value};
    });
    return driver_t::result_t{success, result_work};
  };
  driver->register_transform(tx_sanity1, stage_t::SYMANTIC);

  transform_t tx_addr = [=](target_t &target, std::list<driver_t::work_t> &work) {
    bool success = true;
    std::set<std::shared_ptr<ir::section::container<uint16_t>>> containers;
    for (auto &value : work) {
      auto container = ir::section::find_container(std::get<driver_t::section_t>(value));
      containers.insert(container);
    }

    driver_t::work_iterable_t result_work;
    std::transform(containers.begin(), containers.end(), std::back_inserter(result_work), [&](auto container) {
      // Clang bug prevented success from being mutated directly ??
      auto help = masm::backend::assign_image(target, container);
      success &= help;
      return driver_t::work_iterable_t::value_type{stage_t::ADDRESS_ASSIGN, container};
    });
    return driver_t::result_t{success, result_work};
  };
  driver->register_transform(tx_addr, stage_t::WHOLE_PROGRAM_SANITY);

  transform_t tx_elf = [=](target_t &target, std::list<driver_t::work_t> &work) {
    bool success = true;
    driver_t::work_iterable_t result_work;
    for (auto value : work) {
      if (target->as_elf == nullptr) {
        success &= masm::elf::pack_image(target, std::get<driver_t::image_t>(value));
        result_work.emplace_back(driver_t::work_iterable_t::value_type{stage_t::PACK, target->as_elf});
      }
    }
    return driver_t::result_t{success, result_work};
  };
  driver->register_transform(tx_elf, stage_t::ADDRESS_ASSIGN);

  transform_t tx_end = [=](target_t &target, std::list<driver_t::work_t> &work) {
    driver_t::work_iterable_t result_work;
    std::transform(work.begin(), work.end(), std::back_inserter(result_work), [&](auto elf_image) {
      return driver_t::work_iterable_t::value_type{stage_t::FINISHED, elf_image};
    });
    return driver_t::result_t{true, result_work};
  };
  driver->register_transform(tx_end, stage_t::PACK);

  return driver;
}

std::tuple<bool, typename asmb::pep10::driver::driver_t::group_t> asmb::pep10::driver::assemble(std::string user_text,
                                                                                                std::string os_text) {
  using namespace asmb::pep10::driver;
  auto driver = make_driver();
  auto ex = registry();
  auto fill_registry = [&ex](std::shared_ptr<masm::macro_registry> reg) {
    // TODO: Reference correct book (all spelled out), and handle case where book is not found.
    auto cs6e = *(ex.find_book("cs6e"));
    for (const auto &macro : cs6e->macros())
      reg->register_macro(macro.name, macro.text, masm::MacroType::CoreMacro);
  };

  std::vector<std::shared_ptr<masm::project::source>> sources;

  // Macro registry created automatically.
  auto os = std::make_shared<masm::project::source>();
  os->name = "os";
  os->body = os_text;
  fill_registry(os->macro_registry);
  sources.emplace_back(os);

  auto user = std::make_shared<masm::project::source>();
  user->name = "user";
  user->body = user_text;
  fill_registry(user->macro_registry);
  sources.emplace_back(user);

  auto group = masm::project::init_group<uint16_t>(sources);

  auto res = driver->assemble(group, masm::project::toolchain_stage::PACK);
  return {res.first, group};
}
