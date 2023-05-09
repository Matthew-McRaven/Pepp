#include "./asm/asm.hpp"
#include "./get/fig.hpp"
#include "./get/macro.hpp"
#include "./ls.hpp"
#include "./run.hpp"
#include "./task.hpp"
#include <CLI11.hpp>
#include <QDebug>
#include <QtCore>

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
    if (chOpt->count() > 0)
      task = [&](QObject *parent) {
        return new GetFigTask(edValue, ch, fig, parent);
      };
    else if (macroOpt->count() > 0)
      task = [&](QObject *parent) {
        return new GetMacroTask(edValue, macro, parent);
      };
  });

  auto asmSC = app.add_subcommand("asm", "Assemble stuff");
  std::string userName, osListing;
  std::optional<std::string> osName = std::nullopt, elfName = std::nullopt;
  auto osOpt = asmSC->add_option("--os", osName);
  auto elfOpt = asmSC->add_option("--elf", elfName);
  auto osListingOpt = asmSC->add_option("--os-listing", osListing);
  auto userOpt = asmSC->add_option("user", userName)->required()->expected(1);
  asmSC->callback([&]() {
    task = [&](QObject *parent) {
      auto ret = new AsmTask(edValue, userName, "a.pepo", parent);
      if (osOpt)
        ret->setOsFname(*osName);
      if (elfOpt)
        ret->emitElfTo(*elfName);
      if (osListingOpt)
        ret->setOsListingFname(osListing);
      return ret;
    };
  });

  std::string elfIn, charIn, charOut, memDump;
  uint64_t maxSteps;
  ELFIO::elfio elf;
  auto runSC = app.add_subcommand("run", "Run ISA3 programs");
  auto charInOpt = runSC->add_option(
      "-i,--charIn", charIn,
      "File whose contents are to be buffered behind charIn. The value `-` "
      "will cause charIn to be taken from stdin. When using `-`, failure to "
      "provide stdin will cause program to freeze.");
  auto charOutOpt =
      runSC
          ->add_option("-o,--charOut", charOut,
                       "File to which the contents of charOut will be written. "
                       "The value `-` specifies stdout")
          ->default_val("-");
  auto memDumpOpt = runSC->add_option(
      "--mem-dump", memDump,
      "File to which post-simulation memory-dump will be written.");
  auto elfInOpt = runSC->add_option("elf", elfIn)->required()->expected(1);
  auto maxStepsOpt =
      runSC
          ->add_option("--max,-m", maxSteps,
                       "Maximum number of instructions that will be executed "
                       "before terminating simulator.")
          ->default_val(10'000);
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
