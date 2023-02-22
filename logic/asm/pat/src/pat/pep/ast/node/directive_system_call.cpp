#include "./directive_system_call.hpp"

pat::pep::ast::node::SCall::SCall() : Symbolic() {}

pat::pep::ast::node::SCall::SCall(
    QSharedPointer<pat::ast::argument::Symbolic> argument,
    pat::ast::node::FileLocation sourceLocation, QWeakPointer<Base> parent)
    : Symbolic(argument, sourceLocation, parent) {}

pat::pep::ast::node::SCall::SCall(const SCall &other) : Symbolic(other) {}

pat::pep::ast::node::SCall::SCall(SCall &&other) noexcept {
  swap(*this, other);
}

pat::pep::ast::node::SCall &pat::pep::ast::node::SCall::operator=(SCall other) {
  swap(*this, other);
  return *this;
}

pat::pep::ast::node::SCall::ValidateResult
pat::pep::ast::node::SCall::validate_argument(
    QSharedPointer<const pat::ast::argument::Symbolic> argument) {
  return {.valid = true};
}

QSharedPointer<pat::ast::node::Base> pat::pep::ast::node::SCall::clone() const {
  return QSharedPointer<SCall>::create(*this);
}

QString pat::pep::ast::node::SCall::string() const {
  throw std::logic_error("Unimplemented");
}

pat::pep::ast::node::USCall::USCall() : Symbolic() {}

pat::pep::ast::node::USCall::USCall(
    QSharedPointer<pat::ast::argument::Symbolic> argument,
    pat::ast::node::FileLocation sourceLocation, QWeakPointer<Base> parent)
    : Symbolic(argument, sourceLocation, parent) {}

pat::pep::ast::node::USCall::USCall(const USCall &other) : Symbolic(other) {}

pat::pep::ast::node::USCall::USCall(USCall &&other) noexcept {
  swap(*this, other);
}

pat::pep::ast::node::USCall &
pat::pep::ast::node::USCall::operator=(USCall other) {
  swap(*this, other);
  return *this;
}

pat::pep::ast::node::USCall::ValidateResult
pat::pep::ast::node::USCall::validate_argument(
    QSharedPointer<const pat::ast::argument::Symbolic> argument) {
  return {.valid = true};
}

QSharedPointer<pat::ast::node::Base>
pat::pep::ast::node::USCall::clone() const {
  return QSharedPointer<USCall>::create(*this);
}

QString pat::pep::ast::node::USCall::string() const {
  throw std::logic_error("Unimplemented");
}
