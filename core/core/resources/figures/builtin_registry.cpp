#include "builtin_registry.hpp"
#include <deque>
#include <nlohmann/json.hpp>
#include <regex>
#include <spdlog/spdlog.h>
#include "core/resources/figures/book.hpp"
#include "core/resources/figures/figure.hpp"
#include "core/resources/figures/fragment.hpp"

namespace {

using _Figure = std::shared_ptr<pepp::Figure>;
using _Macro = std::list<std::shared_ptr<pepp::MacroFile>>;

// Helper to extract value from a map or return a default value if not present
template <class Map, class K, class V> Map::mapped_type value_or(const Map &m, const K &key, V fallback) {
  auto it = m.find(key);
  return it != m.end() ? it->second : fallback;
}

// Line selection logic in fragments allows negative indices (which count lines from the end of a file).
// This helper converts negative indices to the correct, positive ones and leaves postive ones unchanged.
int absolute_index(std::vector<int> &list, int index, int _default = 0) {
  if (index >= 0 && index < list.size()) return index;
  else if (index < 0 && index >= -list.size()) return list.size() + index;
  else return _default;
}

// Extract a subset of lines from the input string, where lines are separated by '\n'.
std::string_view select_lines(const std::string &input, int startLine, int endLine) {
  std::vector<int> lineStarts = {0};
  for (int pos = -1; (pos = input.find('\n', pos + 1)) != std::string::npos;) lineStarts.push_back(pos + 1);
  lineStarts.push_back(input.size() + 1);
  auto startIdx = absolute_index(lineStarts, startLine, 0),
       endIdx = absolute_index(lineStarts, endLine, input.size() - 1);
  if (endIdx < startIdx) std::swap(startIdx, endIdx);

  std::size_t from = lineStarts[startIdx];
  std::size_t len = lineStarts[endIdx] - from;
  if (from > input.size()) return {};
  return std::string_view{input}.substr(from, std::min(len, input.size() - from));
}

// Perform template substitution on a single string
std::string templatize(std::string input, std::map<std::string, std::string> substitutions) {
  for (const auto &[key, value] : substitutions) {
    const std::string token = "{" + key + "}";
    for (std::size_t pos = input.find(token); pos != std::string::npos; pos = input.find(token, pos + value.size()))
      input.replace(pos, token.size(), value);
  }
  return input;
}

// Perform template substitution on all string values in a JSON object, recursively.
void templateize(nlohmann::json &object, std::map<std::string, std::string> substitutions) {
  for (auto &[key, value] : object.items()) {
    if (value.is_object()) {
      templateize(value, substitutions);
    } else if (value.is_string()) {
      object[key] = templatize(value.get<std::string>(), substitutions);
    }
  }
}

// Helper to parse a name into (chapter, figure) parts
std::optional<std::pair<std::string, std::string>> ch_fig_from_str(const std::string &key) {
  // Chapter and figure are separated by ':' in a manifest file.
  auto colon = key.find(':');
  if (colon == std::string::npos) {
    spdlog::warn("Invalid chapter:figure name {}", key);
    return std::nullopt;
  }
  // Reject more than one ':' (matches the original's length != 2 check).
  if (key.find(':', colon + 1) != std::string::npos) {
    spdlog::warn("Invalid chapter:figure name {}", key);
    return std::nullopt;
  }
  return std::make_pair(key.substr(0, colon), key.substr(colon + 1));
}

std::optional<pepp::Architecture> arch_from_str(const std::string &key) {
  bool okay = false;
  auto ret = pepp::string_to_arch(key, &okay);
  if (!okay) {
    SPDLOG_WARN("Invalid figure architecture: {}", key);
    return std::nullopt;
  }
  return ret;
}

std::optional<pepp::Abstraction> abs_from_str(const std::string &key) {
  bool okay = false;
  auto ret = pepp::string_to_level(key, &okay);
  if (!okay) {
    SPDLOG_WARN("Invalid figure abstraction: {}", key);
    return std::nullopt;
  }
  return ret;
}

// Helpers for accessing JSON object values with default values
std::string str_of(const nlohmann::json &manifest, const std::string &key) {
  if (!manifest.contains(key)) return {};
  else if (!manifest[key].is_string()) {
    SPDLOG_WARN("Manifest key {} is not a string", key);
    return {};
  }
  return manifest[key].get<std::string>();
}

bool bool_of(const nlohmann::json &manifest, const std::string &key, bool _default = false) {
  if (!manifest.contains(key)) return _default;
  else if (!manifest[key].is_boolean()) {
    SPDLOG_WARN("Manifest key {} is not a boolean", key);
    return _default;
  }
  return manifest[key].get<bool>();
}

int int_of(const nlohmann::json &manifest, const std::string &key, int _default = 0) {
  if (!manifest.contains(key)) return _default;
  else if (!manifest[key].is_number_integer()) {
    SPDLOG_WARN("Manifest key {} is not an integer", key);
    return _default;
  }
  return manifest[key].get<int>();
}

int int_at(const nlohmann::json &lines, std::size_t i, int fallback) {
  return (i < lines.size() && lines[i].is_number_integer()) ? lines[i].get<int>() : fallback;
};

pepp::Test *load_test(const std::string &test_dir, const pepp::BuiltinRegistry::FilesystemProvider *fs) {
  auto test = new pepp::Test();
  // Explore the figure's subdirs, looking for inputs/outputs
  std::deque<std::string> to_visit;
  // Any time we visit a directory, scan that directory's children for more directories and manifest files.
  to_visit.push_back(test_dir);

  while (!to_visit.empty()) {
    auto file = to_visit.front();
    to_visit.pop_front();
    if (auto children = fs->enumerate_files(file); !children.empty()) {
      for (auto &child : children) to_visit.push_back(fs->dir_of(child, file));
      continue;
    } else if (!file.ends_with(".txt")) continue;
    auto data = fs->read_file(file);
    // replace all \r with nothing, normalizing line endings to \n
    std::erase(data, '\r');
    if (file.ends_with("input.txt")) test->input = data;
    else if (file.ends_with("output.txt")) test->output = data;
  }
  return test;
}

pepp::Fragment *load_fragment(const nlohmann::json &item, const std::string &manifest_dir, pepp::Figure *parent,
                              const pepp::BuiltinRegistry::FilesystemProvider *fs, pepp::BuiltinRegistry *registry) {
  // Use smart pointer to avoid cleanup on error paths.
  auto fragment = std::make_unique<pepp::Fragment>();
  fragment->name = str_of(item, "name");
  if (!item.contains("format") || !item["format"].is_string()) {
    SPDLOG_WARN("Invalid fragment format for {}", fragment->name);
    return nullptr;
  } else fragment->language = str_of(item, "format");

  fragment->isHidden = bool_of(item, "hidden", false);
  if (auto isDefault = bool_of(item, "isDefault", false); isDefault && fragment->isHidden) {
    SPDLOG_WARN("isDefault is incompatible with isHidden for {}", fragment->name);
    return nullptr;
  } else if (isDefault && fragment->name.empty()) {
    SPDLOG_WARN("Default fragment must be named {}", fragment->name);
    return nullptr;
  } else fragment->isDefault = isDefault;

  fragment->copyType = str_of(item, "copyType");
  fragment->exportPath = str_of(item, "export");

  if (!item.contains("from") || !item["from"].is_object()) {
    SPDLOG_WARN("Fragment {} missing required 'from' object", fragment->name);
    return nullptr;
  } else if (auto from = item["from"]; true) {
    // Require one of "element" or "file" to be present.
    if (from.contains("file") && from["file"].is_string()) {
      auto rel_path = str_of(from, "file");
      auto abs_path = fs->dir_of(rel_path, manifest_dir);
      fragment->contentsFn = [fs, abs_path]() { return fs->read_file(abs_path); };
    } else if (from.contains("element") && from["element"].is_string()) {
      auto key = str_of(from, "element");
      auto dependee = parent->find_fragment(key);
      if (dependee == nullptr) {

        SPDLOG_WARN("Fragment {} not found in figure {}", key, parent->name_full());
        return nullptr;
      }

      registry->add_dependency(fragment.get(), dependee);
      auto element_ptr = fragment.get();
      fragment->contentsFn = [registry, element_ptr]() { return registry->content_for(*element_ptr); };
    } else {
      SPDLOG_WARN("Fragment {} did not specify a valid source", fragment->name);
      return nullptr;
    }

    // Extract a subset of lines from the file.
    if (from.contains("lines") && from["lines"].is_array()) {
      auto contentsFn = fragment->contentsFn;
      if (auto lines = from["lines"]; !lines[0].is_number() || !lines[1].is_number()) {
        SPDLOG_WARN("Invalid line numbers for {}", fragment->name);
        return nullptr;
      } else {
        int start = int_at(lines, 0, 0), end = int_at(lines, 1, -1);
        fragment->contentsFn = [contentsFn, start, end]() {
          auto contents = contentsFn();
          return std::string{select_lines(contents, start, end)};
        };
      }
    }
  }

  return fragment.release();
}

std::variant<std::monostate, _Figure, _Macro> load_figure(const nlohmann::json &manifest, const std::string &path,
                                                          const pepp::BuiltinRegistry::FilesystemProvider *fs,
                                                          pepp::BuiltinRegistry *registry) {
  const auto manifest_dir = fs->dir_of(path);
  const auto type = str_of(manifest, "type");
  // Extract chapter/figure name
  auto ch_fig = ch_fig_from_str(str_of(manifest, "name"));
  if (!ch_fig) return std::monostate();
  auto [chapter_name, figure_name] = *ch_fig;

  // Extract architecture / abstraction from manifest into enumerated constants
  auto arch = pepp::Architecture::NO_ARCH;
  auto level = pepp::Abstraction::NO_ABS;
  if (auto maybe_arch = arch_from_str(str_of(manifest, "arch")); !maybe_arch) return std::monostate();
  else arch = *maybe_arch;
  if (auto maybe_level = abs_from_str(str_of(manifest, "abstraction")); !maybe_level) return std::monostate();
  else level = *maybe_level;
  auto feats = pepp::parse_features(manifest.value("features", std::string{}));

  const auto prefix = type == "problem" ? "Problem" : "Figure";
  auto figure =
      std::make_shared<pepp::Figure>(arch, level, feats, prefix, chapter_name, figure_name, type == "problem");

  figure->set_is_os(bool_of(manifest, "isOS", false));
  figure->set_is_hidden(bool_of(manifest, "hidden", false));
  if (manifest.contains("description") && manifest["description"].is_string())
    figure->set_description(str_of(manifest, "description"));

  // Add tests
  if (manifest.contains("tests") && manifest["tests"].is_array()) {
    for (const auto &tests_it : std::as_const(manifest["tests"])) {
      if (!tests_it.is_string()) {
        SPDLOG_WARN("Invalid test directory in manifest");
        return std::monostate();
      }
      auto io_str = tests_it.get<std::string>();
      auto abs_dir = fs->dir_of(io_str, manifest_dir);
      auto io = load_test(abs_dir, fs);
      if (io == nullptr) {
        SPDLOG_WARN("Failed to load test from directory {}", abs_dir);
        return std::monostate();
      }
      figure->add_test(io);
    }
  }

  auto substitutions = std::map<std::string, std::string>{{"ch", chapter_name}, {"fig", figure_name}};

  // Add elements
  std::optional<std::string> _default = std::nullopt;
  if (!manifest.contains("fragments") || !manifest["fragments"].is_array()) {
    SPDLOG_WARN("Manifest missing required fragments array");
    return std::monostate();
  }
  for (const auto &frag_it : std::as_const(manifest["fragments"])) {
    nlohmann::json templatized = frag_it;
    templateize(templatized, substitutions);
    auto item = load_fragment(templatized, manifest_dir, &*figure, fs, registry);
    if (item == nullptr) {
      SPDLOG_WARN("Failed to load element {}", str_of(templatized, "name"));
      continue;
    }
    item->figure = figure;
    figure->add_fragment(item);
    if (item->isDefault && !_default.has_value()) _default = item->name;
  }
  figure->set_default_fragment_name(_default.value_or("pepo"));

  return figure;
}

// TODO: remove this function, which was inlined from the old Pep/10 assembler.
// Analyze a macro's text body, and attempt to extract header information.
// Tuple returns 1) is the header well formed, 2) what is the macro's name,
// 3) how many arguments does the macro require?
std::tuple<bool, std::string, u8> analyze_macro_definition(std::string macro_text) {
  static const std::regex macrodecl(R"(^[ \t]*@([a-zA-Z][a-zA-Z0-9_]*)[ \t]+([0-9]|1[0-6])[ \t]*$)");
  /*
   * A macro file must begin with with name of the macro, followed by an
   * arbitrary number of spaces followed by an integer in [0,16] specifying the
   * number of arguments the macro takes.
   *
   * Neither comments nor whitespace may occur before this definition line.
   *
   * Below are valid examples:
   * @DECI 2
   * @UNOP 0
   *
   * Below are invalid examples, with comments descrbing why.
   *
   * Whitepace preceding macro definiton:
   *      @DECI 2
   * Space between macro name and macro symbol @.
   * @ deci 2
   *
   * Line ends in a comment
   * @deci 2 ;My comment
   *
   */

  // First line: up to the first '\n', or the whole string if none.
  auto nl = macro_text.find('\n');
  std::string first_line = (nl == std::string::npos) ? macro_text : macro_text.substr(0, nl);

  // Check if the first line matches the macro declaration regex. If so, extract the name and arg count.
  std::smatch match;
  if (!std::regex_match(first_line, match, macrodecl)) return {false, std::string(), 0};
  std::string name = match[1].str();
  std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::toupper(c); });
  // The regex already constrained group 2 to 0..16, so this can't fail/overflow.
  int arg_count = std::stoi(match[2].str());
  return {true, name, static_cast<std::uint8_t>(arg_count)};
}

