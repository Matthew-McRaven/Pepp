#include "book.hpp"
#include "figure.hpp"
builtins::Book::Book(QString name) : QObject(nullptr), _name(name) {}

QString builtins::Book::name() const { return _name; }

const QList<QSharedPointer<builtins::Figure>> builtins::Book::figures() const {
  return _figures;
}

QSharedPointer<const builtins::Figure>
builtins::Book::findFigure(QString chapter, QString figure) const {
  QList<QSharedPointer<const builtins::Figure>> temp;
  for (auto figurePtr : _figures) {
    if (figurePtr->chapterName() == chapter &&
        figurePtr->figureName() == figure) {
      temp.push_back(figurePtr);
    }
  }
  if (auto length = temp.length(); length == 0)
    return nullptr;
  else if (length == 1)
    return temp.first();
  else {
    qDebug()
        << (u"More than one copy of figure {}.{}"_qs).arg(chapter).arg(figure);
    return nullptr;
  }
}

bool builtins::Book::addFigure(QSharedPointer<Figure> figure) {
  // TODO: Adding N figures will take N^2 time because of the calls to find.
  // Will be necessary to speed this up for large N.
  if (findFigure(figure->chapterName(), figure->figureName()) == nullptr) {
    _figures.push_back(figure);
    return true;
  } else
    return false;
}

const QList<QSharedPointer<builtins::Macro>> builtins::Book::macros() const {
  return _macros;
}

QSharedPointer<const builtins::Macro>
builtins::Book::findMacro(QString name) const {
  QList<QSharedPointer<const builtins::Macro>> temp;
  for (const auto &macroPtr : _macros) {
    if (macroPtr->name == name)
      temp.push_back(macroPtr);
  }
  if (auto length = temp.length(); length == 0)
    return nullptr;
  else if (length == 1)
    return temp.first();
  else {
    qDebug() << (u"More than one copy of macro {}"_qs).arg(name);
    return nullptr;
  }
}

bool builtins::Book::addMacro(QSharedPointer<Macro> macro) {
  // TODO: Adding N macros will take N^2 time because of the calls to find.
  // Will be necessary to speed this up for large N.
  if (findMacro(macro->name) == nullptr) {
    _macros.push_back(macro);
    return true;
  } else
    return false;
}
