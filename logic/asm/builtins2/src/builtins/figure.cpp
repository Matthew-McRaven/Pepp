#include "figure.hpp"

builtins::Figure::Figure(Architecture arch, QString chapter, QString figure)
    : QObject(nullptr), _arch(arch), _chapterName(chapter),
      _figureName(figure) {}

builtins::Figure::~Figure() {
  for (auto value : _tests) {
    delete value;
  }
  for (auto value : _elements.values()) {
    delete value;
  }
}

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

const builtins::Figure *builtins::Figure::defaultOS() const {
  return _defaultOS;
}

bool builtins::Figure::setDefaultOS(const Figure *defaultOS) {
  if (_defaultOS != defaultOS) {
    _defaultOS = defaultOS;
    emit defaultOSChanged();
    return true;
  }
  return false;
}

const QList<const builtins::Test *> builtins::Figure::typesafeTests() const {
  return _tests;
}

QVariantList builtins::Figure::tests() const {
  QVariantList v;
  for (auto x : _tests)
    v.push_back(QVariant::fromValue(x));
  return v;
}
void builtins::Figure::addTest(const Test *test) {
  _tests.push_back(test);
  emit testsChanged();
}

const QMap<QString, const builtins::Element *>
builtins::Figure::typesafeElements() const {
  return _elements;
}

QVariantMap builtins::Figure::elements() const {
  QVariantMap v;
  for (auto key = _elements.keyBegin(); key != _elements.keyEnd(); key++)
    v[*key] = QVariant::fromValue(_elements[*key]);
  return v;
}

bool builtins::Figure::addElement(QString name, const Element *element) {
  if (auto it = _elements.constFind(name); it == _elements.constEnd()) {
    _elements[name] = element;
    emit elementsChanged();
    return true;
  }
  return false;
}
