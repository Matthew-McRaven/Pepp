#include "./group.hpp"
#include "pas/ast/generic/attr_parent.hpp"
#include "pas/ast/generic/attr_sec.hpp"
#include "pas/ast/value/base.hpp"
#include "pas/operations/pepp/is.hpp"

pas::ops::generic::GroupSections::GroupSections(
    QString defaultSectionName,
    std::function<bool(const ast::Node &)> addressable)
    : addressable(addressable) {
  using namespace pas::ast::generic;
  currentSection = QSharedPointer<pas::ast::Node>::create(
      Type{.value = pas::ast::generic::Type::Structural});
  currentSection->set(SectionName{.value = defaultSectionName});
  currentSection->set(SectionFlags{.value = {.R = 1, .W = 1, .X = 1}});

  this->newChildren.value.push_back(currentSection);
}

void attemptParseName(pas::ast::Node &node, const pas::ast::value::Base &arg) {
  if (!arg.isText())
    throw std::logic_error("missing section name");
  node.set(pas::ast::generic::SectionName{.value = arg.rawString()});
}

void pas::ops::generic::GroupSections::operator()(ast::Node &node) {
  using namespace pas::ast::generic;
  if (pas::ops::pepp::isSection()(node)) {
    hasSeenAddressable = false;
    SectionFlags flags;
    auto newSection =
        QSharedPointer<pas::ast::Node>::create(Type{.value = Type::Structural});

    if (node.has<Argument>()) {
      attemptParseName(*newSection, *node.get<Argument>().value);
    } else if (node.has<ArgumentList>()) {
      auto args = node.get<ArgumentList>().value;
      if (args.size() > 2)
        throw std::logic_error("Too many arguments to section");
      attemptParseName(*newSection, *args[0]);

      auto flagArg = args[1];
      if (!flagArg->isText())
        throw std::logic_error("Flags must be text");
      auto flagText = flagArg->rawString();
      flags.value.R = flagText.contains("R", Qt::CaseInsensitive);
      flags.value.W = flagText.contains("W", Qt::CaseInsensitive);
      flags.value.X = flagText.contains("X", Qt::CaseInsensitive);
    }

    newSection->set(flags);

    currentSection = newSection;
    newChildren.value.push_back(currentSection);
    /*noAddressableNodes=true*/
    // Only start a new section if an org is embeded in the middle of another.
  } else if (pas::ops::pepp::isOrg()(node) && hasSeenAddressable) {
    auto newSection =
        QSharedPointer<pas::ast::Node>::create(Type{.value = Type::Structural});
    newSection->set(currentSection->get<SectionName>());
    newSection->set(currentSection->get<SectionFlags>());

    currentSection = newSection;
    newChildren.value.push_back(currentSection);
  } else if (addressable(node))
    hasSeenAddressable = true;

  // Reparent nodes to the appropriate section.
  ast::setParent(node, currentSection);
  ast::addChild(*currentSection, node.sharedFromThis());
}

void pas::ops::generic::groupSections(
    ast::Node &root, std::function<bool(const ast::Node &)> addressable) {
  GroupSections sections(".text", addressable);

  // Can't apply_recurse, because visitor modifies children.
  for (auto &child : children(root)) {
    if (child->has<ast::generic::Children>() &&
        ast::children(*child).size() > 0)
      throw std::logic_error("Not allowed to have nested children.");

    child->apply_self(sections);
  }

  // Fix parent-child relationships between new intermediate nodes and root.
  root.set(sections.newChildren);
  auto sharedRoot = root.sharedFromThis();
  for (auto child : sections.newChildren.value)
    child->set(ast::generic::Parent{.value = sharedRoot});
}
