/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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

#pragma once
#include <QtCore>
#include <qqmlintegration.h>
#include "core/resources/figures/book.hpp"
#include "core/resources/figures/builtin_registry.hpp"
#include "core/resources/figures/figure.hpp"
#include "core/resources/figures/fragment.hpp"
namespace builtins {

class MacroWrapper : public QObject {
  Q_OBJECT
  Q_PROPERTY(int arch READ qml_arch CONSTANT);
  Q_PROPERTY(QString name READ name CONSTANT);
  Q_PROPERTY(QString text READ text CONSTANT);

public:
  MacroWrapper(std::shared_ptr<pepp::MacroFile> wrapper) : QObject(nullptr), _wrapper(std::move(wrapper)) {}
  ~MacroWrapper() = default;
  pepp::MacroFile *underlying() { return _wrapper.get(); }
  const pepp::MacroFile *underlying() const { return _wrapper.get(); }

  int qml_arch() const { return (int)_wrapper->arch; }
  QString name() const { return QString::fromStdString(_wrapper->name); }
  QString text() const { return QString::fromStdString(_wrapper->body); }

private:
  std::shared_ptr<pepp::MacroFile> _wrapper;
};

class TestWrapper : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariant input READ input CONSTANT);
  Q_PROPERTY(QVariant output READ output CONSTANT);

public:
  TestWrapper(std::shared_ptr<pepp::Test> wrapper) : QObject(nullptr), _wrapper(wrapper) {}
  ~TestWrapper() = default;
  pepp::Test *underlying() { return _wrapper.get(); }
  const pepp::Test *underlying() const { return _wrapper.get(); }

  QVariant input() const { return QString::fromStdString(_wrapper->input); }
  QVariant output() const { return QString::fromStdString(_wrapper->output); }

private:
  std::shared_ptr<pepp::Test> _wrapper;
};

class FragmentWrapper : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString name READ name CONSTANT);
  Q_PROPERTY(bool isDefault READ isDefault CONSTANT)
  Q_PROPERTY(bool isHidden READ isHidden CONSTANT)
  Q_PROPERTY(QString copyType READ copyType CONSTANT);
  Q_PROPERTY(QString language READ language CONSTANT);
  Q_PROPERTY(QString content READ content CONSTANT);

public:
  FragmentWrapper(std::shared_ptr<pepp::Fragment> wrapper) : QObject(nullptr), _wrapper(wrapper) {}
  ~FragmentWrapper() = default;
  pepp::Fragment *underlying() { return _wrapper.get(); }
  const pepp::Fragment *underlying() const { return _wrapper.get(); }

  QString name() const { return QString::fromStdString(_wrapper->name); }
  bool isHidden() const { return _wrapper->is_hidden; }
  bool isDefault() const { return _wrapper->is_default; }
  QString copyType() const { return QString::fromStdString(_wrapper->copy_type); }
  QString language() const { return QString::fromStdString(_wrapper->language); }
  QString content() const { return QString::fromStdString(_wrapper->contents()); }

protected:
  std::shared_ptr<pepp::Fragment> _wrapper;
};

/*!
 * \brief Represents a single figure in a textbook
 *
 * A figure is composed of multiple textual fragments, and may include a set of
 * test input:output pairs.
 *
 * This class is meant to be usable in both C++ and QML, so some Q_PROPERTYs
 * have a public API as a variant, but also provide a typesafe API for C++.
 * \see builtins::Figure#_tests \see builtins::Figure#_namedFragments
 *
 */
class FigureWrapper : public QObject {
  Q_OBJECT
  Q_PROPERTY(int arch READ qml_arch CONSTANT);
  Q_PROPERTY(int level READ qml_level CONSTANT);
  Q_PROPERTY(int features READ qml_features CONSTANT);
  Q_PROPERTY(QString prefix READ prefix CONSTANT);
  Q_PROPERTY(QString chapterName READ chapterName CONSTANT);
  Q_PROPERTY(QString figureName READ figureName CONSTANT);
  Q_PROPERTY(QString description READ description CONSTANT);
  Q_PROPERTY(bool isOS READ isOS CONSTANT);
  Q_PROPERTY(bool isHidden READ isHidden CONSTANT);
  Q_PROPERTY(bool isProblem READ isProblem CONSTANT);

