#include "assemblerregistry.hpp"
#include "settings/settings.hpp"
#include "toolchain/helpers/asmb.hpp"
#include "toolchain/pas/ast/generic/attr_keeepalive.hpp"
#include "toolchain/pas/operations/pepp/string.hpp"

struct PepAssembler : public pepp::BuiltinRegistry::Assembler {
  PepAssembler(const pepp::BuiltinRegistry *registry, pepp::Architecture arch) : _registry(registry), _arch(arch) {}
  std::any operator()(const std::string &os, const std::string &user) override {
    QSharedPointer<macro::Registry> macros;
    if (_arch == pepp::Architecture::PEP10) macros = helpers::cs6e_macros(_registry);
    else if (_arch == pepp::Architecture::PEP9) macros = helpers::cs5e_macros(_registry);
    else {
      qWarning("Unsupported architecture for assembler");
      return {};
    }
    // Arbitrary, small, non-zero value.
    if (os.length() <= 5) {
      qWarning("OS looks empty");
      return {};
    }
    helpers::AsmHelper assembler(macros, QString::fromStdString(os), _arch);
    assembler.setUserText(QString::fromStdString(user));
    if (!assembler.assemble()) {
      qWarning("Failed to assemble!!");
      return {};
    }
    auto _errors = assembler.errors();
    if (!_errors.isEmpty()) {
      qWarning("Assembler errors: %s", qPrintable(_errors.join("\n")));
      return {};
    }
    auto root = assembler.userRoot();
    if (root.isNull()) {
      qWarning("Assembler returned a null user root");
      return {};
    }
    return std::any{root};
  }

private:
  const pepp::BuiltinRegistry *_registry = nullptr;
  pepp::Architecture _arch;
};

template <typename ISA> std::string formatH(std::any assembled) {
  try {
    auto node = std::any_cast<QSharedPointer<const pas::ast::Node>>(assembled);
    auto listing = pas::ops::pepp::formatHexListing<ISA>(*node);
    return listing.join("\n").toStdString();
  } catch (const std::bad_any_cast &e) {
    qWarning("Unexpected variant type: %s", e.what());
    return "";
  }
}
template <typename ISA> std::string formatB(std::any assembled) {
  try {
    auto node = std::any_cast<QSharedPointer<const pas::ast::Node>>(assembled);
    auto listing = pas::ops::pepp::formatBinListing<ISA>(*node);
    return listing.join("\n").toStdString();
  } catch (const std::bad_any_cast &e) {
    qWarning("Unexpected variant type: %s", e.what());
    return "";
  }
}
template <typename ISA> std::string formatL(std::any assembled) {
  try {
    auto node = std::any_cast<QSharedPointer<const pas::ast::Node>>(assembled);
    auto listing = pas::ops::pepp::formatListing<ISA>(*node);
    return listing.join("\n").toStdString();
  } catch (const std::bad_any_cast &e) {
    qWarning("Unexpected variant type: %s", e.what());
    return "";
  }
}

template <typename ISA> std::string formatO(std::any assembled) {
  try {
    auto node = std::any_cast<QSharedPointer<const pas::ast::Node>>(assembled);
    auto bytes = pas::ops::pepp::toBytes<ISA>(*node);
    const bool isPep10 = std::is_same_v<ISA, isa::Pep10>;
    return pas::ops::pepp::bytesToObject(bytes, 16, !isPep10).toStdString();
  } catch (const std::bad_any_cast &e) {
    qWarning("Unexpected variant type: %s", e.what());
    return "";
  }
}

struct Pep10HFormatter : public pepp::BuiltinRegistry::Formatter {
  std::string operator()(std::any assembled) override { return formatH<isa::Pep10>(assembled); }
};
struct Pep10BFormatter : public pepp::BuiltinRegistry::Formatter {
  std::string operator()(std::any assembled) override { return formatB<isa::Pep10>(assembled); }
};
struct Pep10LFormatter : public pepp::BuiltinRegistry::Formatter {
  std::string operator()(std::any assembled) override { return formatL<isa::Pep10>(assembled); }
};
struct Pep10OFormatter : public pepp::BuiltinRegistry::Formatter {
  std::string operator()(std::any assembled) override { return formatO<isa::Pep10>(assembled); }
};

