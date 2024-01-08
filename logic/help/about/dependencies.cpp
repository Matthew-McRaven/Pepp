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
    // There should be 6 headers: name, url, license name, license SPDX ID,
    // license text file, and a flag for if development dependency.
    auto parts = line.split(",");
    if (parts.size() != 6) {
      qWarning() << "Failed to parse dependency row: " << line << "\n";
      return {};
    }

    // Parse devDependency to flag. Any non-zero should set the flag
    bool flag;
    bool v = !(parts[5].toInt(&flag) == 0);
    if(!flag) qWarning() << "Failed to parse devDependency as int: " << parts[5] << "\n";

    auto lineText = detail::readFile(parts[4]);
    if (!lineText.has_value())
      return {};
    ret.push_back(about::Dependency{.name = parts[0],
                                    .url = parts[1],
                                    .licenseName = parts[2],
                                    .licenseSPDXID = parts[3],
                                    .licenseText = *lineText,
                                    .devDependency=flag?v:false});
  }
  return ret;
}
