#pragma once
#include "help/builtins/registry.hpp"
#include "toolchain/macro/registry.hpp"

namespace helpers {
QSharedPointer<builtins::Registry> builtins_registry(bool use_app_settings,
                                                     QString directory = builtins::default_book_path);
QSharedPointer<builtins::Registry> registry_with_assemblers(QString directory = builtins::default_book_path);
QSharedPointer<macro::Registry> registry(QSharedPointer<const builtins::Book> book, QStringList directory);
QSharedPointer<const builtins::Book> book(int ed, const builtins::Registry *reg);
QSharedPointer<macro::Registry> cs5e_macros(const builtins::Registry *reg);
QSharedPointer<macro::Registry> cs6e_macros(const builtins::Registry *reg);
} // namespace helpers