std::variant<std::monostate, _Figure, _Macro> load_macro(const nlohmann::json &manifest, const std::string &path,
                                                         const pepp::BuiltinRegistry::FilesystemProvider *fs) {
  _Macro ret;
  auto manifest_dir = fs->dir_of(path);
  if (!manifest.contains("fragments") || !manifest["fragments"].is_object()) {
    SPDLOG_WARN("Manifest missing required fragments object");
    return std::monostate();
  }

  // Add elements
  for (const auto &[key, value] : manifest["fragments"].items()) {
    if (!value.is_string()) {
      SPDLOG_WARN("Fragment name must be a string");
      continue;
    }

    // Perform templatization on manifest values.
    const auto substitutions = std::map<std::string, std::string>{{"name", key}};
    const auto path = templatize(value.get<std::string>(), substitutions);

    // Load the macro
    const auto macro_text = fs->read_file(path, manifest_dir);
    auto nl = macro_text.find('\n');
    const auto macro_body = (nl == std::string::npos) ? std::string{} : macro_text.substr(nl + 1);
    // TODO: extracting the macro body this way is incorrect -- we will actually need to run the macro through the first
    // assembler pass to extract the inline macros!
    auto parsed = analyze_macro_definition(macro_text);
    const bool is_hidden = bool_of(manifest, "isHidden", false);
    const auto family = str_of(manifest, "family");
    const auto arch_str = str_of(manifest, "arch");
    const auto arch = arch_from_str(arch_str).value_or(pepp::Architecture::NO_ARCH);

    if (!std::get<0>(parsed)) {
      SPDLOG_WARN("Invalid macro definition in file {}", path);
      return {};
    }
    auto macro = std::make_shared<pepp::MacroFile>(std::get<1>(parsed), macro_body, std::get<2>(parsed), family, arch,
                                                   is_hidden);

    ret.push_back(macro);
  }
  return ret;
}

