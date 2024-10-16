/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// File: visit.cpp
#include "visit.hpp"

#include <sstream>

#include "table.hpp"
// Bitflags for determining which symbols to access during operations
enum SelectMode : quint8 {
  kSelf = 1,
  kChildren = 2,
  kError = 255,
};

// "bit shift" the children bit into self.
SelectMode toChild(SelectMode mode) { return static_cast<SelectMode>((mode & kChildren ? kSelf : 0) | kChildren); }

SelectMode policyToMode(symbol::TraversalPolicy policy) {
  switch (policy) {
  case symbol::TraversalPolicy::kChildren: [[fallthrough]];
  case symbol::TraversalPolicy::kSiblings: [[fallthrough]];
  case symbol::TraversalPolicy::kWholeTree: return static_cast<SelectMode>(kSelf | kChildren);
  case symbol::TraversalPolicy::kChildrenOnly:
    return static_cast<SelectMode>(kChildren);
    break;
    // default: // MSVC isn't smart enough to determine that all code paths will return/exit.
  }
  qFatal("Unhandled traversal policy");
  return SelectMode::kError;
}

// Get the proper root for a given traversal policy
QSharedPointer<symbol::Table> policyToTargetTable(symbol::TraversalPolicy policy, QSharedPointer<symbol::Table> table) {
  switch (policy) {
  case symbol::TraversalPolicy::kChildren: [[fallthrough]];
  case symbol::TraversalPolicy::kChildrenOnly: return table;
  case symbol::TraversalPolicy::kSiblings: return symbol::parent(table);
  case symbol::TraversalPolicy::kWholeTree:
    return symbol::rootTable(table);
    // default: // MSVC isn't smart enough to determine that all code paths will return/exit.
  }
  qFatal("Unhandled traversal policy");
  return nullptr;
}

QSharedPointer<const symbol::Table> symbol::rootTable(QSharedPointer<const Table> table) {
  if (auto parent = table->parent.lock(); !parent.isNull()) return rootTable(parent);
  else return table;
}

QSharedPointer<symbol::Table> symbol::rootTable(QSharedPointer<Table> table) {
  if (auto parent = table->parent.lock(); !parent.isNull()) return rootTable(parent);
  else return table;
}

void selectByNameImpl(QList<QSharedPointer<symbol::Entry>> &list, QSharedPointer<symbol::Table> table,
                      const QString &name, SelectMode mode) {
  if (mode & kChildren) {
    for (auto &child : table->children()) {
      selectByNameImpl(list, child, name, toChild(mode));
    }
  }
  if (mode & kSelf) {
    auto maybeSymbol = table->get(name);
    if (maybeSymbol) list.push_back(maybeSymbol.value());
  }
}
QList<QSharedPointer<symbol::Entry>> symbol::selectByName(QSharedPointer<Table> table, const QString &name,
                                                          TraversalPolicy policy) {
  QList<QSharedPointer<symbol::Entry>> ret;
  selectByNameImpl(ret, policyToTargetTable(policy, table), name, policyToMode(policy));
  return ret;
}

void existsImpl(bool &exists, QSharedPointer<symbol::Table> table, const QString &name, SelectMode mode) {
  if (mode & kChildren) {
    for (auto &child : table->children()) {
      existsImpl(exists, child, name, toChild(mode));
    }
  }
  if (mode & kSelf) {
    exists |= table->exists(name);
  }
}
bool symbol::exists(QSharedPointer<Table> table, const QString &name, TraversalPolicy policy) {
  bool ret = false;
  existsImpl(ret, policyToTargetTable(policy, table), name, policyToMode(policy));
  return ret;
}

void adjustOffsetImpl(QSharedPointer<symbol::Table> table, quint64 offset, quint64 threshold, SelectMode mode) {
  if (mode & kChildren) {
    for (auto &child : table->children()) {
      adjustOffsetImpl(child, offset, threshold, toChild(mode));
    }
  }
  if (mode & kSelf) {
    for (auto entry : table->entries()) {
      auto value = entry.second->value;
      if (!value->relocatable() || value->value()() < threshold) {
        // pass, assignment logic would have been more convoluted without this
      } else if (auto location = qSharedPointerCast<symbol::value::Location>(value); !value.isNull())
        location->setOffset(offset);
    }
  }
}
void symbol::adjustOffset(QSharedPointer<Table> table, quint64 offset, quint64 threshold, TraversalPolicy policy) {
  adjustOffsetImpl(policyToTargetTable(policy, table), offset, threshold, policyToMode(policy));
}

void enumerateImpl(QList<QSharedPointer<symbol::Entry>> &list, QSharedPointer<symbol::Table> table, SelectMode mode) {
  if (mode & kChildren) {
    for (auto &child : table->children()) {
      enumerateImpl(list, child, toChild(mode));
    }
  }
  if (mode & kSelf) {
    for (auto entry : table->entries()) list.push_back(entry.second);
  }
}
QList<QSharedPointer<symbol::Entry>> symbol::enumerate(QSharedPointer<Table> table, TraversalPolicy policy) {
  QList<QSharedPointer<symbol::Entry>> ret;
  enumerateImpl(ret, policyToTargetTable(policy, table), policyToMode(policy));
  return ret;
}

QString symbol::tableListing(QSharedPointer<Table> table, quint8 maxBytes, TraversalPolicy policy) {
  using namespace Qt::StringLiterals;
  // TODO: Use QString throughout, potentially eliding spurious data copy.
  auto symbols = enumerate(table, policy);
  auto it = symbols.cbegin();
  // Compute the bitwidth of the symbol table.
  // Helper lambda to pretty print a single symbol.
  auto format = [&maxBytes](const auto &sym) {
    return u"%1 0x%2"_s.arg(sym->name.leftJustified(9, ' '))
        .arg(QString::number(sym->value->value()(), 16).toUpper(), 2 * maxBytes, u'0');
  };
  std::ostringstream ss;
  bool lhs = true;
  while (it != symbols.cend()) {
    if (lhs) ss << format(*it++).toStdString();
    else ss << "         " << format(*it++).toStdString() << std::endl;
    lhs ^= true;
  }

  // The last entry was on the left, but we did not append a newline. Insert
  // here to fix #399
  if (!lhs) ss << std::endl;

  return QString::fromStdString(ss.str());
}

QSharedPointer<symbol::Table> symbol::parent(QSharedPointer<Table> table) {
  if (table->parent) {
    if (auto parent = table->parent.lock(); parent) return parent;
  }
  return table;
}
QSharedPointer<const symbol::Table> symbol::parent(QSharedPointer<const Table> table) {
  if (table->parent) {
    if (auto parent = table->parent.lock(); parent) return parent;
  }
  return table;
}