struct Pep9HFormatter : public pepp::BuiltinRegistry::Formatter {
  std::string operator()(std::any assembled) override { return formatH<isa::Pep9>(assembled); }
};
struct Pep9BFormatter : public pepp::BuiltinRegistry::Formatter {
  std::string operator()(std::any assembled) override { return formatB<isa::Pep9>(assembled); }
};
struct Pep9LFormatter : public pepp::BuiltinRegistry::Formatter {
  std::string operator()(std::any assembled) override { return formatL<isa::Pep9>(assembled); }
};
struct Pep9OFormatter : public pepp::BuiltinRegistry::Formatter {
  std::string operator()(std::any assembled) override { return formatO<isa::Pep9>(assembled); }
};

std::shared_ptr<pepp::BuiltinRegistry> helpers::builtins_registry(bool use_app_settings, QString directory) {
  using R = QSharedPointer<pepp::BuiltinRegistry>;
  const auto dir = use_app_settings ? pepp::settings::AppSettings().general()->figureDirectory() : directory;
  auto fs_provider = builtins::QtFilesystemProvider::create(dir);
  return std::make_shared<pepp::BuiltinRegistry>(std::move(fs_provider));
}

std::shared_ptr<pepp::BuiltinRegistry> helpers::registry_with_assemblers(bool use_app_settings, QString directory) {
  auto registry = builtins_registry(use_app_settings, directory);
  registry->add_assembler(pepp::Architecture::PEP10,
                          std::make_unique<PepAssembler>(&*registry, pepp::Architecture::PEP10));
  registry->add_formatter(pepp::Architecture::PEP10, "peph", std::make_unique<Pep10HFormatter>());
  registry->add_formatter(pepp::Architecture::PEP10, "pepb", std::make_unique<Pep10BFormatter>());
  registry->add_formatter(pepp::Architecture::PEP10, "pepl", std::make_unique<Pep10LFormatter>());
  registry->add_formatter(pepp::Architecture::PEP10, "pepo", std::make_unique<Pep10OFormatter>());
  registry->add_assembler(pepp::Architecture::PEP9,
                          std::make_unique<PepAssembler>(&*registry, pepp::Architecture::PEP9));
  registry->add_formatter(pepp::Architecture::PEP9, "peph", std::make_unique<Pep9HFormatter>());
  registry->add_formatter(pepp::Architecture::PEP9, "pepb", std::make_unique<Pep9BFormatter>());
  registry->add_formatter(pepp::Architecture::PEP9, "pepl", std::make_unique<Pep9LFormatter>());
  registry->add_formatter(pepp::Architecture::PEP9, "pepo", std::make_unique<Pep9OFormatter>());
  return registry;
}

std::shared_ptr<const pepp::Book> helpers::book(int ed, const pepp::BuiltinRegistry *reg) {
  std::string book_name;
  switch (ed) {
  case 4: book_name = "Computer Systems, 4th Edition"; break;
  case 5: book_name = "Computer Systems, 5th Edition"; break;
  case 6: book_name = "Computer Systems, 6th Edition"; break;
  default: return nullptr;
  }

  auto book = reg->find_book(book_name);
  return book;
}

QSharedPointer<macro::Registry> helpers::cs5e_macros(const pepp::BuiltinRegistry *reg) {
  auto book = helpers::book(5, reg);
  return helpers::registry(book, {});
}

QSharedPointer<macro::Registry> helpers::cs6e_macros(const pepp::BuiltinRegistry *reg) {
  auto book = helpers::book(6, reg);
  return helpers::registry(book, {});
}
