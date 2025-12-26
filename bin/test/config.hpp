/*
 * Copyright (c) 2024-2026 J. Stanley Warford, Matthew McRaven
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

#include <QtCore>
#include <catch.hpp>
inline QString term_path()
{
  auto dir = QCoreApplication::applicationDirPath();
  QDir d(dir);
#ifdef Q_OS_WIN32
  static const QString filename = "pepp-term.exe";
#else
  static const QString filename = "pepp-term";
#endif
  if (d.exists(filename)) return d.absoluteFilePath(filename);
  throw std::logic_error("Couldn't find terminal app, tried: " + d.absoluteFilePath(filename).toStdString());
}

inline void wait_return(QProcess &proc, auto exitCode = 0)
{
    REQUIRE(proc.waitForStarted());
    REQUIRE(proc.waitForFinished());
    auto actualCode = proc.exitCode();
    if (actualCode != exitCode) {
        qDebug() << proc.readAllStandardError();
        auto fmt = QStringLiteral("Process exited with unexpected exit status %1").arg(actualCode);
        FAIL(fmt.toStdString().c_str());
    }
}

// Replace platform-newlines with \n
inline QString out(QProcess &proc)
{
    auto out = proc.readAllStandardOutput();
    out.replace("\r", "");
    return out;
}