std::variant<std::monostate, _Figure, _Macro> load_manifest(const nlohmann::json &manifest, const std::string &path,
                                                            const pepp::BuiltinRegistry::FilesystemProvider *fs,
                                                            pepp::BuiltinRegistry *registry) {
  const auto type = str_of(manifest, "type");
  if (type == "figure" || type == "problem") return load_figure(manifest, path, fs, registry);
  else if (type == "macro") return load_macro(manifest, path, fs);
  else return std::monostate();
}

} // namespace
pepp::BuiltinRegistry::BuiltinRegistry(std::unique_ptr<FilesystemProvider> fs) : _fs(std::move(fs)) {
  for (const auto &book_path : _fs->enumerate_files("")) {
    auto book = load_book(book_path);
    if (book == nullptr) spdlog::warn("Failed to load book at {}", book_path);
    else if (find_book(book->name()) != nullptr) spdlog::critical("Duplicate book");
    else _books.push_back(book);
  }
}

std::list<std::shared_ptr<const pepp::Book>> pepp::BuiltinRegistry::books() const { return _books; }

std::shared_ptr<const pepp::Book> pepp::BuiltinRegistry::find_book(std::string name) const {
  u64 count = 0;
  std::shared_ptr<const pepp::Book> result = nullptr;
  for (const auto &bookPtr : _books) {
    if (bookPtr->name() == name) {
      result = bookPtr;
      count++;
    }
  }
  if (count > 1) {
    spdlog::warn("More than one copy of book {} found", name);
    return nullptr;
  } else return result;
}


