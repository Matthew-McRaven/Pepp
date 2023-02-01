#pragma once

#include <QObject>
Q_MOC_INCLUDE("builtins/book.hpp")
namespace builtins {
class Book;
class Registry : public QObject {
  Q_OBJECT
  Q_PROPERTY(
      QList<QSharedPointer<const builtins::Book>> books READ books CONSTANT);

public:
  // Crawling the Qt help system to create books is handled inside CTOR.
  explicit Registry(void *asm_toolchains);
  QList<QSharedPointer<const builtins::Book>> books() const;
  QSharedPointer<const builtins::Book> findBook(QString name);

private:
  QList<QSharedPointer<const builtins::Book>> _books;
};

class Test;
class Figure;
class Element;
class Macro;
namespace detail {
QSharedPointer<::builtins::Element> loadElement(QString elementPath);
QSharedPointer<::builtins::Element> generateElement(QString fromElementPath,
                                                    void *asm_toolchains);
QSharedPointer<::builtins::Test> loadTest(QString testDirPath);
QSharedPointer<::builtins::Figure> loadFigure(QString manifestPath);
void linkFigureOS(QString manifestPath,
                  QSharedPointer<::builtins::Figure> figure,
                  QSharedPointer<const builtins::Book> book);
QList<QSharedPointer<::builtins::Macro>> loadMacro(QString manifestPath);
QSharedPointer<::builtins::Book> loadBook(QString tocPath);
QList<QString> enumerateBooks(QString prefix);
} // end namespace detail
} // end namespace builtins
