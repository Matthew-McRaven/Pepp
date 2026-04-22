#pragma once
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace pepp::tc {

struct MacroDefinition {
  struct Argument {
    std::string name;
    std::optional<std::string> default_value;
  };
  std::string name;
  std::vector<Argument> arguments;
  std::string body;
};

// A collection of macro definitions searchable by name. A registry can have a parent which can be searched on misses in
// this instance. This hierarchy allows for macro definitions to be reused across multiple assembly source files without
// additional reallocations. If a child registry wishes to redefine a macro from the parent, it can insert a new
// definition with the same name and SearchMode::Local. If redefining an already-existing macro in this instance, pair a
// purge+insert(...,SearchMode::Local).
class MacroRegistry {
public:
  enum class SearchMode {
    Local,           // Operation only checks this registry instance.
    Parent,          // Operation only checks this instance's ancestors.
    LocalThenParent, // Operation check this instance before ancestors.
  };
  explicit MacroRegistry(std::shared_ptr<const MacroRegistry> parent = nullptr);
  bool contains(const std::string &name, SearchMode mode = SearchMode::LocalThenParent) const;
  std::shared_ptr<const MacroDefinition> find(const std::string &name,
                                              SearchMode mode = SearchMode::LocalThenParent) const;
  // Remove a macro from this registry without affecting the (const) parents.
  // Will insert a tombstone record to prevent LocalThenParent from recursing into the parent.
  // Existing macros instantiations will continue to use their existing MacroDefinition after a purge.
  void purge(const std::string &name);

  // If a macro of the same name exists in the given search path that is not deleted, return false.
  // Otherwise, the maco is inserted into this registry and returns true.
  // To overwrite a macro definition from a parent, insert with SearchMode::Local.
  // To replace an existing definition in this registry, purge(name)+insert(...,SearchMode::Local).
  bool insert(std::shared_ptr<MacroDefinition> definition, SearchMode mode = SearchMode::LocalThenParent);

private:
  struct MacroRecord {
    // Must be mutable so purge() can modify through the pointer returned by find().
    mutable bool deleted = false;
    std::shared_ptr<MacroDefinition> definition;
  };
  struct Hash {
    using is_transparent = void;
    size_t operator()(const std::shared_ptr<MacroDefinition> &md) const { return std::hash<std::string>{}(md->name); }
    size_t operator()(const MacroRecord &md) const { return std::hash<std::string>{}(md.definition->name); }
    size_t operator()(std::string_view sv) const { return std::hash<std::string_view>{}(sv); }
  };
  struct Equals {
    using is_transparent = void;
    bool operator()(const MacroRecord &a, std::string_view b) const { return a.definition->name == b; }
    bool operator()(std::string_view a, const MacroRecord &b) const { return a == b.definition->name; }
    bool operator()(const MacroRecord &a, const MacroRecord &b) const {
      return a.definition->name == b.definition->name;
    }
  };
  std::shared_ptr<const MacroRegistry> _parent = nullptr;
  std::unordered_set<MacroRecord, Hash, Equals> _macros;
};
} // namespace pepp::tc
