#include "./directive_external.hpp"

pat::pep::ast::node::Export::Export() : Symbolic() {}

pat::pep::ast::node::Export::Export(
    QSharedPointer<pat::ast::argument::Symbolic> argument,
    pat::ast::node::FileLocation sourceLocation, QWeakPointer<Base> parent)
    : Symbolic(argument, sourceLocation, parent) {}

pat::pep::ast::node::Export::Export(const Export &other) : Symbolic(other) {}

pat::pep::ast::node::Export::Export(Export &&other) noexcept {
  swap(*this, other);
}

pat::pep::ast::node::Export &
pat::pep::ast::node::Export::operator=(Export other) {
  swap(*this, other);
  return *this;
}

pat::pep::ast::node::Export::ValidateResult
pat::pep::ast::node::Export::validate_argument(
    QSharedPointer<const pat::ast::argument::Symbolic> argument) {
  return {.valid = true};
}

QSharedPointer<pat::ast::node::Base>
pat::pep::ast::node::Export::clone() const {
  return QSharedPointer<Export>::create(*this);
}

QString pat::pep::ast::node::Export::string() const {
  throw std::logic_error("Unimplemented");
}

pat::pep::ast::node::Import::Import() : Symbolic() {}

pat::pep::ast::node::Import::Import(
    QSharedPointer<pat::ast::argument::Symbolic> argument,
    pat::ast::node::FileLocation sourceLocation, QWeakPointer<Base> parent)
    : Symbolic(argument, sourceLocation, parent) {}

pat::pep::ast::node::Import::Import(const Import &other) : Symbolic(other) {}

pat::pep::ast::node::Import::Import(Import &&other) noexcept {
  swap(*this, other);
}

pat::pep::ast::node::Import &
pat::pep::ast::node::Import::operator=(Import other) {
  swap(*this, other);
  return *this;
}

pat::pep::ast::node::Import::ValidateResult
pat::pep::ast::node::Import::validate_argument(
    QSharedPointer<const pat::ast::argument::Symbolic> argument) {
  return {.valid = true};
}

QSharedPointer<pat::ast::node::Base>
pat::pep::ast::node::Import::clone() const {
  return QSharedPointer<Import>::create(*this);
}

QString pat::pep::ast::node::Import::string() const {
  throw std::logic_error("Unimplemented");
}
