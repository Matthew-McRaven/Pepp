#include "dependencies.hpp"
#include "read.hpp"
#include <QDebug>

QList<about::Dependency> about::dependencies() {
  auto depText = detail::readFile(":/about/dependencies.csv");
  if (!depText.has_value())
    return {};
  QList<about::Dependency> ret;
  bool first = true;
  for (auto &line : depText->split("\n")) {
    // First line is headers, skip.
    if (first) {
      first = false;
      continue;
    } else if (line.isEmpty())
      continue;
    // There should be 5 headers: name, url, license name, license SPDX ID,
    // license text file.
    auto parts = line.split(",");
    if (parts.size() != 5) {
      qWarning() << "Failed to parse dependency row: " << line << "\n";
      return {};
    }
    auto lineText = detail::readFile(parts[4]);
    if (!lineText.has_value())
      return {};
    ret.push_back(about::Dependency{.name = parts[0],
                                    .url = parts[1],
                                    .licenseName = parts[2],
                                    .licenseSPDXID = parts[3],
                                    .licenseText = *lineText});
  }
  return ret;
}
