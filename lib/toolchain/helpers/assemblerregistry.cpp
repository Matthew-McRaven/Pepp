#include "assemblerregistry.hpp"
#include "settings/settings.hpp"
#include "toolchain/helpers/asmb.hpp"
#include "toolchain/pas/operations/pepp/string.hpp"

struct PepAssembler : public builtins::Registry::Assembler {
  PepAssembler(const builtins::Registry *registry, pepp::Architecture arch) : _registry(registry), _arch(arch) {}
  QVariant operator()(const QString &os, const QString &user) override {
    QSharedPointer<macro::Registry> macros;
    if (_arch == pepp::Architecture::PEP10) macros = helpers::cs6e_macros(_registry);
    else if (_arch == pepp::Architecture::PEP9) macros = helpers::cs5e_macros(_registry);
    else {
      qWarning("Unsupported architecture for assembler");
      return {};
    }
    helpers::AsmHelper assembler(macros, os, _arch);
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
  pepp::Architecture _arch;
};

template <typename ISA> QString formatH(QVariant assembled) {
  if (!assembled.canConvert<QSharedPointer<const pas::ast::Node>>()) {
    qWarning("Unexpected variant type");
    return "";
  }
  auto node = assembled.value<QSharedPointer<const pas::ast::Node>>();
  auto listing = pas::ops::pepp::formatHexListing<ISA>(*node);
  return listing.join("\n");
}
template <typename ISA> QString formatB(QVariant assembled) {
  if (!assembled.canConvert<QSharedPointer<const pas::ast::Node>>()) {
    qWarning("Unexpected variant type");
    return "";
  }
  auto node = assembled.value<QSharedPointer<const pas::ast::Node>>();
  auto listing = pas::ops::pepp::formatBinListing<ISA>(*node);
  return listing.join("\n");
}
template <typename ISA> QString formatL(QVariant assembled) {
  if (!assembled.canConvert<QSharedPointer<const pas::ast::Node>>()) {
    qWarning("Unexpected variant type");
    return "";
  }
  auto node = assembled.value<QSharedPointer<const pas::ast::Node>>();
  auto listing = pas::ops::pepp::formatListing<ISA>(*node);
  return listing.join("\n");
}

template <typename ISA> QString formatO(QVariant assembled) {
  if (!assembled.canConvert<QSharedPointer<const pas::ast::Node>>()) {
    qWarning("Unexpected variant type");
    return "";
  }
  auto node = assembled.value<QSharedPointer<const pas::ast::Node>>();
  auto bytes = pas::ops::pepp::toBytes<ISA>(*node);
  const bool isPep10 = std::is_same_v<ISA, isa::Pep10>;
  return pas::ops::pepp::bytesToObject(bytes, 16, !isPep10);
}

struct Pep10HFormatter : public builtins::Registry::Formatter {
  QString operator()(QVariant assembled) override { return formatH<isa::Pep10>(assembled); }
};
struct Pep10BFormatter : public builtins::Registry::Formatter {
  QString operator()(QVariant assembled) override { return formatB<isa::Pep10>(assembled); }
};
struct Pep10LFormatter : public builtins::Registry::Formatter {
  QString operator()(QVariant assembled) override { return formatL<isa::Pep10>(assembled); }
};
struct Pep10OFormatter : public builtins::Registry::Formatter {
  QString operator()(QVariant assembled) override { return formatO<isa::Pep10>(assembled); }
};

struct Pep9HFormatter : public builtins::Registry::Formatter {
  QString operator()(QVariant assembled) override { return formatH<isa::Pep9>(assembled); }
};
struct Pep9BFormatter : public builtins::Registry::Formatter {
  QString operator()(QVariant assembled) override { return formatB<isa::Pep9>(assembled); }
};
struct Pep9LFormatter : public builtins::Registry::Formatter {
  QString operator()(QVariant assembled) override { return formatL<isa::Pep9>(assembled); }
};
struct Pep9OFormatter : public builtins::Registry::Formatter {
  QString operator()(QVariant assembled) override { return formatO<isa::Pep9>(assembled); }
};

QSharedPointer<builtins::Registry> helpers::builtins_registry(bool use_app_settings, QString directory) {
  using R = QSharedPointer<builtins::Registry>;
  if (use_app_settings) return R::create(pepp::settings::AppSettings().general()->figureDirectory());
  else return R::create(directory);
}

QSharedPointer<builtins::Registry> helpers::registry_with_assemblers(QString directory) {
  auto registry = builtins_registry(true, directory);
  registry->addAssembler(pepp::Architecture::PEP10,
                         std::make_unique<PepAssembler>(&*registry, pepp::Architecture::PEP10));
  registry->addFormatter(pepp::Architecture::PEP10, "peph", std::make_unique<Pep10HFormatter>());
  registry->addFormatter(pepp::Architecture::PEP10, "pepb", std::make_unique<Pep10BFormatter>());
  registry->addFormatter(pepp::Architecture::PEP10, "pepl", std::make_unique<Pep10LFormatter>());
  registry->addFormatter(pepp::Architecture::PEP10, "pepo", std::make_unique<Pep10OFormatter>());
  registry->addAssembler(pepp::Architecture::PEP9,
                         std::make_unique<PepAssembler>(&*registry, pepp::Architecture::PEP9));
  registry->addFormatter(pepp::Architecture::PEP9, "peph", std::make_unique<Pep9HFormatter>());
  registry->addFormatter(pepp::Architecture::PEP9, "pepb", std::make_unique<Pep9BFormatter>());
  registry->addFormatter(pepp::Architecture::PEP9, "pepl", std::make_unique<Pep9LFormatter>());
  registry->addFormatter(pepp::Architecture::PEP9, "pepo", std::make_unique<Pep9OFormatter>());
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
