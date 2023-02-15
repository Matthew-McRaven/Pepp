#include "figure.hpp"

builtins::Figure::Figure(Architecture arch, QString chapter, QString figure)
    : QObject(nullptr), _arch(arch), _chapterName(chapter),
      _figureName(figure) {}

builtins::Figure::~Figure() {
  for (auto value : _tests) {
    delete value;
  }
  for (auto value : _elements) {
    delete value;
  }
}

builtins::Architecture builtins::Figure::arch() const { return _arch; }

QString builtins::Figure::chapterName() const { return _chapterName; }

QString builtins::Figure::figureName() const { return _figureName; }

bool builtins::Figure::isOS() const { return _isOS; }

bool builtins::Figure::setIsOS(bool value) {
  // Do not omit OS if isOS remains unchanged
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
  // Do not emit event if OS remains unchanged.
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
  // Convert type-correct map to a QVariantMap, which can be accessed natively
  // in QML
  QVariantList v;
  for (auto x : _tests)
    v.push_back(QVariant::fromValue(x));
  return v;
}
void builtins::Figure::addTest(const Test *test) {
  // Unlike elements, do not de-duplicate tests, as this appears to be
  // difficult.
  _tests.push_back(test);
  emit testsChanged();
}

const QMap<QString, const builtins::Element *>
builtins::Figure::typesafeElements() const {
  return _elements;
}

QVariantMap builtins::Figure::elements() const {
  // Convert type-correct map to a QVariantMap, which can be accessed natively
  // in QML
  QVariantMap v;
  for (auto key = _elements.keyBegin(); key != _elements.keyEnd(); key++)
    v[*key] = QVariant::fromValue(_elements[*key]);
  return v;
}

bool builtins::Figure::addElement(QString name, const Element *element) {
  // Only signal update if the figure does not already contain an element of the
  // same name (e.g., programming language)
  if (auto it = _elements.constFind(name); it == _elements.constEnd()) {
    _elements[name] = element;
    emit elementsChanged();
    return true;
  }
  return false;
}
