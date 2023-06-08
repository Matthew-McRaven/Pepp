#include "./about.hpp"
#include "about/dependencies.hpp"
#include "about/pepp.hpp"
#include "about/version.hpp"
#include <TextFlow.hpp>
#include <iostream>

AboutTask::AboutTask(QObject *parent) : Task(parent) {}

void AboutTask::run() {
  std::cout << u"Pepp Terminal, Version %1\nBased on commit %2\n\n"_qs
                   .arg(about::versionString())
                   .arg(about::g_GIT_SHA1)
                   .toStdString();
  std::cout << "Report issues or check for updates:\n";
  std::cout << "\t" << about::projectRepoURL().toStdString() << "\n\n";
  std::cout << "Authors:\n";
  for (const auto &maintainer : about::maintainers())
    std::cout << "\t" << maintainer.name.toStdString() << " <"
              << maintainer.email.toStdString() << ">\n";
  std::cout << "\nLicensing:\n";
  auto lines = TextFlow::Column(about::licenseNotice().toStdString());
  for (const auto &line : lines) {
    std::cout
        << "\t"
        << QString::fromStdString(line).replace("\n", "\n\t").toStdString()
        << "\n";
  }
  std::cout
      << "\n\n\tFor further licensing info, execute this program with the "
         "`license` subcommand.\n";
  return emit finished(0);
}
