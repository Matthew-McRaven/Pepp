// File: table.tpp
/*
    The Pep/10 suite of applications (Pep10, Pep10CPU, Pep10Term) are
    simulators for the Pep/10 virtual machine, and allow users to
    create, simulate, and debug across various levels of abstraction.

    Copyright (C) 2021 J. Stanley Warford & Matthew McRaven, Pepperdine University

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "visit.hpp"

template<typename value_t>
[[maybe_unused]] symbol::BranchTable<value_t>::BranchTable(std::shared_ptr<BranchTable<value_t>> parent) : parent_(
    parent) {}

template<typename value_t> void symbol::BranchTable<value_t>::add_child(NodeType<value_t> child) {
  children_.push_back(child);
}

template<typename value_t>
symbol::LeafTable<value_t>::LeafTable(std::shared_ptr<BranchTable<value_t>> parent) : parent_(parent) {}
template<typename value_t>
std::optional<std::shared_ptr<symbol::entry<value_t>>> symbol::LeafTable<value_t>::get(const std::string &name) const {
  if (auto item = name_to_entry_.find(name); item != name_to_entry_.end())
    return item->second;
  else
    return std::nullopt;
}

template<typename value_t>
typename symbol::LeafTable<value_t>::entry_ptr_t symbol::LeafTable<value_t>::reference(const std::string &name) {
  // Create a local definition if one does not already exist
  entry_ptr_t local_definition;
  if (auto pair = name_to_entry_.find(name); pair == name_to_entry_.end()) {
    local_definition = std::make_shared<symbol::entry<value_t>>(*this, name);
    name_to_entry_[name] = local_definition;
  } else {
    local_definition = pair->second;
  }

  // Check for the presence of other symbols with the same name
  auto as_ptr = this->shared_from_this();
  auto symbols = symbol::select_by_name<value_t>({as_ptr}, name, TraversalPolicy::kWholeTree);
  int global_count = 0;
  for (auto symbol : symbols) {
    if (&symbol->parent == &*this)
      continue; // We will be examining our symbols later.
    global_count += (symbol->binding == symbol::binding_t::kGlobal) ? 1 : 0;
  }

  // If there's more than one global definition, we already know that the program is invalid.
  if (global_count > 1)
    local_definition->state = symbol::definition_state::kExternalMultiple;
    // If there's one definition, take on most of the properties of that definition
  else if (global_count == 1) {
    for (auto other : symbols) {
      if (other->binding == symbol::binding_t::kGlobal) {
        local_definition->value = std::make_shared<symbol::value_pointer<value_t>>(other);
        // Mark the symbol as imported, so that we can tell the difference between our
        // global symbols and others' globals.
        local_definition->binding = symbol::binding_t::kImported;
        local_definition->state = other->state;
      }
    }
  }
  return local_definition;
}

template<typename value_t>
typename symbol::LeafTable<value_t>::entry_ptr_t symbol::LeafTable<value_t>::define(const std::string &name) {
  auto entry = reference(name);

  // Check for the presence of other symbols with the same name
  auto as_ptr = this->shared_from_this();
  auto same_name = symbol::select_by_name<value_t>(as_ptr, name, TraversalPolicy::kWholeTree);

  switch (entry->binding) {
  case symbol::binding_t::kImported:entry->state = definition_state::kExternalMultiple;
    return entry;
  case symbol::binding_t::kGlobal:
    if (entry->state == definition_state::kUndefined)
      entry->state = definition_state::kSingle;
    else if (entry->state == definition_state::kSingle)
      entry->state = definition_state::kMultiple;

    for (auto other : same_name) {
      if (&other->parent == &*this)
        continue;
      else if (other->binding == symbol::binding_t::kImported) {
        other->value = std::make_shared<symbol::value_pointer<value_t>>(entry);
        // Mark the symbol as imported, so that we can tell the difference between our
        // global symbols and others' globals.
        other->binding = symbol::binding_t::kImported;
        other->state = entry->state;
      }
    }
    return entry;
  case symbol::binding_t::kLocal:
    if (entry->state == definition_state::kUndefined)
      entry->state = definition_state::kSingle;
    else if (entry->state == definition_state::kSingle)
      entry->state = definition_state::kMultiple;
    return entry;
  }
}

template<typename value_t> void symbol::LeafTable<value_t>::mark_global(const std::string &name) {
  auto symbol = reference(name);
  symbol->binding = symbol::binding_t::kGlobal;

  // Check for the presence of other symbols with the same name
  auto as_ptr = this->shared_from_this();
  // Must gather all symbols from entire tree, otherwise some local references may not be updated.
  auto same_name = symbol::select_by_name<value_t>({as_ptr}, name, TraversalPolicy::kWholeTree);

  for (auto other : same_name) {
    if (&other->parent == &*this)
      continue; // We will be examining our symbols later.
    if (other->binding == symbol::binding_t::kGlobal) {
      other->state = symbol::definition_state::kExternalMultiple;
      symbol->state = symbol::definition_state::kExternalMultiple;
    } else if (other->binding == symbol::binding_t::kLocal) {
      other->value = std::make_shared<symbol::value_pointer<value_t>>(symbol);
      // Mark the symbol as imported, so that we can tell the difference between our
      // global symbols and others' globals.
      other->binding = symbol::binding_t::kImported;
      // Follow the other symbols definition state to prevent the other copy from being undefined.
      other->state = symbol->state;
    }
  }
}

template<typename value_t> bool symbol::LeafTable<value_t>::exists(const std::string &name) const {
  return name_to_entry_.find(name) != name_to_entry_.end();
}

template<typename value_t>
auto symbol::LeafTable<value_t>::entries() const -> symbol::LeafTable<value_t>::const_range {
  return name_to_entry_ | boost::adaptors::map_values;
}

template<typename value_t> auto symbol::LeafTable<value_t>::entries() -> symbol::LeafTable<value_t>::range {
  return name_to_entry_ | boost::adaptors::map_values;
}

template<typename value_t>
std::shared_ptr<symbol::LeafTable<value_t>> symbol::insert_leaf(std::shared_ptr<symbol::BranchTable<value_t>> parent) {
  auto ret = std::make_shared<symbol::LeafTable<value_t>>(parent);
  parent->add_child(ret);
  return ret;
}

template<typename value_t>
std::shared_ptr<symbol::BranchTable<value_t>>
symbol::insert_branch(std::shared_ptr<symbol::BranchTable<value_t>> parent) {
  auto ret = std::make_shared<symbol::BranchTable<value_t>>(parent);
  parent->add_child(ret);
  return ret;
}

template<typename value_t> symbol::NodeType<value_t> symbol::parent(symbol::NodeType<value_t> table) {
  if (std::holds_alternative<std::shared_ptr<BranchTable<value_t>>>(table)) {
    auto ptr = std::get<std::shared_ptr<BranchTable<value_t>>>(table);
    if (auto parent = ptr->parent_.lock(); parent)
      return parent;
    else
      return ptr;
  } else if (std::holds_alternative<std::shared_ptr<LeafTable<value_t>>>(table)) {
    auto ptr = std::get<std::shared_ptr<LeafTable<value_t>>>(table);
    if (auto parent = ptr->parent_.lock(); parent)
      return parent;
    else
      return ptr;
  } else
    assert(0 && "Something went wrong with the variant type conversion");
  // This will never be reached, but compiler can't tell that above assert is a catch-all.
  return {};
}