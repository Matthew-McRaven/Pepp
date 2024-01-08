#include "pepp.hpp"
#include "help/about/version.hpp"
#include "read.hpp"

QString about::projectRepoURL() {
  auto text = about::detail::readFile(":/about/repo.url");
  if (!text.has_value()) {
    qWarning() << "Unable to read repo URL.\n";
    return {};
  }
  return *text;
}

QList<about::Maintainer> about::maintainers() {
  QList<Maintainer> ret;
  auto text = detail::readFile(":/about/maintainers.csv");
  // maintainers.csv has no headers
  for (const auto &line : text->split("\n")) {
    // Skip empty lines.
    if (line.isEmpty())
      continue;
    auto parts = line.split(",");
    if (parts.size() != 2) {
      qWarning() << "Failed to parse maintainers row: " << line << "\n";
      return {};
    }
    ret.push_back(about::Maintainer{.name = parts[0], .email = parts[1]});
  }
  return ret;
}

QList<QString> about::contributors() {
  QList<QString> ret;
  auto text = detail::readFile(":/about/contributors.csv");
  // maintainers.csv has no headers
  for (const auto &line : text->split("\n")) {
    // Skip empty lines.
    if (line.isEmpty())
      continue;
    ret.push_back(line);
  }
  return ret;
}

QString about::licenseFull() {
  auto text = detail::readFile(":/about/LICENSE_FULL");
  return text.value_or("");
}

QString about::licenseNotice() {
  auto text = detail::readFile(":/about/LICENSE_NOTICE");
  return text.value_or("");
}

QString about::versionString() {
  return u"%1.%2.%3"_qs.arg(about::g_MAJOR_VERSION).arg(about::g_MINOR_VERSION).arg(about::g_PATCH_VERSION);
}
