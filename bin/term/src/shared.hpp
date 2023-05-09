#pragma once
#include "builtins/book.hpp"
#include "builtins/registry.hpp"
namespace detail {
QSharedPointer<const builtins::Book> book(int ed);
}
