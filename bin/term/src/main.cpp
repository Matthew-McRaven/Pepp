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

#include "./asm/asm.hpp"
#include "./get/fig.hpp"
#include "./get/macro.hpp"
#include "./ls.hpp"
#include "./run.hpp"
#include "./task.hpp"
#include "throughput.hpp"
#include <CLI11.hpp>
#include <QDebug>
#include <QtCore>

using task_factory_t = std::function<Task *(QObject *)>;
#include "main.moc"

int main(int argc, char **argv) {
  CLI::App app{"Pepp Terminal", "pepp"};
  app.set_help_flag("-h,--help", "Display this help message and exit.");

  int edValue = 6;
  auto ed = app.add_flag("-e,--edition", edValue,
                         "Which edition of Computer Systems to target. "
                         "Possible values are 4, 5, and 6.")
                ->default_val(6)
                ->expected(4, 6);
  task_factory_t task;

  auto list = app.add_subcommand("ls", "Produce list of figures and macros");
  list->callback([&]() {
    task = [&](QObject *parent) { return new ListTask(edValue, parent); };
  });

  auto get = app.add_subcommand("get", "Fetch the body of a figure or macro");
  auto get_selector = get->add_option_group("")->required();
  auto get_figure = get_selector->add_option_group("[--ch]");
  std::string ch, fig, macro, type, prob;
  auto chOpt = get_figure->add_option("--ch", ch, "")->required();
  auto get_item = get_figure->add_option_group("[--fig, --prob]");
  auto figOpt = get_item->add_option("--fig", fig, "");
  auto probOpt = get_item->add_option("--prob", prob, "");
  auto typeOpt = get_figure->add_option("--type", type, "")->default_val("pep");
  auto macroOpt = get_selector->add_option("--macro", macro, "");
  get_selector->require_option(1);
  get_item->require_option(1);
  get->callback([&]() {
    if (chOpt->count() > 0)
      task = [&](QObject *parent) {
        if (*figOpt)
          return new GetFigTask(edValue, ch, fig, type, true, parent);
        else
          return new GetFigTask(edValue, ch, fig, type, false, parent);
      };
    else if (macroOpt->count() > 0)
      task = [&](QObject *parent) {
        return new GetMacroTask(edValue, macro, parent);
      };
  });

  auto asmSC = app.add_subcommand("asm", "Assemble stuff");
  bool bm;
  std::string userName, osListing, pepoOut, errOut;
  std::optional<std::string> osName = std::nullopt, elfName = std::nullopt;
  std::list<std::string> macroDirs;
  auto osOpt = asmSC->add_option("--os", osName);
  CLI::Option *bmAsmOpt = nullptr;
  if (edValue == 6)
    bmAsmOpt =
        asmSC->add_flag("--bm", bm, "Use bare metal OS.")->excludes(osOpt);
  auto elfOpt = asmSC->add_option("--elf", elfName);
  auto pepoOpt = asmSC->add_option("-o", pepoOut);
  auto errOpt = asmSC->add_option("-e", errOut);
  auto osListingOpt = asmSC->add_option("--os-listing", osListing);
  auto macroDirOpts = asmSC->add_option("--md,--macro-dir", macroDirs);

  asmSC->add_option("-s,user", userName)->required()->expected(1);
  asmSC->callback([&]() {
    task = [&](QObject *parent) {
      auto ret = new AsmTask(edValue, userName, parent);
      if (bmAsmOpt && *bmAsmOpt)
        ret->setBm(bm);
      else if (*osOpt)
        ret->setOsFname(*osName);
      if (*elfOpt)
        ret->emitElfTo(*elfName);
      if (*osListingOpt)
        ret->setOsListingFname(osListing);
      if (*errOpt)
        ret->setErrName(errOut);
      if (*pepoOpt)
        ret->setPepoName(pepoOut);
      if (*macroDirOpts)
        ret->setMacroDirs(macroDirs);
      return ret;
    };
  });

  bool skipLoad, skipDispatch /*,bm comes from asm*/;
  std::string objIn, charIn, charOut, memDump, osIn;
  uint64_t maxSteps;
  std::map<std::string, quint64> regOverrides;
  auto runSC = app.add_subcommand("run", "Run ISA3 programs");
  auto charInOpt = runSC->add_option(
      "-i,--charIn", charIn,
      "File whose contents are to be buffered behind charIn. The value `-` "
      "will cause charIn to be taken from stdin. When using `-`, failure to "
      "provide stdin will cause program to freeze.");
  runSC
      ->add_option("-o,--charOut", charOut,
                   "File to which the contents of charOut will be written. "
                   "The value `-` specifies stdout")
      ->default_val("-");
  auto memDumpOpt = runSC->add_option(
      "--mem-dump", memDump,
      "File to which post-simulation memory-dump will be written.");
  runSC->add_option("-s,obj", objIn)->required()->expected(1);
  runSC
      ->add_option("--max,-m", maxSteps,
                   "Maximum number of instructions that will be executed "
                   "before terminating simulator.")
      ->default_val(125'000);
  auto osInOpt =
      runSC->add_option("--os", osIn, "File from which os will be read.");
  CLI::Option *bmRunOpt = nullptr;
  if (edValue == 6)
    bmRunOpt =
        runSC->add_flag("--bm", bm, "Use bare metal OS.")->excludes(osInOpt);
  if (edValue == 6) {
    runSC->add_flag("--skip-load", skipLoad)->group("");
    runSC->add_flag("--skip-dispatch", skipDispatch)->group("");
  }
  auto regOverrideOpt = runSC->add_option("--reg", regOverrides)->group("");
  runSC->callback([&]() {
    task = [&](QObject *parent) {
      auto ret = new RunTask(edValue, objIn, parent);
      if (*charInOpt)
        ret->setCharIn(charIn);
      ret->setCharOut(charOut);
      if (*memDumpOpt)
        ret->setMemDump(memDump);
      if (bmRunOpt && *bmRunOpt)
        ret->setBm(bm);
      else if (*osInOpt)
        ret->setOsIn(osIn);
      ret->setMaxSteps(maxSteps);
      ret->setSkipLoad(skipLoad);
      ret->setSkipDispatch(skipDispatch);
      for (auto &reg : regOverrides)
        ret->addRegisterOverride(reg.first, reg.second);
      return ret;
    };
  });

  auto instrThruSC =
      app.add_subcommand("mit", "Measure instruction throughput");
  instrThruSC->group("");
  instrThruSC->callback([&task]() {
    task = [](QObject *parent) { return new ThroughputTask(parent); };
  });
  try {
    app.parse(argc, argv);
    if (!task)
      throw CLI::CallForHelp();
  } catch (const CLI::CallForHelp &e) {
    std::cout << app.help() << std::endl;
    return 0;
  } catch (const CLI::ParseError &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  QCoreApplication a(argc, argv);
  auto taskInstance = task(&a);
  QObject::connect(taskInstance, &Task::finished, &a, QCoreApplication::exit);
  QTimer::singleShot(0, taskInstance, &Task::run);
  return a.exec();
}

Task::Task(QObject *parent) : QObject(parent) {}
