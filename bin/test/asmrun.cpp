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

#include <catch.hpp>
#include "config.hpp"

TEST_CASE("Terminal, asmrun", "[term][cli]") {
  auto path = term_path();
  SECTION("Bare Metal, pepo") {
    QTemporaryDir dir;
    // Assemble
    {
      auto in = QFile(dir.filePath("in.pep"));
      REQUIRE(in.open(QIODevice::WriteOnly));
      in.write("SCALL 0,i\n");
      in.close();

      QProcess term;
      term.setWorkingDirectory(dir.path());
      term.start(path, {"asm",
                        "-s"
                        "in.pep",
                        "--bm", "-o", "out.pepo"});
      wait_return(term, 0);
    }
    // Execute
    {
      QProcess term;
      term.setWorkingDirectory(dir.path());
      term.start(path, {"run", "--bm", "-s", "out.pepo"});
      wait_return(term, 0);
      CHECK(out(term) == "Cannot use system calls in bare metal mode\n");
    }
  }

  SECTION("Bare Metal, elf") {
    QTemporaryDir dir;

    // Get OS, user sources
    {
      QProcess term;
      term.setWorkingDirectory(dir.path());
      term.start(path, {"get", "--ch", "os", "--fig", "pep10baremetal", "--type", "pep"});
      wait_return(term, 0);
      auto in_os = QFile(dir.filePath("os.pep"));
      REQUIRE(in_os.open(QIODevice::WriteOnly));
      in_os.write(out(term).toUtf8());
      in_os.close();

      auto in_user = QFile(dir.filePath("in.pep"));
      REQUIRE(in_user.open(QIODevice::WriteOnly));
      in_user.write("SCALL 0,i\n");
      in_user.close();
    }

    // Assemble
    {
      QProcess term;
      term.setWorkingDirectory(dir.path());
      term.start(path, {"asm",
                        "-s"
                        "in.pep",
                        "--os", "os.pep", "-o", "out.pepo", "--elf", "out.elf"});
      wait_return(term, 0);
    }

    // Execute
    {
      QProcess term;
      term.setWorkingDirectory(dir.path());
      term.start(path, {"run", "-s", "out.elf"});
      wait_return(term, 0);
      CHECK(out(term) == "Cannot use system calls in bare metal mode\n");
    }
  }

  /*SECTION("Fig 05.27") {
    QTemporaryDir dir;
    // Get user sources
    {
      QProcess term;
      term.setWorkingDirectory(dir.path());
      term.start(path, {"-e", "5", "get", "--ch", "05", "--fig", "27", "--type", "pep"});
      wait_return(term, 0);

      auto in_prog = QFile(dir.filePath("in.pep"));
      REQUIRE(in_prog.open(QIODevice::WriteOnly));
      in_prog.write(out(term).toUtf8());
      in_prog.close();

      auto in_txt = QFile(dir.filePath("in.txt"));
      REQUIRE(in_txt.open(QIODevice::WriteOnly));
      in_txt.write("10\n+20\n\n");
      in_txt.close();
    }
    // Assemble
    {
      QProcess term;
      term.setWorkingDirectory(dir.path());
      term.start(path, {"asm",
                        "-s"
                        "in.pep",
                        "--elf", "out.elf", "-o", "out.pepo"});
      wait_return(term, 0);
    }
    // Execute
    {
      QProcess term;
      term.setWorkingDirectory(dir.path());
      term.start(path, {"run", "-s", "out.elf", "-i", "in.txt", "-o", "-"});
      wait_return(term, 0);
      CHECK(out(term).toStdString() == "score = 25\n\n");
    }
  }*/

  SECTION("Custom macro directories") {
    QTemporaryDir dir;
    // Create user sources & macros
    {
      auto in_prog = QFile(dir.filePath("in.pep"));
      REQUIRE(in_prog.open(QIODevice::WriteOnly));
      in_prog.write("@MAC\n@CAM");
      in_prog.close();

      auto macro_dir1 = QDir(dir.path()).filePath("macros1");
      QDir(macro_dir1).mkpath(".");
      auto in_mac = QFile(macro_dir1 + "/mac.pepm");
      REQUIRE(in_mac.open(QIODevice::WriteOnly));
      in_mac.write("@MAC 0\nLDWA 0,i\n");
      in_mac.close();

      auto macro_dir2 = QDir(dir.path()).filePath("macros2");
      QDir(macro_dir2).mkpath(".");
      auto in_cam = QFile(macro_dir2 + "/cam.pepm");
      REQUIRE(in_cam.open(QIODevice::WriteOnly));
      in_cam.write("@CAM 0\nSTWA 0,d\n");
      in_cam.close();
    }
    // Assemble
    {
      QProcess term;
      term.setWorkingDirectory(dir.path());
      term.start(path, {"asm",
                        "-s"
                        "in.pep",
                        "--macro-dir", "macros1", "--macro-dir", "macros2", "-o", "out.pepo"});
      wait_return(term, 0);
      QFile out_pepo(dir.filePath("out.pepo"));
      REQUIRE(out_pepo.open(QIODevice::ReadOnly));
      auto lines = out_pepo.readAll();
      lines.replace("\r", "");
      CHECK(lines.toStdString() == "C0 00 00 E1 00 00 zz\n");
    }
  }

  SECTION("CLI Run overrides") {
    QTemporaryDir dir;
    // Create user source
    {
      auto in_prog = QFile(dir.filePath("in.pep"));
      REQUIRE(in_prog.open(QIODevice::WriteOnly));
      in_prog.write("STBA charOut,d\nLDBA 1,i\nSTBA pwrOff,d");
      in_prog.close();
    }
    // Assemble
    {
      QProcess term;
      term.setWorkingDirectory(dir.path());

      term.start(path, {
                           "asm",
                           "-s"
                           "in.pep",
                           "--elf",
                           "out.elf",
                       });
      wait_return(term, 0);
    }
    // Execute
    {
      QProcess term;
      term.setWorkingDirectory(dir.path());
      // Skip OS routines, start in user program with pre-loaded
      // accumulator value.
      term.start(path, {"run", "-s", "out.elf", "--reg", "Pc", "0x0000", "--reg", "a", "67", "-o", "-"});
      wait_return(term, 0);
      CHECK(out(term).toStdString() == "C\n");
    }
  }
}
