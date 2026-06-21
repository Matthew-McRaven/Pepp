#pragma once
#include "core/resources/figures/builtin_registry.hpp"
#include "help/builtins/figure_wrappers.hpp"
#include "toolchain/macro/registry.hpp"

namespace helpers {
std::shared_ptr<pepp::BuiltinRegistry> builtins_registry(bool use_app_settings,
                                                         QString directory = builtins::default_book_path);
std::shared_ptr<pepp::BuiltinRegistry> registry_with_assemblers(bool use_app_settings = false,
                                                                QString directory = builtins::default_book_path);
std::shared_ptr<const pepp::Book> book(int ed, const pepp::BuiltinRegistry *reg);

QSharedPointer<macro::Registry> registry(std::shared_ptr<const pepp::Book> book, QStringList directory);
QSharedPointer<macro::Registry> cs5e_macros(const pepp::BuiltinRegistry *reg);
QSharedPointer<macro::Registry> cs6e_macros(const pepp::BuiltinRegistry *reg);
} // namespace helpers
