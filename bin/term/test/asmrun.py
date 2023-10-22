#  Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.

import argparse
import contextlib
import unittest
import subprocess
import tempfile
import sys
import os
executable = ""


class TestCase(unittest.TestCase):
    def test_baremetal_flag_flags(self):
        with tempfile.TemporaryDirectory() as cwd:
            try:
                with open(f"{cwd}/in.pep", "wb") as f:
                    f.write(b"SCALL 0,i")
            except IOError:
                self.fail("Writing contents should not fail")

            ret = subprocess.run([executable,
                                  "asm",
                                  "-s",
                                  "in.pep",
                                  "--bm",
                                  "-o",
                                  "out.pepo"],
                                 cwd=cwd,
                                 capture_output=True)
            self.assertEqual(ret.returncode, 0)

            ret = subprocess.run(
                [executable, "run", "--bm","-s", "out.pepo"], cwd=cwd, capture_output=True)
            self.assertEqual(ret.returncode, 0)
            self.assertEqual(
                ret.stdout.replace(b"\r",b""),
                b"Cannot use system calls in bare metal mode\n")

    def test_baremetal(self):
        with tempfile.TemporaryDirectory() as cwd:
            ret = subprocess.run(
                [executable, "get", "--ch", "os", "--fig", "pep10baremetal"], cwd=cwd, capture_output=True)
            self.assertEqual(ret.returncode, 0)
            try:
                with open(f"{cwd}/os.pep", "wb") as f:
                    f.write(ret.stdout)
                with open(f"{cwd}/in.pep", "wb") as f:
                    f.write(b"SCALL 0,i")
            except IOError:
                self.fail("Writing contents should not fail")
            args = [executable,
            "asm",
            "-s",
            "in.pep",
            "--os",
            "os.pep",
            "-o",
            "out.pepo",
            "--elf",
            "in.elf"]
            ret = subprocess.run(args,
                                 cwd=cwd,
                                 capture_output=True)
            self.assertEqual(ret.returncode, 0)

            ret = subprocess.run(
                [executable, "run", "-s", "in.elf"], cwd=cwd, capture_output=True)
            self.assertEqual(ret.returncode, 0)
            self.assertEqual(
                ret.stdout.replace(b"\r",b""),
                b"Cannot use system calls in bare metal mode\n")

    def test_0527_flags(self):
        with tempfile.TemporaryDirectory() as cwd:
            print(cwd, flush=True)
            ret = subprocess.run(
                [executable, "get", "--ch", "05", "--fig", "27"], cwd=cwd, capture_output=True)
            self.assertEqual(ret.returncode, 0)
            self.assertNotEqual(len(ret.stdout), 0)

            try:
                with open(f"{cwd}/in.pep", "wb") as f:
                    f.write(ret.stdout)
                with open(f"{cwd}/in.txt", "wt") as f:
                    f.write("10"+os.linesep)
                    f.write("20\n"+os.linesep)
            except IOError:
                self.fail("Writing contents should not fail")
            args = [executable,
            "asm",
            "-s",
            "in.pep",
            "-o",
            "out.pepo",
            "--elf",
            "in.elf"]

            ret = subprocess.run(args,
                                 cwd=cwd,
                                 capture_output=True)

            self.assertEqual(ret.returncode, 0)

            ret = subprocess.run([executable,
                                  "run",
                                  "-s",
                                  "in.elf",
                                  "-i",
                                  "in.txt",
                                  "-o",
                                  "-"],
                                 cwd=cwd,
                                 capture_output=True)
            self.assertEqual(ret.returncode, 0)
            self.assertEqual(ret.stdout.replace(b"\r",b""), b"score = 25\n\n")

    def test_0527_positionals(self):
        with tempfile.TemporaryDirectory() as cwd:
            ret = subprocess.run(
                [executable, "get", "--ch", "05", "--fig", "27"], cwd=cwd, capture_output=True)
            self.assertEqual(ret.returncode, 0)
            self.assertNotEqual(len(ret.stdout), 0)
            try:
                with open(f"{cwd}/in.pep", "wb") as f:
                    f.write(ret.stdout)
                with open(f"{cwd}/in.txt", "wt") as f:
                    f.write("10\n")
                    f.write("20\n")
            except IOError:
                self.fail("Writing contents should not fail")

            ret = subprocess.run(
                [executable, "asm", "in.pep", "--elf", "in.elf"], cwd=cwd, capture_output=True)
            self.assertEqual(ret.returncode, 0)
            ret = subprocess.run([executable,
                                  "run",
                                  "in.elf",
                                  "-i",
                                  "in.txt",
                                  "-o",
                                  "-"],
                                 cwd=cwd,
                                 #env=dict(os.environ, ForceBP="1"),
                                 capture_output=True)
            self.assertEqual(ret.returncode, 0)
            self.assertEqual(ret.stdout.replace(b"\r",b""), b"score = 25\n\n")

    def test_macro_dir_1(self):
        with tempfile.TemporaryDirectory() as cwd:
            try:
                os.makedirs(f"{cwd}/macros")
                with open(f"{cwd}/macros/mac.pepm", "wt") as f:
                    f.write("@MAC 0\nLDWA 0,i")
                with open(f"{cwd}/in.pep", "wt") as f:
                    f.write("@MAC\n")
                ret = subprocess.run([executable,
                                      "asm",
                                      "in.pep",
                                      "--macro-dir",
                                      "macros",
                                      "-e",
                                      "in.err.txt"],
                                     cwd=cwd,
                                     capture_output=True)

                self.assertEqual(ret.returncode, 0)
                with open(f"{cwd}/in.pepo", "rt") as f:
                    contents = "\n".join(f.readlines()).strip()
                    self.assertEqual(contents, "40 00 00 zz")
            except IOError:
                self.fail("Writing contents should not fail")

    def test_macro_dir_2(self):
        with tempfile.TemporaryDirectory() as cwd:
            try:
                os.makedirs(f"{cwd}/macros1")
                os.makedirs(f"{cwd}/macros2")
                with open(f"{cwd}/macros1/mac.pepm", "wt") as f:
                    f.write("@MAC 0\nLDWA 0,i")
                with open(f"{cwd}/macros2/cam.pepm", "wt") as f:
                    f.write("@CAM 0\nSTWA 0,d")

                with open(f"{cwd}/in.pep", "wt") as f:
                    f.write("@MAC\n@CAM")
                ret = subprocess.run([executable,
                                      "asm",
                                      "in.pep",
                                      "--macro-dir",
                                      "macros1",
                                      "--macro-dir",
                                      "macros2",
                                      "-e",
                                      "in.err.txt"],
                                     cwd=cwd,
                                     capture_output=True)

                self.assertEqual(ret.returncode, 0)
                with open(f"{cwd}/in.pepo", "rt") as f:
                    contents = "\n".join(f.readlines()).strip()
                    self.assertEqual(contents, "40 00 00 61 00 00 zz")
            except IOError:
                self.fail("Writing contents should not fail")

    def test_reg_override(self):
        with tempfile.TemporaryDirectory() as cwd:
            try:
                with open(f"{cwd}/in.pep", "wt") as f:
                    f.write("STBA charOut,d\nLDBA 1,i\nSTBA pwrOff,d")
            except IOError:
                self.fail("Writing contents should not fail")
            args = [executable, "asm", f"in.pep", "--elf", "in.elf"]
            print(" ".join(args), flush=True)
            ret = subprocess.run(args, cwd=cwd, capture_output=True)
            self.assertEqual(ret.returncode, 0)

            # Skip OS routines, start in user program with pre-loaded
            # accumulator value.
            args = [executable,
            "run",
            "-s",
            "in.elf",
            "--skip-load",
            "--skip-dispatch",
            "--reg",
            "Pc",
            "0x0000",
            "--reg",
            "a",
            "67",
            "-o",
            "-"]

            ret = subprocess.run(args,
                                 cwd=cwd,
                                 capture_output=True)
            self.assertEqual(ret.returncode, 0)
            self.assertEqual(ret.stdout.replace(b"\r",b""), b"C\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog="pepp-term-test")
    parser.add_argument("executable")
    args = parser.parse_args()
    executable = args.executable
    with contextlib.redirect_stdout(sys.stderr):
        suite = unittest.defaultTestLoader.loadTestsFromTestCase(TestCase)
        result = unittest.TextTestRunner(stream=sys.stderr, verbosity=2, buffer=True).run(suite)
        assert len(result.failures) == 0
