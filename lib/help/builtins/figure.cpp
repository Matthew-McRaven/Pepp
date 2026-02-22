/*
 * Copyright (c) 2023-2027 J. Stanley Warford, Matthew McRaven
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

builtins::Figure::Figure(pepp::Architecture arch, pepp::Abstraction level, pepp::Features feats, QString prefix,
                         QString chapter, QString figure, bool isProblem)
    : QObject(nullptr), _arch(arch), _level(level), _features(feats), _prefix(prefix), _chapterName(chapter),
      _figureName(figure), _isProblem(isProblem) {}

builtins::Figure::~Figure() {
  for (auto value : _tests) delete value;
  _namedFragments.clear();
  for (auto value : _allFragments) delete value;
}

pepp::Architecture builtins::Figure::arch() const { return _arch; }

pepp::Abstraction builtins::Figure::level() const { return _level; }

pepp::Features builtins::Figure::features() const { return _features; }

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
  // Convert type-correct map to a QVariantMap, which can be accessed natively in QML
  QVariantList v;
  for (auto x : _tests) v.push_back(QVariant::fromValue(x));
  return v;
}
void builtins::Figure::addTest(const Test *test) {
  // Unlike fragments, do not de-duplicate tests, as this appears to be difficult.
  _tests.push_back(test);
  emit testsChanged();
}

const builtins::Fragment *builtins::Figure::findFragment(QString name) const {
  if (auto ret = _namedFragments.constFind(name); ret != _namedFragments.constEnd()) return ret.value();
  else return nullptr;
}

const QList<const builtins::Fragment *> &builtins::Figure::typesafeFragments() const { return _allFragments; }

const QMap<QString, const builtins::Fragment *> builtins::Figure::typesafeNamedFragments() const {
  return _namedFragments;
}

QVariantMap builtins::Figure::namedFragments() const {
  // Convert type-correct map to a QVariantMap, which can be accessed natively in QML
  QVariantMap v;
  for (auto key = _namedFragments.keyBegin(); key != _namedFragments.keyEnd(); key++)
    v[*key] = QVariant::fromValue(_namedFragments[*key]);
  return v;
}

bool builtins::Figure::addFragment(const Fragment *frag) {
  QString name = frag->name;

  // Only signal update if the figure does not already contain an element of the
  // same name (e.g., programming language)
  if (name.isEmpty()) {
    _allFragments.push_back(frag);
    return true;
  } else if (auto it = _namedFragments.constFind(name); it == _namedFragments.constEnd()) {
    _allFragments.push_back(frag);
    _namedFragments[name] = frag;
    emit fragmentsChanged();
    return true;
  } else return false;
}

QString builtins::Figure::defaultFragmentName() const { return _defaultFragmentName; }

void builtins::Figure::setDefaultFragmentName(QString name) { _defaultFragmentName = name; }

QString builtins::Figure::defaultFragmentText() const {
  if (_namedFragments.contains(_defaultFragmentName)) return _namedFragments[_defaultFragmentName]->contentsFn();
  return QString{};
}

QString builtins::Figure::defaultOSText() const {
  if (_defaultOS) return _defaultOS->defaultFragmentText();
  return QString{};
}