void pepp::BuiltinRegistry::add_dependency(const Fragment *dependent, const Fragment *dependee) {
  _dependencies[dependent] = dependee;
  if (!_dependees.contains(dependee)) _dependees[dependee] = {};
  _dependees[dependee].push_back(dependent);
}

std::string pepp::BuiltinRegistry::content_for(Fragment &fragment) {
  // If the element has already been computed, use that value.
  if (_contents.contains(&fragment)) return _contents[&fragment];

  // Otherwise find the element that we depend on, and assemble it.
  auto dependee = value_or(_dependencies, &fragment, nullptr);
  if (dependee == nullptr) return _contents[&fragment] = "";
  else compute_dependencies(dependee);
  return _contents[&fragment];
}

void pepp::BuiltinRegistry::add_assembler(Architecture arch, std::unique_ptr<Assembler> &&assembler) {
  _assemblers[arch] = std::move(assembler);
}

void pepp::BuiltinRegistry::add_formatter(Architecture arch, std::string format,
                                          std::unique_ptr<Formatter> &&formatter) {
  _formatters[std::make_pair(arch, format)] = std::move(formatter);
}

void pepp::BuiltinRegistry::compute_dependencies(const Fragment *dependee) {
  // Compute dependee's value using correct assembler/compiler toolchain.
  if (!_dependees.contains(dependee)) return;

  auto dependents = _dependees.at(dependee);
  if (dependents.empty()) return;
  auto figure = dependee->figure.lock();
  if (figure == nullptr) {
    SPDLOG_WARN("Dependee %s has no figure", dependee->name);
    return;
  }
  auto assembler = _assemblers.find(figure->arch());
  if (assembler == _assemblers.end()) {
    const auto arch = pepp::arch_as_string(dependee->figure.lock()->arch());
    SPDLOG_WARN("No assembler for architecture {}", arch);
    return;
  }
  auto os = figure->default_os();
  auto os_text = os->default_fragment_text();
  auto assembled = (*assembler).second->operator()(os_text, dependee->contents());
  // For each dependent of the thing being assembled, compute the desired output.
  // This avoids needing to assemble the same dependee multiple times.
  for (const auto &dependent : std::as_const(dependents)) {
    auto figure = dependent->figure.lock();
    if (figure == nullptr) {
      SPDLOG_WARN("Dependent {} has no figure", dependent->name);
      _contents[dependent] = "";
      continue;
    }
    auto formatter = _formatters.find({figure->arch(), dependent->language});
    if (formatter == _formatters.end()) {
      const auto arch = pepp::arch_as_string(dependee->figure.lock()->arch());
      SPDLOG_WARN("No formatter for architecture {} and format {}", arch, dependent->language);

      _contents[dependent] = "";
      continue;
    }
    if (!_contents.contains(dependent)) _contents[dependent] = (*formatter).second->operator()(assembled);
  }
}

