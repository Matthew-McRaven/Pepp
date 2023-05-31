import argparse
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
                ret.stdout,
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

            ret = subprocess.run([executable,
                                  "asm",
                                  "-s",
                                  "in.pep",
                                  "--os",
                                  "os.pep",
                                  "-o",
                                  "out.pepo",
                                  "--elf",
                                  "in.elf"],
                                 cwd=cwd,
                                 capture_output=True)
            self.assertEqual(ret.returncode, 0)

            ret = subprocess.run(
                [executable, "run", "-s", "in.elf"], cwd=cwd, capture_output=True)
            self.assertEqual(ret.returncode, 0)
            self.assertEqual(
                ret.stdout,
                b"Cannot use system calls in bare metal mode\n")

    def test_0527_flags(self):
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

            ret = subprocess.run([executable,
                                  "asm",
                                  "-s",
                                  "in.pep",
                                  "-o",
                                  "out.pepo",
                                  "--elf",
                                  "in.elf"],
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
            self.assertEqual(ret.stdout, b"score = 25\n\n")

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
                                 capture_output=True)
            self.assertEqual(ret.returncode, 0)
            self.assertEqual(ret.stdout, b"score = 25\n\n")

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
            ret = subprocess.run(
                [executable, "asm", "in.pep", "--elf", "in.elf"], cwd=cwd, capture_output=True)
            self.assertEqual(ret.returncode, 0)

            # Skip OS routines, start in user program with pre-loaded
            # accumulator value.
            ret = subprocess.run([executable,
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
                                  "-"],
                                 cwd=cwd,
                                 capture_output=True)
            self.assertEqual(ret.returncode, 0)
            self.assertEqual(ret.stdout, b"C\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog="pepp-term-test")
    parser.add_argument("executable")
    args = parser.parse_args()
    executable = args.executable
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(TestCase)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    sys.exit(len(result.failures))
