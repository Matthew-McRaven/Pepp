#pragma once
#include <functional>
#include "./packed_types.hpp"

namespace pepp::bts {

// A fixup which requires no additional parameters to function.
// If there is a dependency between fixups, you must order them manually.
// They are used to solve problems of the following type:
//   You need a _DYNAMIC symbol that points to the start of the .dynamic section.
//   To know the start of the .dynamic section, the file needs to be laid out.
//   However, adding the _DYNAMIC symbol changes the layout.
// This class can be used to break that dependency loop for fixed-size types using the following pattern.
//   Allocate the _DYNAMIC symbol, set its value to 0.
//   Create a function which assigns _DYNAMIC's value to the sh_addr of the .dynamic section.
//   ...
//   Perform layout, and apply all fixups.
struct AbsoluteFixup {
  std::function<void()> update;
};

} // namespace pepp::bts
