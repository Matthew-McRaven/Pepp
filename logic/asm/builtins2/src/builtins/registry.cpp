#include "registry.hpp"
#include "book.hpp"
#include "elements.hpp"
#include "figure.hpp"
#include <QDir>
#include <QDirIterator>
#include <QJsonDocument>
#include <QStringConverter>
builtins::Registry::Registry(void *asm_toolchains) : QObject(nullptr) {
  for (const auto &bookPath : detail::enumerateBooks(":/books")) {
    auto book = detail::loadBook(bookPath);
    if (book == nullptr)
      qFatal("Failed to load book");
    else if (findBook(book->name()) != nullptr)
      qFatal("Duplicate book");
    _books.push_back(book);
  }
}

QList<QSharedPointer<const builtins::Book>> builtins::Registry::books() const {
  return _books;
}

QSharedPointer<const builtins::Book>
builtins::Registry::findBook(QString name) {
  QList<QSharedPointer<const builtins::Book>> temp;
  for (auto &bookPtr : _books) {
    if (bookPtr->name() == name) {
      temp.push_back(bookPtr);
    }
  }
  if (auto length = temp.length(); length == 0)
    return nullptr;
  else if (length == 1)
    return temp.first();
  else {
    qDebug() << (u"More than one copy of book {}"_qs).arg(name);
    return nullptr;
  }
}

QByteArray read(QString path) {
  QFile asFile(path);
  asFile.open(QFile::ReadOnly);
  auto bytes = asFile.readAll();
  asFile.close();
  return bytes;
}

builtins::Element *builtins::detail::loadElement(QString elementPath) {
  auto element = new builtins::Element();
  QString data = read(elementPath);
  element->contents = data;
  element->generated = false;
  return element;
}

builtins::Element *builtins::detail::generateElement(QString fromElementPath,
                                                     void *asm_toolchains) {
  return nullptr;
}

builtins::Test *builtins::detail::loadTest(QString testDirPath) {
  auto test = new builtins::Test();
  QDirIterator dir(testDirPath);
  while (dir.hasNext()) {
    auto file = dir.next();
    QString data = read(file);
    if (file.endsWith("input.txt"))
      test->input = data;
    else if (file.endsWith("output.txt"))
      test->output = data;
  }
  return test;
}

QSharedPointer<builtins::Figure>
builtins::detail::loadFigure(QString manifestPath) {
  auto manifestDir = QFileInfo(manifestPath).dir();
  // Read figure manifest get field names;
  auto manifestBytes = read(manifestPath);
  auto manifest = QJsonDocument::fromJson(manifestBytes);
  QString chFig = manifest["name"].toString();
  if (chFig.indexOf(":") == -1)
    qFatal("Invalid figure name");
  auto chFigSplit = chFig.split(":");
  auto chapterName = chFigSplit[0];
  auto figureName = chFigSplit[1];
  auto archStr = manifest["arch"].toString();
  bool okay = false;
  auto archInt = QMetaEnum::fromType<builtins::Architecture>().keyToValue(
      archStr.toUpper().toStdString().data(), &okay);
  auto arch = static_cast<builtins::Architecture>(archInt);
  if (!okay)
    qFatal("Invalid architecture");
  auto figure =
      QSharedPointer<builtins::Figure>::create(arch, chapterName, figureName);

  // Add tests
  auto ios = manifest["ios"];
  auto iosArray = ios.toArray();
  for (auto ioDir : qAsConst(iosArray)) {
    auto io = loadTest(manifestDir.absoluteFilePath(ioDir.toString()));
    if (io == nullptr)
      qFatal("Invalid IO");
    figure->addTest(io);
  }

  // Add elements
  auto items = manifest["items"];
  auto itemsArray = items.toObject();
  auto itemsArrayKeys = itemsArray.keys();
  for (auto language : qAsConst(itemsArrayKeys)) {
    QString itemTemplatePath = itemsArray[language].toString();
    auto itemPath = itemTemplatePath.replace("{ch}", chapterName)
                        .replace("{fig}", figureName);
    auto item = loadElement(manifestDir.absoluteFilePath(itemPath));
    item->figure = figure;
    item->language = language;
    if (item == nullptr)
      qFatal("Invalid item");
    figure->addElement(language, item);
  }

  return figure;
}

QList<QSharedPointer<builtins::Macro>>
builtins::detail::loadMacro(QString manifestPath) {
  return {};
}

void builtins::detail::linkFigureOS(QString manifestPath,
                                    QSharedPointer<Figure> figure,
                                    QSharedPointer<const Book> book) {}

QSharedPointer<builtins::Book> builtins::detail::loadBook(QString tocPath) {

  // Read ToC to get book name
  auto tocBytes = read(tocPath);
  auto toc = QJsonDocument::fromJson(tocBytes);
  // And create a book object to stick figures in
  auto book =
      QSharedPointer<builtins::Book>::create(toc["bookName"].toString());

  QDirIterator iter(QFileInfo(tocPath).dir().absolutePath(),
                    QDirIterator::Subdirectories);
  QList<std::tuple<QString, QSharedPointer<builtins::Figure>>> revisit;
  while (iter.hasNext()) {
    auto next = iter.next();
    if (next.endsWith("figure.json")) {
      auto figure = loadFigure(next);
      if (figure == nullptr)
        qFatal("Failed to load figure");
      revisit.push_back({next, figure});
      book->addFigure(figure);
    } else if (next.endsWith("macro.json")) {
      auto macros = loadMacro(next);
      for (auto &macro : macros) {
        if (macro == nullptr)
          qFatal("Failed to load macro");
        book->addMacro(macro);
      }
    }
  }
  for (auto &[path, figure] : revisit)
    linkFigureOS(path, figure, book);

  return book;
}

QList<QString> builtins::detail::enumerateBooks(QString prefix) {
  QList<QString> ret;
  QDirIterator iter(prefix);
  while (iter.hasNext()) {
    auto next = iter.next();
    auto maybeManifest = QFile(QDir(next).filePath("toc.json"));
    if (maybeManifest.exists())
      ret.push_back(maybeManifest.fileName());
  }
  return ret;
}
