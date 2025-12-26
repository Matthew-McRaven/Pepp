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

#include "catch.hpp"
#include "config.hpp"

TEST_CASE("Terminal sample applications", "[term][cli]") {
  const auto path = term_path();
  auto sample_dirs = QDir(":/samples");
  for (auto sample_dirname : sample_dirs.entryList()) {
    auto sample_dirpath = sample_dirs.path() + "/" + sample_dirname;
    auto sample_dir = QDir(sample_dirpath);

    bool in_pep = false, out_pepo = false, in_txt = false, out_txt = false;
    DYNAMIC_SECTION(sample_dirname.toStdString()) {
      QTemporaryDir dir;
      if (in_pep = sample_dir.exists("in.pep"); in_pep)
        QFile::copy(sample_dir.filePath("in.pep"), dir.filePath("in.pep"));
      if (out_pepo = sample_dir.exists("out.pepo"); out_pepo)
        QFile::copy(sample_dir.filePath("out.pepo"), dir.filePath("ref_out.pepo"));
      if (in_txt = sample_dir.exists("in.txt"); in_txt)
        QFile::copy(sample_dir.filePath("in.txt"), dir.filePath("in.txt"));
      if (out_txt = sample_dir.exists("out.txt"); out_txt)
        QFile::copy(sample_dir.filePath("out.txt"), dir.filePath("ref_out.txt"));

      // Assemble and compare object code outputs.
      if (in_pep && out_pepo) {
        QProcess term;
        term.setWorkingDirectory(dir.path());
        term.start(path, {
                             "asm",
                             "-s",
                             "in.pep",
                             "-o",
                             "out.pepo",
                         });
        wait_return(term, 0);

        QStringList result;
        for (auto fname : {dir.filePath("out.pepo"), dir.filePath("ref_out.pepo")}) {
          auto f = QFile(fname);
          REQUIRE(f.open(QIODevice::ReadOnly));
          auto text = f.readAll();
          text.replace("\r", "");
          result.push_back(text);
        }
        CHECK(result[0].toStdString() == result[1].toStdString());
      }
      // Assemble and execute, comparing output text.
      else if (in_pep && out_txt) {
        {
          QProcess term;
          term.setWorkingDirectory(dir.path());
          term.start(path, {
                               "asm",
                               "-s",
                               "in.pep",
                               "--elf",
                               "in.elf",
                           });
          wait_return(term, 0);
        }
        {
          QProcess term;
          term.setWorkingDirectory(dir.path());
          if (in_txt)
            term.start(path, {"run", "-s", "in.elf", "-o", "out.txt", "-i", "in.txt"});
          else
            term.start(path, {
                                 "run",
                                 "-s",
                                 "in.elf",
                                 "-o",
                                 "out.txt",
                             });
          wait_return(term, 0);
          QStringList result;
          for (auto fname : {dir.filePath("out.txt"), dir.filePath("ref_out.txt")}) {
            auto f = QFile(fname);
            REQUIRE(f.open(QIODevice::ReadOnly));
            auto text = f.readAll();
            text.replace("\r", "");
            result.push_back(text);
          }
          CHECK(result[0].toStdString() == result[1].toStdString());
        }
      } else
        REQUIRE(false);
    }
  }
}
