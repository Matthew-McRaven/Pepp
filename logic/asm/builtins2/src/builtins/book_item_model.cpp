#include "book_item_model.hpp"
#include "./book.hpp"
#include "./figure.hpp"
#include "./registry.hpp"
void addBookItems(QStandardItem *parent,
                  QSharedPointer<const builtins::Book> book) {
  auto figures = book->figures();
  for (const auto &figure : figures) {
    auto figureItem = new QStandardItem(
        u"%1.%2"_qs.arg(figure->chapterName(), figure->figureName()));
    figureItem->setData("figure", builtins::FigureConstants::FIG_ROLE_KIND);
    // Registry is read-only, and BookModel keeps it alive for the lifetime of
    // the model. Therefore, we can use the data() safely.
    figureItem->setData(QVariant::fromValue(figure.data()),
                        builtins::FigureConstants::FIG_ROLE_PAYLOAD);
    parent->appendRow(figureItem);
  }
}

builtins::BookModel::BookModel(QSharedPointer<Registry> registry)
    : _registry(registry) {
  this->setColumnCount(1);
  auto books = registry->books();
  for (const auto &book : books) {
    auto bookItem = new QStandardItem(book->name());
    addBookItems(bookItem, book);
    bookItem->setData("book", builtins::FigureConstants::FIG_ROLE_KIND);

    // Registry is read-only, and BookModel keeps it alive for the lifetime of
    // the model. Therefore, we can use the data() safely.
    auto list = QList<builtins::Figure *>();
    for (const auto &fig : book->figures())
      list.push_back(fig.data());
    bookItem->setData(QVariant::fromValue(list),
                      builtins::FigureConstants::FIG_ROLE_PAYLOAD);

    this->appendRow(bookItem);
  }
  auto at = this->item(0, 0);
}

QHash<int, QByteArray> builtins::BookModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[builtins::FigureConstants::FIG_ROLE_KIND] = "kind";
  roles[builtins::FigureConstants::FIG_ROLE_PAYLOAD] = "payload";
  roles[Qt::DisplayRole] = "display";
  return roles;
}
