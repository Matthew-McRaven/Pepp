#pragma once
#include "help/builtins/registry.hpp"

namespace helpers {
QSharedPointer<builtins::Registry> registry_with_assemblers(QString directory = builtins::default_book_path);
}
