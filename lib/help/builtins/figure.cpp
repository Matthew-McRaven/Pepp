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

#include "figure.hpp"

builtins::Figure::Figure(pepp::Architecture arch, pepp::Abstraction level, QString prefix, QString chapter,
                         QString figure, bool isProblem)
    : QObject(nullptr), _arch(arch), _level(level), _prefix(prefix), _chapterName(chapter), _figureName(figure),
      _isProblem(isProblem) {}

builtins::Figure::~Figure() {
  for (auto value : _tests) {
    delete value;
  }
  for (auto value : _elements) {
    delete value;
  }
}

pepp::Architecture builtins::Figure::arch() const { return _arch; }

pepp::Abstraction builtins::Figure::level() const { return _level; }

QString builtins::Figure::prefix() const { return _prefix; }

QString builtins::Figure::chapterName() const { return _chapterName; }

QString builtins::Figure::figureName() const { return _figureName; }

bool builtins::Figure::isProblem() const { return _isProblem; }

QString builtins::Figure::description() const { return _description; }

void builtins::Figure::setDescription(QString description) { _description = description; }

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

bool builtins::Figure::isHidden() const { return _isHidden; }

bool builtins::Figure::setIsHidden(bool value) {
  if (value != _isHidden) {
    _isHidden = value;
    emit isHiddenChanged();
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
  for (auto x : _tests) v.push_back(QVariant::fromValue(x));
  return v;
}
void builtins::Figure::addTest(const Test *test) {
  // Unlike elements, do not de-duplicate tests, as this appears to be
  // difficult.
  _tests.push_back(test);
  emit testsChanged();
}

const builtins::Element *builtins::Figure::findElement(QString name) const {
  if (auto ret = _elements.constFind(name); ret != _elements.constEnd()) return ret.value();
  else return nullptr;
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

QString builtins::Figure::defaultElement() const { return _defaultElement; }

void builtins::Figure::setDefaultElement(QString lang) { _defaultElement = lang; }
