#include "figure.hpp"

builtins::Figure::Figure(Architecture arch, QString chapter, QString figure)
    : QObject(nullptr), _arch(arch), _chapterName(chapter),
      _figureName(figure) {}

builtins::Architecture builtins::Figure::arch() const { return _arch; }

QString builtins::Figure::chapterName() const { return _chapterName; }

QString builtins::Figure::figureName() const { return _figureName; }

bool builtins::Figure::isOS() const { return _isOS; }

bool builtins::Figure::setIsOS(bool value) {
  if (value != _isOS) {
    _isOS = value;
    emit isOSChanged();
    return true;
  }
  return false;
}

QSharedPointer<const builtins::Figure> builtins::Figure::defaultOS() const {
  return _defaultOS;
}

bool builtins::Figure::setDefaultOS(QSharedPointer<const Figure> defaultOS) {
  if (defaultOS != defaultOS) {
    _defaultOS = defaultOS;
    emit defaultOSChanged();
    return true;
  }
  return false;
}

const QList<QSharedPointer<const builtins::Test>>
builtins::Figure::tests() const {
  return _tests;
}

void builtins::Figure::addTest(QSharedPointer<Test> test) {
  _tests.push_back(test);
  emit testsChanged();
}

const QMap<QString, QSharedPointer<const builtins::Element>>
builtins::Figure::elements() const {
  return _elements;
}

bool builtins::Figure::addElement(QString name,
                                  QSharedPointer<Element> element) {
  if (auto it = _elements.constFind(name); it != _elements.constEnd()) {
    _elements[name] = element;
    emit elementsChanged();
    return true;
  }
  return false;
}
