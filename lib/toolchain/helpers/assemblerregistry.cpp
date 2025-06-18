#include "assemblerregistry.hpp"
#include "settings/settings.hpp"
#include "toolchain/helpers/asmb.hpp"
#include "toolchain/pas/operations/pepp/string.hpp"

struct Pep10Assembler : public builtins::Registry::Assembler {
  Pep10Assembler(const builtins::Registry *registry) : _registry(registry) {}
  QVariant operator()(const QString &os, const QString &user) override {
    helpers::AsmHelper assembler(helpers::cs6e_macros(_registry), os, pepp::Architecture::PEP10);
    assembler.setUserText(user);
    if (!assembler.assemble()) {
      qWarning("Failed to assemble!!");
      return {};
    }
    auto root = assembler.userRoot();
    if (root.isNull()) {
      qWarning("Assembler returned a null user root");
      return {};
    }
    return QVariant::fromValue(root);
  }

private:
  const builtins::Registry *_registry;
};
struct Pep10HFormatter : public builtins::Registry::Formatter {
  QString operator()(QVariant assembled) override {
    if (!assembled.canConvert<QSharedPointer<const pas::ast::Node>>()) {
      qWarning("Unexpected variant type");
      return "";
    }
    auto node = assembled.value<QSharedPointer<const pas::ast::Node>>();
    auto listing = pas::ops::pepp::formatHexListing<isa::Pep10>(*node);
    return listing.join("\n");
  }
};
struct Pep10BFormatter : public builtins::Registry::Formatter {
  QString operator()(QVariant assembled) override {
    if (!assembled.canConvert<QSharedPointer<const pas::ast::Node>>()) {
      qWarning("Unexpected variant type");
      return "";
    }
    auto node = assembled.value<QSharedPointer<const pas::ast::Node>>();
    auto listing = pas::ops::pepp::formatBinListing<isa::Pep10>(*node);
    return listing.join("\n");
  }
};
struct Pep10LFormatter : public builtins::Registry::Formatter {
  QString operator()(QVariant assembled) override {
    if (!assembled.canConvert<QSharedPointer<const pas::ast::Node>>()) {
      qWarning("Unexpected variant type");
      return "";
    }
    auto node = assembled.value<QSharedPointer<const pas::ast::Node>>();
    auto listing = pas::ops::pepp::formatListing<isa::Pep10>(*node);
    return listing.join("\n");
  }
};
struct Pep10OFormatter : public builtins::Registry::Formatter {
  QString operator()(QVariant assembled) override {
    if (!assembled.canConvert<QSharedPointer<const pas::ast::Node>>()) {
      qWarning("Unexpected variant type");
      return "";
    }
    auto node = assembled.value<QSharedPointer<const pas::ast::Node>>();
    auto bytes = pas::ops::pepp::toBytes<isa::Pep10>(*node);
    return pas::ops::pepp::bytesToObject(bytes, 16, false);
  }
};

QSharedPointer<builtins::Registry> helpers::builtins_registry(bool use_app_settings, QString directory) {
  using R = QSharedPointer<builtins::Registry>;
  if (use_app_settings) return R::create(pepp::settings::AppSettings().general()->figureDirectory());
  else return R::create(directory);
}

QSharedPointer<builtins::Registry> helpers::registry_with_assemblers(QString directory) {
  auto registry = builtins_registry(true, directory);
  registry->addAssembler(pepp::Architecture::PEP10, std::make_unique<Pep10Assembler>(&*registry));
  registry->addFormatter(pepp::Architecture::PEP10, "peph", std::make_unique<Pep10HFormatter>());
  registry->addFormatter(pepp::Architecture::PEP10, "pepb", std::make_unique<Pep10BFormatter>());
  registry->addFormatter(pepp::Architecture::PEP10, "pepl", std::make_unique<Pep10LFormatter>());
  registry->addFormatter(pepp::Architecture::PEP10, "pepo", std::make_unique<Pep10OFormatter>());
  return registry;
}

QSharedPointer<const builtins::Book> helpers::book(int ed, const builtins::Registry *reg) {
  QString bookName;
  switch (ed) {
  case 4: bookName = "Computer Systems, 4th Edition"; break;
  case 5: bookName = "Computer Systems, 5th Edition"; break;
  case 6: bookName = "Computer Systems, 6th Edition"; break;
  default: return nullptr;
  }

  auto book = reg->findBook(bookName);
  return book;
}

QSharedPointer<macro::Registry> helpers::cs5e_macros(const builtins::Registry *reg) {
  auto book = helpers::book(5, reg);
  return helpers::registry(book, {});
}

QSharedPointer<macro::Registry> helpers::cs6e_macros(const builtins::Registry *reg) {
  auto book = helpers::book(6, reg);
  return helpers::registry(book, {});
}
