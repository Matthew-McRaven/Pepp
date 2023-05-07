#include "./run.hpp"
#include "./task.hpp"
#include "builtins/book.hpp"
#include "builtins/figure.hpp"
#include "builtins/registry.hpp"
#include "isa/pep10.hpp"
#include "macro/registry.hpp"
#include "pas/driver/pep10.hpp"
#include "pas/obj/pep10.hpp"
#include "pas/operations/generic/errors.hpp"
#include "pas/operations/pepp/bytes.hpp"
#include <CLI11.hpp>
#include <QDebug>
#include <QtCore>

class GetFigTask : public Task {
public:
  GetFigTask(int ed, std::string ch, std::string fig,
             QObject *parent = nullptr);
  void run() override;

private:
  int ed;
  std::string ch, fig;
};

class AsmTask : public Task {
public:
  AsmTask(int ed, std::string userFname, std::string out,
          QObject *parent = nullptr);
  void setOsFname(std::string fname);
  void emitElfTo(std::string fname);
  void run() override;

private:
  int ed;
  std::string userIn, pepoOut;
  std::optional<std::string> osIn, peplOut, elfOut;
};

using task_factory_t = std::function<Task *(QObject *)>;
#include "main.moc"

int main(int argc, char **argv) {
  CLI::App app{"Magic app", "pepp"};
  app.set_help_flag("-h,--help", "test");
  // auto help = app.add_flag("-h,--help");
  app.add_flag("-f", "test");

  int edValue = 6;
  auto ed =
      app.add_flag(
             "-e,--edition", edValue,
             "Which book edition to target. Possible values are 4, 5, and 6.")
          ->default_val(6)
          ->expected(4, 6);
  task_factory_t task;

  auto list = app.add_subcommand("ls", "Produce list of figures and macros");
  list->callback([&]() {
    task = [&](QObject *parent) { return new ListTask(edValue, parent); };
  });

  auto get = app.add_subcommand("get", "Fetch the body of a figure or macro");
  auto get_selector = get->add_option_group("")->required();
  auto get_figure = get_selector->add_option_group("[--ch, --fig]");
  std::string ch, fig, macro;
  auto chOpt = get_figure->add_option("--ch", ch, "")->required();
  auto figOpt = get_figure->add_option("--fig", fig, "")->required();
  auto macroOpt = get_selector->add_option("--macro", macro, "");
  get_selector->require_option(1);
  get->callback([&]() {
    if (chOpt->count() > 0) {
      task = [&](QObject *parent) {
        return new GetFigTask(edValue, ch, fig, parent);
      };

    } else
      throw std::logic_error("agh");
  });

  auto asmSC = app.add_subcommand("asm", "Assemble stuff");
  std::string userName;
  std::optional<std::string> osName = std::nullopt, elfName = std::nullopt;
  auto osOpt = asmSC->add_option("--os", osName);
  auto elfOpt = asmSC->add_option("--elf", elfName)->default_val("a.elf");
  auto userOpt = asmSC->add_option("user", userName)->required()->expected(1);
  asmSC->callback([&]() {
    task = [&](QObject *parent) {
      auto ret = new AsmTask(edValue, userName, "a.pepo", parent);
      if (osOpt)
        ret->setOsFname(*osName);
      ret->emitElfTo(elfOpt->as<std::string>());
      return ret;
    };
  });

  std::string elfIn, charIn, charOut, memDump;
  uint64_t maxSteps;
  ELFIO::elfio elf;
  auto runSC = app.add_subcommand("run", "Run ISA3 programs");
  auto charInOpt = runSC->add_option("--charIn", charIn)->default_val("-");
  auto charOutOpt = runSC->add_option("--charOut", charOut)->default_val("-");
  auto memDumpOpt =
      runSC->add_option("--memDump", memDump)->default_val("mem.bin");
  auto elfInOpt = runSC->add_option("elf", elfIn)->required()->expected(1);
  auto maxStepsOpt =
      runSC->add_option("--max,-m", maxSteps)->default_val(10'000);
  runSC->callback([&]() {
    task = [&](QObject *parent) {
      elf.load(elfIn);
      auto ret = new RunTask(elf, parent);
      if (charInOpt)
        ret->setCharIn(charIn);
      if (charOutOpt)
        ret->setCharOut(charOut);
      if (memDumpOpt)
        ret->setMemDump(memDump);
      ret->setMaxSteps(maxSteps);
      return ret;
    };
  });

  try {
    app.parse(argc, argv);
    if (!task)
      throw CLI::CallForHelp();
  } catch (const CLI::CallForHelp &e) {
    std::cout << app.help();
    return 0;
  } catch (const CLI::ParseError &e) {
    std::cerr << e.what();
    return 1;
  }

  QCoreApplication a(argc, argv);
  auto taskInstance = task(&a);
  QObject::connect(taskInstance, &Task::finished, &a, QCoreApplication::exit);
  QTimer::singleShot(0, taskInstance, &Task::run);
  return a.exec();
}

Task::Task(QObject *parent) : QObject(parent) {}

ListTask::ListTask(int ed, QObject *parent) : Task(parent), ed(ed) {}

void ListTask::run() {
  QString bookName;
  switch (ed) {
  case 6:
    bookName = "Computer Systems, 6th Edition";
  default:
    emit finished(1);
  }

  auto reg = builtins::Registry(nullptr);
  auto book = reg.findBook(bookName);

  if (book.isNull())
    emit finished(1);

  auto figures = book->figures();
  auto macros = book->macros();

  std::cout << "Figures: " << std::endl;
  for (auto &figure : figures)
    std::cout << u"%1.%2"_qs.arg(figure->chapterName(), figure->figureName())
                     .toStdString()
              << std::endl;

  std::cout << std::endl;

  std::cout << "Macros: " << std::endl;
  for (auto &macro : macros)
    std::cout
        << u"%1 %2"_qs.arg(macro->name()).arg(macro->argCount()).toStdString()
        << std::endl;

  emit finished(0);
}

GetFigTask::GetFigTask(int ed, std::string ch, std::string fig, QObject *parent)
    : Task(parent), ed(ed), ch(ch), fig(fig) {}

void GetFigTask::run() {
  QString bookName;
  switch (ed) {
  case 6:
    bookName = "Computer Systems, 6th Edition";
  default:
    emit finished(1);
  }

  auto reg = builtins::Registry(nullptr);
  auto book = reg.findBook(bookName);

  auto figure =
      book->findFigure(QString::fromStdString(ch), QString::fromStdString(fig));
  if (figure.isNull())
    return emit finished(1);
  if (!figure->typesafeElements().contains("pep"))
    return emit finished(2);

  auto body = figure->typesafeElements()["pep"]->contents;
  std::cout << body.toStdString() << std::endl;

  emit finished(0);
}

AsmTask::AsmTask(int ed, std::string userFname, std::string out,
                 QObject *parent)
    : Task(parent), ed(ed), userIn(userFname), pepoOut(out) {}

void AsmTask::setOsFname(std::string fname) { osIn = fname; }

void AsmTask::emitElfTo(std::string fname) { elfOut = fname; }

void AsmTask::run() {
  auto bookRegistry = builtins::Registry(nullptr);
  auto macroRegistry = QSharedPointer<macro::Registry>::create();
  auto book = bookRegistry.findBook("Computer Systems, 6th Edition");
  for (auto &macro : book->macros())
    macroRegistry->registerMacro(macro::types::Core, macro);

  std::ifstream uIn(userIn);
  std::stringstream uBuf;
  uBuf << uIn.rdbuf();
  std::string userContents = uBuf.str();
  // If no OS, default to full.
  std::string osContents;
  if (osIn->empty()) {
    auto os = book->findFigure("os", "full");
    osContents = os->typesafeElements()["pep"]->contents.toStdString();
  } else {
    std::ifstream _osIn(osIn.value());
    std::stringstream osBuf;
    osBuf << _osIn.rdbuf();
    osContents = osBuf.str();
  }
  auto pipeline = pas::driver::pep10::pipeline(
      {{QString::fromStdString(osContents), {.isOS = true}},
       {QString::fromStdString(userContents), {.isOS = false}}},
      macroRegistry);
  auto result = pipeline->assemble(pas::driver::pep10::Stage::End);

  auto osTarget = pipeline->pipelines[0].first;
  auto osRoot = osTarget->bodies[pas::driver::repr::Nodes::name]
                    .value<pas::driver::repr::Nodes>()
                    .value;
  auto userTarget = pipeline->pipelines[1].first;
  auto userRoot = userTarget->bodies[pas::driver::repr::Nodes::name]
                      .value<pas::driver::repr::Nodes>()
                      .value;
  if (!result) {
    auto osErrors = pas::ops::generic::collectErrors(*osRoot);
    for (auto &err : osErrors)
      std::cerr << err.first.value.line << err.second.message.toStdString()
                << std::endl;
    auto userErrors = pas::ops::generic::collectErrors(*userRoot);
    for (auto &err : userErrors)
      std::cerr << err.first.value.line << err.second.message.toStdString()
                << std::endl;
    return emit finished(1);
  }
  auto elf = pas::obj::pep10::createElf();
  pas::obj::pep10::combineSections(*osRoot);
  pas::obj::pep10::writeOS(elf, *osRoot);
  pas::obj::pep10::combineSections(*userRoot);
  pas::obj::pep10::writeUser(elf, *userRoot);
  if (elfOut.has_value()) {
    elf.save(elfOut.value());
  }
  qDebug() << pas::ops::pepp::toBytes<isa::Pep10>(*userRoot);
  emit finished(0);
}
