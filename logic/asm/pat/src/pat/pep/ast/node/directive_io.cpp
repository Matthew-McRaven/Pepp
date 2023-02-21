#include "./directive_io.hpp"

pat::pep::ast::node::Input::Input() : Symbolic() {}

pat::pep::ast::node::Input::Input(
    QSharedPointer<pat::ast::argument::Symbolic> argument,
    pat::ast::node::FileLocation sourceLocation, QWeakPointer<Base> parent)
    : Symbolic(argument, sourceLocation, parent) {}

pat::pep::ast::node::Input::Input(const Input &other) : Symbolic(other) {}

pat::pep::ast::node::Input::Input(Input &&other) noexcept {
  swap(*this, other);
}

pat::pep::ast::node::Input &pat::pep::ast::node::Input::operator=(Input other) {
  swap(*this, other);
  return *this;
}

pat::pep::ast::node::Input::ValidateResult
pat::pep::ast::node::Input::validate_argument(
    QSharedPointer<const pat::ast::argument::Symbolic> argument) {
  return {.valid = true};
}

QSharedPointer<pat::ast::Value> pat::pep::ast::node::Input::clone() const {
  return QSharedPointer<Input>::create(*this);
}

QString pat::pep::ast::node::Input::string() const {
  throw std::logic_error("Unimplemented");
}

pat::pep::ast::node::Output::Output() : Symbolic() {}

pat::pep::ast::node::Output::Output(
    QSharedPointer<pat::ast::argument::Symbolic> argument,
    pat::ast::node::FileLocation sourceLocation, QWeakPointer<Base> parent)
    : Symbolic(argument, sourceLocation, parent) {}

pat::pep::ast::node::Output::Output(const Output &other) : Symbolic(other) {}

pat::pep::ast::node::Output::Output(Output &&other) noexcept {
  swap(*this, other);
}

pat::pep::ast::node::Output &
pat::pep::ast::node::Output::operator=(Output other) {
  swap(*this, other);
  return *this;
}

pat::pep::ast::node::Output::ValidateResult
pat::pep::ast::node::Output::validate_argument(
    QSharedPointer<const pat::ast::argument::Symbolic> argument) {
  return {.valid = true};
}

QSharedPointer<pat::ast::Value> pat::pep::ast::node::Output::clone() const {
  return QSharedPointer<Output>::create(*this);
}

QString pat::pep::ast::node::Output::string() const {
  throw std::logic_error("Unimplemented");
}