  // Must use variants if we want these to be accessed from QML.
  // We provide a type safe version, which should be used instead if in C++.
  // See builtins::Test for properties
  Q_PROPERTY(QVariantList tests READ tests CONSTANT);
  // See builtins::Element for available properties
  Q_PROPERTY(QVariantMap fragments READ namedFragments CONSTANT);
  Q_PROPERTY(QString defaultFragmentName READ defaultFragmentName CONSTANT);

public:
  FigureWrapper(std::shared_ptr<const pepp::Figure> wrapper) : QObject(nullptr), _wrapper(wrapper) {}
  ~FigureWrapper() = default;
  const pepp::Figure *underlying() const { return _wrapper.get(); }
  std::shared_ptr<const pepp::Figure> underlying_shared() const { return _wrapper; }

  pepp::Architecture arch() const { return _wrapper->arch(); }
  int qml_arch() const { return static_cast<int>(_wrapper->arch()); }
  pepp::Abstraction level() const { return _wrapper->level(); }
  int qml_level() const { return static_cast<int>(_wrapper->level()); }
  pepp::Features features() const { return _wrapper->features(); }
  int qml_features() const { return static_cast<int>(_wrapper->features()); }

  QString prefix() const { return QString::fromStdString(_wrapper->name_prefix()); }
  QString chapterName() const { return QString::fromStdString(_wrapper->name_chapter()); }
  QString figureName() const { return QString::fromStdString(_wrapper->name_figure()); }
  bool isProblem() const { return _wrapper->is_problem(); }

  QString description() const { return QString::fromStdString(_wrapper->description()); }

  bool isOS() const { return _wrapper->is_os(); }

  bool isHidden() const { return _wrapper->is_hidden(); }

  QString defaultFragmentName() const { return QString::fromStdString(_wrapper->default_fragment_name()); }
  Q_INVOKABLE QString defaultFragmentText() const { return QString::fromStdString(_wrapper->default_fragment_text()); }
  Q_INVOKABLE QString defaultOSText() const { return QString::fromStdString(_wrapper->default_os_text()); }

  // All of the following create datastructures on the fly -- please limit calls
  // If you want access to the non-type-erased properties, use the appropriate methods on underlying().
  QVariantList tests() const;
  QVariantMap namedFragments() const;

private:
  std::shared_ptr<const pepp::Figure> _wrapper;
};

class BookWrapper : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString name READ name CONSTANT);
  Q_PROPERTY(const QVariantList figures READ figures CONSTANT);
  Q_PROPERTY(const QVariantList macros READ macros CONSTANT);

public:
  explicit BookWrapper(std::shared_ptr<const pepp::Book> wrapper) : QObject(nullptr), _wrapper(std::move(wrapper)) {}
  ~BookWrapper() = default;

  const pepp::Book *underlying() const { return _wrapper.get(); }

  QString name() const { return QString::fromStdString(_wrapper->name()); }
  const QVariantList figures() const;
  const QVariantList macros() const;

private:
  std::shared_ptr<const pepp::Book> _wrapper;
  // Cache the return values of figures() and macros() to avoid repeated allocations on the QML side.
  // This is safe because book contents do not change dynamically.
  mutable QVariantList _figures, _macros;
};

static const char *default_book_path = ":/books";
struct QtFilesystemProvider : public pepp::BuiltinRegistry::FilesystemProvider {
  QtFilesystemProvider(QString base_path = default_book_path);
  std::string join(const std::string &path, const std::string &parent) const override;
  std::string dir_of(const std::string &path) const override;
  std::string read_file(const std::string &path) const override;
  std::vector<std::string> enumerate_files(const std::string &directory) const override;
  bool using_external_figures() const override;
  static std::unique_ptr<QtFilesystemProvider> create(QString base_path = default_book_path) {
    return std::make_unique<QtFilesystemProvider>(base_path);
  }

private:
  static QString resolve(const QString &base, const QString &path);
  QString _base;
};

} // end namespace builtins
Q_DECLARE_METATYPE(builtins::MacroWrapper);
Q_DECLARE_METATYPE(builtins::FragmentWrapper);
Q_DECLARE_METATYPE(builtins::FigureWrapper);
Q_DECLARE_METATYPE(builtins::BookWrapper);
