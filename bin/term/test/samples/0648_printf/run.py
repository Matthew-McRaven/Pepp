import argparse
import unittest
import subprocess
import tempfile
import sys
import os
executable = ""
class TestCase(unittest.TestCase):
    def test_assemble(self):
        with tempfile.TemporaryDirectory() as cwd:
                ret = subprocess.run([executable, "asm", f"{os.getcwd()}/in.pep", "-o", f"{cwd}/out.pepo",
                "--elf", f"{cwd}/out.elf"], cwd=cwd, capture_output=True)
                self.assertEqual(ret.returncode, 0, f"{ret.returncode}")
                self.assertEqual(os.path.exists(f"{cwd}/out.elf"), True)
                ret = subprocess.run([executable, "run", f"{cwd}/out.elf", "-o", f"{cwd}/out.txt", "-i"
                , f"{os.getcwd()}/in.txt"], cwd=cwd, capture_output=True)
                actual, expected = "", ""
                try:
                  with open(f"{cwd}/out.txt", "rt") as f:
                    actual = "\n".join(f.readlines())
                  with open(f"{os.getcwd()}/out.txt", "rt") as f:
                    expected = "\n".join(f.readlines())
                except IOError as e:
                  print(e.strerror)
                  self.fail("Failed file IO")
                  print(actual, expected, "t")
                self.assertEqual(actual, expected)


if __name__ == "__main__":
        parser = argparse.ArgumentParser(prog="pepp-term-test-sample")
        parser.add_argument("executable")
        args = parser.parse_args()
        executable = args.executable
        suite = unittest.defaultTestLoader.loadTestsFromTestCase(TestCase)
        result = unittest.TextTestRunner(verbosity=2).run(suite)
        sys.exit(len(result.failures))
