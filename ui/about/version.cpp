/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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

#include "version.hpp"
#include "help/about/version.hpp"

Version::Version(QObject *parent) : QObject(parent) {}
QString Version::git_sha() { return about::g_GIT_SHA1(); }
QString Version::git_tag() { return about::g_GIT_TAG(); }
bool Version::git_dirty() { return about::g_GIT_LOCAL_CHANGES(); }
int Version::version_major() { return about::g_MAJOR_VERSION(); }
int Version::version_minor() { return about::g_MINOR_VERSION(); }
int Version::version_patch() { return about::g_PATCH_VERSION(); }
QString Version::version_str_full() {
  return u"%1.%2.%3"_qs.arg(version_major()).arg(version_minor()).arg(version_patch());
}
