#include "builtin_registry.hpp"
#include <spdlog/spdlog.h>
#include "core/resources/figures/book.hpp"
#include "core/resources/figures/figure.hpp"
#include "core/resources/figures/fragment.hpp"

pepp::BuiltinRegistry::BuiltinRegistry(std::string directory) {
  _usingExternalFigures = (directory != pepp::default_builtins_path);
  for (const auto &book_path : detail::enumerate_books(directory)) {
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

template <class Map, class K, class V> Map::mapped_type value_or(const Map &m, const K &key, V fallback) {
  auto it = m.find(key);
  return it != m.end() ? it->second : fallback;
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

namespace {

int absolute_index(std::vector<int> &list, int index, int _default = 0) {
  if (index >= 0 && index < list.size()) return index;
  else if (index < 0 && index >= -list.size()) return list.size() + index;
  else return _default;
}

std::string_view selectLines(const std::string &input, int startLine, int endLine) {
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

std::string templatize(std::string input, std::map<std::string, std::string> substitutions) {

  for (const auto &[key, value] : substitutions) {
    const std::string token = "{" + key + "}";
    for (std::size_t pos = input.find(token); pos != std::string::npos; pos = input.find(token, pos + value.size()))
      input.replace(pos, token.size(), value);
  }
  return input;
}

void templateize(void *&object, std::map<std::string, std::string> substitutions) {
  /*
  auto keys = object.keys();
  for (const auto &key : std::as_const(keys)) {
    auto value = object[key];
    if (value.isObject()) {
      auto innerObject = value.toObject();
      templateize(innerObject, substitutions);
      object[key] = innerObject;
    } else if (value.isString()) {
      object[key] = templatize(value.toString(), substitutions);
    }
  }*/
}

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

pepp::Fragment *loadFragment(const void *&item, const std::string &manifest_dir, pepp::Figure *parent,
                             pepp::BuiltinRegistry *registry) {
  // Use smart pointer to avoid cleanup on error paths.
  /*
  auto fragment = std::make_unique<builtins::Fragment>();
  fragment->name = item["name"].toString("");
  if (!item["format"].isString()) {
    qWarning("Invalid fragment format for %s", fragment->name.toStdString().c_str());
    return nullptr;
  } else fragment->language = item["format"].toString();

  fragment->isHidden = item["hidden"].toBool(false);
  if (auto isDefault = item["isDefault"].toBool(false); isDefault && fragment->isHidden) {
    qWarning("isDefault is incompatible with isHidden for %s", fragment->name.toStdString().c_str());
    return nullptr;
  } else if (isDefault && fragment->name.isEmpty()) {
    qWarning("Default fragment must be named %s", fragment->name.toStdString().c_str());
    return nullptr;
  } else fragment->isDefault = isDefault;

  fragment->copyType = item["copyType"].toString("");
  fragment->exportPath = item["export"].toString("");

  if (!item["from"].isObject()) {
    qWarning("from element must be an object %s", fragment->name.toStdString().c_str());
    return nullptr;
  } else if (auto from = item["from"].toObject(); true) {
    // Require one of "element" or "file" to be present.
    if (from["file"].isString()) {
      auto absPath = manifestDir.absoluteFilePath(from["file"].toString());
      fragment->contentsFn = [=]() { return loadFromFile(absPath); };
    } else if (from["element"].isString()) {
      auto dependee = parent->findFragment(from["element"].toString());
      if (dependee == nullptr) {
        qWarning("Fragment %s not found in figure %s", from["element"].toString().toStdString().c_str(),
                 parent->figureName().toStdString().c_str());
        return nullptr;
      }

      registry->addDependency(fragment.get(), dependee);
      auto elementPtr = fragment.get();
      fragment->contentsFn = [registry, elementPtr]() { return registry->contentFor(*elementPtr); };
    } else {
      qWarning("Did not specify a valid source");
      return nullptr;
    }

    // Extract a subset of lines from the file.
    if (from["lines"].isArray()) {
      auto contentsFn = fragment->contentsFn;
      if (auto lines = from["lines"].toArray(); !lines[0].isDouble() || !lines[1].isDouble()) {
        qWarning("Invalid line numbers for %s", fragment->name.toStdString().c_str());
        return nullptr;
      } else {
        auto startLine = lines[0].toInt(0), endLine = lines[1].toInt(-1);
        fragment->contentsFn = [=]() {
          auto contents = contentsFn();
          return selectLines(contents, startLine, endLine);
        };
      }
    }
  }

  return fragment.release();
*/
  return nullptr;
}
} // namespace

std::variant<std::monostate, pepp::BuiltinRegistry::_Figure, pepp::BuiltinRegistry::_Macro>
pepp::BuiltinRegistry::load_manifest(const void *&manifest, const std::string &path) {
  // const auto type = manifest["type"].toString("").toLower();
  const auto type = "placeholder";
  if (type == "figure" || type == "problem") return load_figure(manifest, path);
  else if (type == "macro") return load_macro(manifest, path);
  else return std::monostate();
}

std::variant<std::monostate, pepp::BuiltinRegistry::_Figure, pepp::BuiltinRegistry::_Macro>
pepp::BuiltinRegistry::load_figure(const void *&manifest, const std::string &path) {
  throw std::runtime_error("BuiltinRegistry::load_figure not implemented");
}

std::variant<std::monostate, pepp::BuiltinRegistry::_Figure, pepp::BuiltinRegistry::_Macro>
pepp::BuiltinRegistry::load_macro(const void *&manifest, const std::string &path) {
  throw std::runtime_error("BuiltinRegistry::load_macro not implemented");
}

std::shared_ptr<pepp::Book> pepp::BuiltinRegistry::load_book(const std::string &toc_path) {
  throw std::runtime_error("BuiltinRegistry::load_book not implemented");
}

pepp::Test *pepp::detail::load_test(const std::string &test_dir) {
  throw std::runtime_error("detail::load_test not implemented");
}

void pepp::detail::link_figure_to_OS(const std::string &manifest_path, std::shared_ptr<Figure> figure,
                                     std::shared_ptr<const Book> book) {
  throw std::runtime_error("detail::link_figure_to_OS not implemented");
}

std::list<std::string> pepp::detail::enumerate_books(const std::string &prefix) {
  throw std::runtime_error("detail::enumerate_books not implemented");
}
