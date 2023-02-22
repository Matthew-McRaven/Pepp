#include "./directive_section.hpp"
pat::ast::node::Section::Section() : Directive() {}

pat::ast::node::Section::Section(QSharedPointer<argument::Identifier> argument,
                                 FileLocation sourceLocation,
                                 QWeakPointer<Base> parent)
    : Directive(sourceLocation, parent) {}

pat::ast::node::Section::Section(const Section &other)
    : Directive(other), _config(other._config), _argument(other._argument),
      _attributes(other._attributes) {}

pat::ast::node::Section::Section(Section &&other) noexcept {
  swap(*this, other);
}

pat::ast::node::Section &pat::ast::node::Section::operator=(Section other) {
  swap(*this, other);
  return *this;
}

QSharedPointer<pat::ast::node::Base> pat::ast::node::Section::clone() const {
  return QSharedPointer<Section>::create(*this);
}

quint64 pat::ast::node::Section::size() const { return 0; }

bool pat::ast::node::Section::value(quint8 *dest, qsizetype length,
                                    bits::BitOrder destEndian) const {
  return true;
}

QString pat::ast::node::Section::string() const {
  throw std::logic_error("Unimplemented");
}

const pat::ast::node::AddressSpan &
pat::ast::node::Section::addressSpan() const {
  throw std::logic_error("Unimplemented");
}

void pat::ast::node::Section::updateAddressSpan(void *update) const {
  throw std::logic_error("Unimplemented");
}

bool pat::ast::node::Section::emitsBytes() const { return false; }

void pat::ast::node::Section::setEmitsBytes(bool emitBytes) {}