std::shared_ptr<pepp::Book> pepp::BuiltinRegistry::load_book(const std::string &toc_path) {
  static const auto bookNameKey = "bookName";
  // Read ToC to get book name
  auto toc_bytes = _fs->read_file(toc_path);
  nlohmann::json toc;
  try {
    toc = nlohmann::json::parse(toc_bytes);
  } catch (const nlohmann::json::parse_error &e) {
    SPDLOG_WARN("Failed to parse ToC at {}: {}", toc_path, e.what());
    return nullptr;
  }

  if (!toc.contains(bookNameKey)) return nullptr;
  // And create a book object to stick figures in
  auto book = std::make_shared<Book>(toc[bookNameKey].get<std::string>());

  // Explore the book's subdirectories, looking for figures and macros
  std::deque<std::string> to_visit;
  // Any time we visit a directory, scan that directory's children for more directories and manifest files.
  to_visit.push_back(toc_path);

  // Maintain a list of figures that need to be linked to their default OS
  std::list<std::tuple<std::string, std::shared_ptr<Figure>>> revisit;
  while (!to_visit.empty()) {
    auto next = to_visit.front();
    to_visit.pop_front();

    if (auto children = _fs->enumerate_files(next); !children.empty()) {
      for (const auto &child : children) to_visit.push_back(child);
    } else if (next.ends_with("manifest.json")) {
      auto manifest_bytes = _fs->read_file(next);
      auto manifest = nlohmann::json::parse(manifest_bytes);
      if (auto v = int_of(manifest, "version", 0); v == 2) {
        auto item = load_manifest(manifest, next, _fs.get(), this);
        if (std::holds_alternative<std::monostate>(item)) {
          SPDLOG_WARN("Failed to load manifest {} for {}", next, book->name());
        } else if (std::holds_alternative<_Figure>(item)) {
          auto figure = std::get<_Figure>(item);
          revisit.push_back({next, figure});
          if (figure->is_problem()) book->add_problem(figure);
          else book->add_figure(figure);
        } else if (std::holds_alternative<_Macro>(item)) {
          auto macros = std::get<_Macro>(item);
          for (auto &macro : macros) {
            if (macro == nullptr) SPDLOG_WARN("Failed to load macro {} for {}", next, book->name());
            else book->add_macro(macro);
          }
        }
      } else SPDLOG_WARN("Unknown manifest version {} for {}", v, next);
    }
  }

  // Revist all figures and attempt to link to default OS
  for (auto &[path, figure] : revisit) link_figure_to_OS(path, figure, book);

  return book;
}

