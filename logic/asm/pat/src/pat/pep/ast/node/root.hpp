#pragma once
#include "./types.hpp"
#include "symbol/table.hpp"
namespace pat::pep::ast::node {

template <typename ISA> class SectionGroup {
public:
  explicit SectionGroup();
  explicit SectionGroup(QSharedPointer<symbol::Table> symbolTable);
  SectionGroup(const SectionGroup &other);
  SectionGroup(SectionGroup &&other) noexcept;
  SectionGroup &operator=(SectionGroup other);
  friend void swap(SectionGroup &first, SectionGroup &second) {
    using std::swap;
    swap(first._symbolTable, second._symbolTable);
    swap(first._children, second._children);
  }

  QSharedPointer<symbol::Table> symbolTable();
  QSharedPointer<const symbol::Table> symbolTable() const;
  void setSymbolTable(QSharedPointer<symbol::Table> symbolTable);
  void addChild(QSharedPointer<ChildNode<ISA>> node);

  // Value interface
  QSharedPointer<SectionGroup> clone() const;
  quint64 size() const;

private:
  QSharedPointer<symbol::Table> _symbolTable = nullptr;
  QList<QSharedPointer<ChildNode<ISA>>> _children = {};
};

template <typename ISA> class Root {
public:
  explicit Root();
  explicit Root(QSharedPointer<symbol::Table> symbolTable);
  Root(const Root &other);
  Root(Root &&other) noexcept;
  Root &operator=(Root other);
  friend void swap(Root &first, Root &second) {
    using std::swap;
    swap(first._symbolTable, second._symbolTable);
    swap(first._children, second._children);
  }

  QSharedPointer<symbol::Table> symbolTable();
  QSharedPointer<const symbol::Table> symbolTable() const;
  void setSymbolTable(QSharedPointer<symbol::Table> symbolTable);
  void addChild(QSharedPointer<ChildNode<ISA>> node);

  // Value interface
  QSharedPointer<Root> clone() const;

private:
  QSharedPointer<symbol::Table> _symbolTable = nullptr;
  QList<QSharedPointer<SectionGroup<ISA>>> _children = {};
};
}; // namespace pat::pep::ast::node
