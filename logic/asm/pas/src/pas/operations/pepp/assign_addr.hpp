#pragma once
#include "pas/ast/value/symbolic.hpp"
#include "symbol/table.hpp"
#include "symbol/visit.hpp"
#include "pas/ast/op.hpp"
#include "pas/operations/pepp/size.hpp"
#include "pas/ast/generic/attr_directive.hpp"
#include <QtCore>

namespace pas::ast {
class Node;
} // namespace pas::ast

namespace pas::ops::pepp {

namespace detail {
// TODO: extend to handle sections
template <typename ISA>
void assignAddressesImpl(ast::Node &node, quint16 &start,
                         Direction direction = Direction::Forward);
bool hasBurn(QList<QSharedPointer<ast::Node>> &list);
quint16 getBurnArg(QList<QSharedPointer<ast::Node>> &list);
} // namespace detail
// Only works if the given a single section.
template <typename ISA> void assignAddresses(ast::Node &section);
} // namespace pas::ops::pepp

template <typename ISA>
void pas::ops::pepp::detail::assignAddressesImpl(ast::Node &node, quint16 &base,
                                                 Direction direction) {
  auto size = pepp::explicitSize<ISA>(node, base, direction);
  auto symBase = base;
  quint16 newBase = base;
  // Skip over nodes where addresses make no sense
  auto type = pas::ast::type(node).value;
  if(type == pas::ast::generic::Type::Blank || type == pas::ast::generic::Type::Comment) return;

  static const QSet<QString> addresslessDirectives = {u"END"_qs, u"EQUATE"_qs, u"EXPORT"_qs, u"IMPORT"_qs,
                                                      u"INPUT"_qs, u"OUTPT"_qs, u"SCALL"_qs, u"SECTION"_qs, u"USCALL"_qs};
  if(type == pas::ast::generic::Type::Directive && addresslessDirectives.contains(node.get<pas::ast::generic::Directive>().value)) return;
  /*quint16 alignSize = 0;
  if(type == pas::ast::generic::Type::Directive && node.get<ast::generic::Directive>().value == "ALIGN") {
    auto arg = node.get<ast::generic::ArgumentList>().value[0];
    arg->value((quint8*)(&alignSize), 2, pas::bits::hostOrder()) ;
  }
  if(type == pas::ast::generic::Type::Directive && node.get<ast::generic::Directive>().value == "ALIGN" && base % alignSize == 0){

  }*/
  else if (direction == Direction::Forward) {
    // Must explicitly handle address wrap-around, because math inside set
    // address widens implicitly.
    newBase = (base + size) % 0xFFFF;
    // size is 1-index, while base is 0-indexed. Offset by 1. Unless size is 0,
    // in which case no adjustment is necessary.
    ast::setAddress(node, base,  size);
    base = newBase;
  } else {
    newBase = (base - size) % 0xFFFF;
    // size is 1-index, while base is 0-indexed. Offset by 1. Unless size is 0,
    // in which case no adjustment is necessary.
    ast::setAddress(node, (newBase + (size > 0 ? 1 : 0)) % 0xFFFF, size);
    base = newBase;
    symBase = base;
  }

  auto isEquate = pas::ops::generic::isSet();
  isEquate.directiveAliases = {"EQUATE"};
  // EQUATEs don't use addresses, and must be handled differently
  if (node.has<ast::generic::Directive>() && isEquate(node)) {
    auto symbol = node.get<ast::generic::SymbolDeclaration>().value;
    auto argument = node.get<ast::generic::Argument>().value;
    // Check if the argument is a symbol, if so, determine if argument belongs
    // to a different tree (i.e., is global).
    if (auto symbolic = dynamic_cast<ast::value::Symbolic *>(&*argument);
        symbolic != nullptr) {
      auto other = symbolic->symbol();
      if (symbol::rootTable(other->parent.sharedFromThis()) ==
          symbol::rootTable(symbol->parent.sharedFromThis())) {
        symbol->value =
            QSharedPointer<symbol::value::InternalPointer>::create(other);
      } else {
        symbol->value = QSharedPointer<symbol::value::ExternalPointer>::create(
            other->parent.sharedFromThis(), other);
      }
    } else {
      auto bits = symbol::value::MaskedBits{
          .byteCount = 2, .bitPattern = 0, .mask = 0xFFFF};
      argument->value(reinterpret_cast<quint8 *>(&bits.bitPattern), 8,
                      bits::hostOrder());
      symbol->value = QSharedPointer<symbol::value::Constant>::create(bits);
    }
  } else if (node.has<ast::generic::SymbolDeclaration>()) {
    auto isCode = node.has<ast::pepp::Instruction<ISA>>();
    auto symbol = node.get<ast::generic::SymbolDeclaration>().value;
    symbol->value = QSharedPointer<symbol::value::Location>::create(
        2, symBase, 0, isCode ? symbol::Type::kCode : symbol::Type::kObject);
  }
}

template <typename ISA> void pas::ops::pepp::assignAddresses(ast::Node &root) {
  quint16 base = 0;
  if (root.has<ast::generic::Children>()) {
    auto children = root.get<ast::generic::Children>().value;
    if (detail::hasBurn(children)) {
      base = detail::getBurnArg(children);
      for (auto child = children.rbegin(); child != children.rend(); ++child)
        detail::assignAddressesImpl<ISA>(**child, base, Direction::Backward);
    } else {
      for (auto &child : children)
        detail::assignAddressesImpl<ISA>(*child, base, Direction::Forward);
    }
  }
}
