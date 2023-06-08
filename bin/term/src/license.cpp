#include "./license.hpp"
#include "about/dependencies.hpp"
#include <TextFlow.hpp>
#include <iostream>

LicenseTask::LicenseTask(QObject *parent) : Task(parent) {}

void LicenseTask::run() {
  for (const auto &dependency : about::dependencies()) {
    std::cout << dependency.name.toStdString() << "\n";
    std::cout << "    URL: " << dependency.url.toStdString() << "\n";
    std::cout << "    License: " << dependency.licenseName.toStdString()
              << "\n";
    std::cout << "    License Text:\n";
    auto lines = TextFlow::Column(dependency.licenseText.toStdString());
    // License-injected newlines break formatting, so must manually substitute
    // in-place.
    for (const auto &line : lines)
      std::cout << "        "
                << QString::fromStdString(line)
                       .replace("\n", "\n        ")
                       .toStdString()
                << "\n";
    std::cout << "\n\n";
  }
  return emit finished(0);
}