void pepp::BuiltinRegistry::link_figure_to_OS(const std::string &manifest_path, std::shared_ptr<pepp::Figure> figure,
                                              std::shared_ptr<const pepp::Book> book) {
  // Read figure manifest to determine if figure is an OS, or if it links against an existing figure.
  auto manifest_bytes = _fs->read_file(manifest_path);
  nlohmann::json manifest;
  try {
    manifest = nlohmann::json::parse(manifest_bytes);
  } catch (const nlohmann::json::parse_error &e) {
    SPDLOG_WARN("Failed to parse manifest at {}: {}", manifest_path, e.what());
    return;
  }

  if (!manifest.contains("is_os") || !manifest["is_os"].get<bool>()) return;
  std::string ch_fig = str_of(manifest, "default_os");
  // Some figures (like microocde) don't use an OS, so it's not an error for this to be missing
  if (ch_fig.empty()) return;
  // If the key is present, it must be in the expected format
  auto split = ch_fig_from_str(ch_fig);
  if (!split.has_value()) throw std::invalid_argument("Invalid OS figure name in manifest " + manifest_path);
  const auto [os_ch, os_fig] = *ch_fig_from_str(ch_fig);
  auto os = book->find_figure(os_ch, os_fig);
  if (!os) spdlog::warn("Could not find OS for {} in book {}", ch_fig, book->name());
  figure->set_default_os(os.get());
}