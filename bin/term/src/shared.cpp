#include "./shared.hpp"
QSharedPointer<const builtins::Book> detail::book(int ed) {
  QString bookName;
  switch (ed) {
  case 6:
    bookName = "Computer Systems, 6th Edition";
  default:
    return nullptr;
  }

  auto reg = builtins::Registry(nullptr);
  auto book = reg.findBook(bookName);
  return book;
}
