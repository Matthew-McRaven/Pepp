/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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

#include "book_item_model.hpp"
#include "builtins/book.hpp"
#include "builtins/figure.hpp"
#include "builtins/registry.hpp"
void addBookItems(QStandardItem *parent, QSharedPointer<const builtins::Book> book) {
  auto figures = book->figures();
  for (const auto &figure : figures) {
    auto figureItem =
        new QStandardItem(u"%1 %2.%3"_qs.arg(figure->prefix(), figure->chapterName(), figure->figureName()));
    figureItem->setData("figure", builtins::FigureConstants::FIG_ROLE_KIND);
    // Registry is read-only, and BookModel keeps it alive for the lifetime of
    // the model. Therefore, we can use the data() safely.
    figureItem->setData(QVariant::fromValue(figure.data()), builtins::FigureConstants::FIG_ROLE_PAYLOAD);
    figureItem->setData(book->name(), builtins::FigureConstants::FIG_ROLE_EDITION);
    figureItem->setData(QVariant::fromValue(figure->arch()), builtins::FigureConstants::FIG_ROLE_ARCHITECTURE);
    figureItem->setData(QVariant::fromValue(figure->level()), builtins::FigureConstants::FIG_ROLE_ABSTRACTION);
    parent->appendRow(figureItem);
  }
}
QSharedPointer<builtins::Registry> createRegistry() { return QSharedPointer<builtins::Registry>::create(nullptr); }
builtins::BookModel::BookModel(QObject *parent) : QStandardItemModel(parent), _registry(createRegistry()) {
  this->setColumnCount(1);
  auto books = _registry->books();
  for (const auto &book : books) {
    auto bookItem = new QStandardItem(book->name());
    addBookItems(bookItem, book);

    // Mark the item as being a book
    bookItem->setData("book", builtins::FigureConstants::FIG_ROLE_KIND);
    bookItem->setData(book->name(), builtins::FigureConstants::FIG_ROLE_EDITION);
    // Registry is read-only, and BookModel keeps it alive for the lifetime of
    // the model. Therefore, we can use the data() safely.
    auto list = QList<builtins::Figure *>();
    for (const auto &fig : book->figures())
      list.push_back(fig.data());
    bookItem->setData(QVariant::fromValue(list), builtins::FigureConstants::FIG_ROLE_PAYLOAD);

    this->appendRow(bookItem);
  }
}

QHash<int, QByteArray> builtins::BookModel::roleNames() const {
  // Extend QStandardItemModel role names
  QHash<int, QByteArray> roles = QStandardItemModel::roleNames();
  // Add our custom roles, so that the can be accessed by name in QML
  roles[builtins::FigureConstants::FIG_ROLE_KIND] = "kind";
  roles[builtins::FigureConstants::FIG_ROLE_PAYLOAD] = "payload";
  roles[builtins::FigureConstants::FIG_ROLE_EDITION] = "edition";
  roles[builtins::FigureConstants::FIG_ROLE_ARCHITECTURE] = "architecture";
  roles[builtins::FigureConstants::FIG_ROLE_ABSTRACTION] = "abstraction";
  return roles;
}
