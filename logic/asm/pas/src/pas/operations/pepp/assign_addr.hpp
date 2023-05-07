#pragma once
#include "pas/ast/generic/attr_directive.hpp"
#include "pas/ast/generic/attr_sec.hpp"
#include "pas/ast/op.hpp"
#include "pas/ast/value/symbolic.hpp"
#include "pas/operations/pepp/size.hpp"
#include "symbol/table.hpp"
#include "symbol/visit.hpp"
#include <QtCore>
#include <pas/ast/generic/attr_hide.hpp>

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
// Handles multiple, nested sections.
template <typename ISA> void assignAddresses(ast::Node &root);
} // namespace pas::ops::pepp

template <typename ISA>
void pas::ops::pepp::detail::assignAddressesImpl(ast::Node &node, quint16 &base,
                                                 Direction direction) {
  auto size = pepp::explicitSize<ISA>(node, base, direction);
  auto symBase = base;
  quint16 newBase = base;
  // Skip over nodes where addresses make no sense
  auto type = pas::ast::type(node).value;
  if (type == pas::ast::generic::Type::Blank ||
      type == pas::ast::generic::Type::Comment)
    return;
  /*quint16 alignSize = 0;
  if(type == pas::ast::generic::Type::Directive &&
  node.get<ast::generic::Directive>().value == "ALIGN") { auto arg =
  node.get<ast::generic::ArgumentList>().value[0];
    arg->value((quint8*)(&alignSize), 2, pas::bits::hostOrder()) ;
  }
  if(type == pas::ast::generic::Type::Directive &&
  node.get<ast::generic::Directive>().value == "ALIGN" && base % alignSize ==
  0){

  }*/

  static const QSet<QString>
      addresslessDirectives =
          {
              u"END"_qs,    u"EXPORT"_qs, u"IMPORT"_qs,  u"INPUT"_qs,
              u"OUTPUT"_qs, u"SCALL"_qs,  u"SECTION"_qs, u"USCALL"_qs,
              u"SECTION"_qs}; // Don't skip ORG, because it updates the base
                              // address. Also do not skip EQUATE, because it
                              // modifies symbols.
  if (type == pas::ast::generic::Type::Directive &&
      addresslessDirectives.contains(
          node.get<pas::ast::generic::Directive>().value)) {
    pas::ast::generic::Hide hide;
    if (node.has<ast::generic::Hide>())
      hide = node.get<ast::generic::Hide>();
    hide.value.addressInListing = true;
    node.set(hide);
    return;
  }
  auto isEquate = pas::ops::generic::isSet();
  isEquate.directiveAliases = {"EQUATE"};
  if (generic::isOrg()(node)) {
    auto arg = node.get<ast::generic::Argument>().value;
    arg->value(reinterpret_cast<quint8 *>(&base), 2, bits::hostOrder());
    newBase = symBase = base;
  } else if (node.has<ast::generic::Directive>() && isEquate(node)) {
    // EQUATEs don't use addresses, and must be handled differently
    auto symbol = node.get<ast::generic::SymbolDeclaration>().value;
    auto argument = node.get<ast::generic::Argument>().value;
    // Check if the argument is a symbol, if so, determine if argument belongs
    // to a different tree (i.e., is global).
    if (auto symbolic = dynamic_cast<ast::value::Symbolic *>(&*argument);
        symbolic != nullptr) {
      auto other = symbolic->symbol();
      if (symbol::rootTable(other->parent.sharedFromThis()) ==
          symbol::rootTable(symbol->parent.sharedFromThis())) {
        symbol->value = QSharedPointer<symbol::value::InternalPointer>::create(
            sizeof(quint16), other);
      } else {
        symbol->value = QSharedPointer<symbol::value::ExternalPointer>::create(
            sizeof(quint16), other->parent.sharedFromThis(), other);
      }
    } else {
      auto bits = symbol::value::MaskedBits{
          .byteCount = 2, .bitPattern = 0, .mask = 0xFFFF};
      argument->value(reinterpret_cast<quint8 *>(&bits.bitPattern), 8,
                      bits::hostOrder());
      symbol->value = QSharedPointer<symbol::value::Constant>::create(bits);
    }
    return; // Must return early, or symbol will be clobbered below.
  } else if (direction == Direction::Forward) {
    // Must explicitly handle address wrap-around, because math inside set
    // address widens implicitly.
    newBase = (base + size) % 0x10000;
    // size is 1-index, while base is 0-indexed. Offset by 1. Unless size is 0,
    // in which case no adjustment is necessary.
    ast::setAddress(node, base, size);
    base = newBase;
  } else {
    newBase = (base - size) % 0x10000;
    // size is 1-index, while base is 0-indexed. Offset by 1. Unless size is 0,
    // in which case no adjustment is necessary.
    auto adjustedAddress = newBase + (size > 0 ? 1 : 0);
    // If we use newBase, we are off-by-one when size is non-zero.
    symBase = adjustedAddress;
    ast::setAddress(node, adjustedAddress % 0x10000, size);
    base = newBase;
  }

  if (node.has<ast::generic::SymbolDeclaration>()) {
    auto isCode = node.has<ast::pepp::Instruction<ISA>>();
    auto symbol = node.get<ast::generic::SymbolDeclaration>().value;
    symbol->value = QSharedPointer<symbol::value::Location>::create(
        size, sizeof(quint16), symBase, 0,
        isCode ? symbol::Type::kCode : symbol::Type::kObject);
  }
}

template <typename ISA> void pas::ops::pepp::assignAddresses(ast::Node &root) {
  QMap<QString, quint16> map;
  // Assign addresses section-by-section.
  // Assumes no nested sections, because this is impossible with ELF.
  // If we allow subsections (like NASM), this will need to move to full
  // recursion.
  for (auto &child : children(root)) {
    if (generic::isStructural()(*child)) {
      if (!child->has<pas::ast::generic::SectionName>())
        throw std::logic_error("Sections must be named");

      auto name = child->get<pas::ast::generic::SectionName>().value;
      auto sectionChildren = pas::ast::children(*child);
      bool hasBurn = detail::hasBurn(sectionChildren);

      // Setup entry in address table if the section hasn't been encountered
      // before.
      if (!map.contains(name)) {
        if (hasBurn)
          map[name] = detail::getBurnArg(sectionChildren);
        else
          map[name] = 0;
      }

      quint16 *base = &map[name];
      if (hasBurn)
        for (auto child = sectionChildren.rbegin();
             child != sectionChildren.rend(); ++child)
          detail::assignAddressesImpl<ISA>(**child, *base, Direction::Backward);
      else
        for (auto &child : sectionChildren)
          detail::assignAddressesImpl<ISA>(*child, *base, Direction::Forward);

    } else
      throw std::logic_error("code needs to be in a section!");
  }
}
