#include "./ast_from_parse_tree.hpp"

#include "pat/ast/node/directive_align.hpp"
#include "pat/ast/node/directive_bytes.hpp"
#include "pat/ast/node/directive_set.hpp"
#include "pat/ast/node/directive_skip.hpp"
#include "pat/ast/node/directive_string.hpp"
#include "pat/pep/ast/node/directive_burn.hpp"
#include "pat/pep/ast/node/directive_end.hpp"
#include "pat/pep/ast/node/directive_external.hpp"
#include "pat/pep/ast/node/directive_io.hpp"
#include "pat/pep/ast/node/directive_system_call.hpp"

QSharedPointer<pat::ast::node::Base>
pat::pep::ast::visitor::detail::align(const DirectiveType &line, ST symTab) {
  return single_argument_gen<pat::ast::node::Align>(line, symTab);
}

QSharedPointer<pat::ast::node::Base>
pat::pep::ast::visitor::detail::ascii(const DirectiveType &line, ST symTab) {
  return single_argument_gen<pat::ast::node::ASCII>(line, symTab);
}

QSharedPointer<pat::ast::node::Base>
pat::pep::ast::visitor::detail::block(const DirectiveType &line, ST symTab) {
  return single_argument_gen<pat::ast::node::Skip>(line, symTab);
}

QSharedPointer<pat::ast::node::Base>
pat::pep::ast::visitor::detail::burn(const DirectiveType &line, ST symTab) {
  using ::pat::ast::node::Base;
  using ::pat::ast::node::FileLocation;
  auto visitor = visitor::ParseToArg();
  visitor.symTab = symTab;
  if (auto count = line.args.size(); count != 1)
    throw std::logic_error("Requires 1 arg");
  else {
    auto shared = line.args[0].apply_visitor(visitor);
    auto casted = qSharedPointerCast<pat::ast::argument::Hexadecimal>(shared);
    if (visitor.error)
      throw std::logic_error("error");
    else if (casted == nullptr)
      throw std::logic_error("invalid argument type");
    if (auto result = pat::pep::ast::node::Burn::validate_argument(casted);
        result.valid) {
      auto ret = QSharedPointer<pat::pep::ast::node::Burn>::create(
          casted, FileLocation(), QWeakPointer<Base>());
      if (line.hasComment)
        ret->setComment(QString::fromStdString(line.comment));
      if (line.symbol.empty())
        throw std::logic_error("burn can't have a symbol");
      return ret;
    } else
      throw std::logic_error("error");
  };
}

QSharedPointer<pat::ast::node::Base>
pat::pep::ast::visitor::detail::byte(const DirectiveType &line, ST symTab) {
  return bytes_gen<pat::ast::node::Byte1>(line, symTab);
}

QSharedPointer<pat::ast::node::Base>
pat::pep::ast::visitor::detail::end(const DirectiveType &line, ST symTab) {
  using ::pat::ast::node::Base;
  using ::pat::ast::node::FileLocation;
  if (auto count = line.args.size(); count != 0)
    throw std::logic_error("Requires 0 arg");
  else {
    auto ret = QSharedPointer<pat::pep::ast::node::End>::create(
        false, FileLocation(), QWeakPointer<Base>());
    if (line.hasComment)
      ret->setComment(QString::fromStdString(line.comment));
    if (line.symbol.empty())
      throw std::logic_error("end can't have a symbol");
    else
      return ret;
  }
};

QSharedPointer<pat::ast::node::Base>
pat::pep::ast::visitor::detail::equate(const DirectiveType &line, ST symTab) {
  auto ret = single_argument_gen<pat::ast::node::Set>(line, symTab);
  auto casted = qSharedPointerCast<pat::ast::node::Set>(ret);
  if (casted && casted->symbol().isNull())
    throw std::logic_error("equate needs symbol");
  else
    return ret;
}

QSharedPointer<pat::ast::node::Base>
pat::pep::ast::visitor::detail::_export(const DirectiveType &line, ST symTab) {
  return io_or_scall_gen<pat::pep::ast::node::Export>(line, symTab);
}

QSharedPointer<pat::ast::node::Base>
pat::pep::ast::visitor::detail::import(const DirectiveType &line, ST symTab) {
  return io_or_scall_gen<pat::pep::ast::node::Import>(line, symTab);
}

QSharedPointer<pat::ast::node::Base>
pat::pep::ast::visitor::detail::input(const DirectiveType &line, ST symTab) {
  return io_or_scall_gen<pat::pep::ast::node::Input>(line, symTab);
}

QSharedPointer<pat::ast::node::Base>
pat::pep::ast::visitor::detail::output(const DirectiveType &line, ST symTab) {
  return io_or_scall_gen<pat::pep::ast::node::Output>(line, symTab);
}

QSharedPointer<pat::ast::node::Base>
pat::pep::ast::visitor::detail::scall(const DirectiveType &line, ST symTab) {
  return io_or_scall_gen<pat::pep::ast::node::SCall>(line, symTab);
}

QSharedPointer<pat::ast::node::Base>
pat::pep::ast::visitor::detail::section(const DirectiveType &line, ST symTab) {
  throw std::logic_error("Unimplemented");
}

QSharedPointer<pat::ast::node::Base>
pat::pep::ast::visitor::detail::uscall(const DirectiveType &line, ST symTab) {
  return io_or_scall_gen<pat::pep::ast::node::USCall>(line, symTab);
}

QSharedPointer<pat::ast::node::Base>
pat::pep::ast::visitor::detail::word(const DirectiveType &line, ST symTab) {
  return bytes_gen<pat::ast::node::Byte2>(line, symTab);
}
