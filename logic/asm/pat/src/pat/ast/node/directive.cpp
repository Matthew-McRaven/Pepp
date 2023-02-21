#include "./directive.hpp"
pat::ast::node::Directive::Directive() : Base() {}

pat::ast::node::Directive::Directive(FileLocation sourceLocation,
                                     QWeakPointer<Base> parent)
    : Base(sourceLocation, parent) {}

QSharedPointer<const symbol::Entry> pat::ast::node::Directive::symbol() const {
  return _symbol;
}

QSharedPointer<symbol::Entry> pat::ast::node::Directive::symbol() {
  return _symbol;
}

void pat::ast::node::Directive::setSymbol(
    QSharedPointer<symbol::Entry> symbol) {
  _symbol = symbol;
}

std::optional<QString> pat::ast::node::Directive::comment() const {
  return _comment;
}

void pat::ast::node::Directive::setComment(std::optional<QString> comment) {
  _comment = comment;
}

pat::ast::node::Directive::Directive(const Directive &other)
    : Base(other), _symbol(other._symbol), _comment(other._comment) {}

pat::ast::node::Directive &
pat::ast::node::Directive::operator=(const Directive &other) {
  Base::operator=(other);
  this->_symbol = other._symbol;
  this->_comment = other._comment;
  return *this;
}
