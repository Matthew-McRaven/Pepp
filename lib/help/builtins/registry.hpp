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

#include <QObject>

// Needed to prevent type_traits from complaining that Book has throwing dtor.
#include "book.hpp"
#include "project/architectures.hpp"
#include "project/levels.hpp"
namespace macro {
class Declaration;
}

namespace builtins {
class Test;
class Figure;
class Fragment;
static const char *default_book_path = ":/books";

class Registry {
public:
  struct Assembler {
    virtual ~Assembler() = default;
    virtual QVariant operator()(const QString &os, const QString &user) = 0;
  };
  struct Formatter {
    virtual ~Formatter() = default;
    virtual QString operator()(QVariant assembled) = 0;
  };
  struct FilesystemProvider {
    virtual ~FilesystemProvider() = default;
    virtual QString readFile(const QString &path) = 0;
    virtual QStringList enumerateFiles(const QString &directory) = 0;
    // Return true if these are not the "compiled in" figures.
    virtual bool using_external_figures() const = 0;
  };
  // Crawling the Qt help system to create books is handled inside CTOR.
  explicit Registry(std::unique_ptr<FilesystemProvider> fs);
  QList<QSharedPointer<const builtins::Book>> books() const;
  QSharedPointer<const builtins::Book> findBook(QString name) const;
  bool usingExternalFigures() const { return _fs->using_external_figures(); }
  void addDependency(const Fragment *dependent, const Fragment *dependee);
  QString contentFor(Fragment &fragment);
  void addAssembler(pepp::Architecture arch, std::unique_ptr<Assembler> &&assembler);
  void addFormatter(pepp::Architecture arch, QString format, std::unique_ptr<Formatter> &&formatter);

private:
  using _Figure = QSharedPointer<builtins::Figure>;
  using _Macro = QList<QSharedPointer<macro::Declaration>>;
  std::variant<std::monostate, _Figure, _Macro> loadManifestV2(const QJsonDocument &manifest, const QString &path);
  std::variant<std::monostate, _Figure, _Macro> loadFigureV2(const QJsonDocument &manifest, const QString &path);
  std::variant<std::monostate, _Figure, _Macro> loadMacroV2(const QJsonDocument &manifest, const QString &path);

  void linkFigureOS(const QString &manifestPath, QSharedPointer<Figure> figure,
                    QSharedPointer<const builtins::Book> book);
  ::builtins::Test *loadTest(QString testDirPath);
  ::builtins::Fragment *loadFragment(const QJsonObject &item, const QDir &manifestDir, builtins::Figure *parent);

  QSharedPointer<::builtins::Book> loadBook(QString tocPath);
  std::unique_ptr<FilesystemProvider> _fs;
  QList<QSharedPointer<const builtins::Book>> _books;
  // Given an element, determine which element it depends on.
  QMap<const Fragment * /*dependent*/, const Fragment * /*dependee*/> _dependencies;
  // Given an element, determine which elements depend on it.
  QMap<const Fragment * /*dependee*/, QList<const Fragment *> /*dependents*/> _dependees;
  void computeDependencies(const Fragment *dependee);
  QMap<const Fragment *, QString> _contents;
  // Use std::map so that unique pointers are less painful. QMap COW features do not interact well.
  std::map<pepp::Architecture, std::unique_ptr<Assembler>> _assemblers;
  std::map<QPair<pepp::Architecture, QString>, std::unique_ptr<Formatter>> _formatters;
};

namespace detail {} // end namespace detail

class QRCFSProvider : public Registry::FilesystemProvider {
public:
  explicit QRCFSProvider(QString prefix) : _prefix(std::move(prefix)) {}
  QString readFile(const QString &path) override;
  QStringList enumerateFiles(const QString &directory) override;
  bool using_external_figures() const override;

private:
  QString resolve(QString path) const;
  QString _prefix;
};
std::unique_ptr<Registry::FilesystemProvider> makeQRCFSProvider(QString prefix = default_book_path);
} // end namespace builtins
