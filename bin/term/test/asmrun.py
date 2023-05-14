import argparse
import unittest
import subprocess
import tempfile
import sys
executable = ""
class TestCase(unittest.TestCase):
    def test_0527_flags(self):
        with tempfile.TemporaryDirectory() as cwd:
                ret = subprocess.run([executable, "get", "--ch", "05", "--fig", "27"], cwd=cwd, capture_output=True)
                self.assertEqual(ret.returncode, 0)
                self.assertNotEqual(len(ret.stdout), 0)
                with open(f"{cwd}/in.pep", "wb") as f:
                        f.write(ret.stdout)

                ret = subprocess.run([executable, "asm", "-s", "in.pep", "-o", "out.pepo", "--elf", "in.elf"], cwd=cwd, capture_output=True)
                self.assertEqual(ret.returncode, 0)
                with open(f"{cwd}/in.txt", "wt") as f:
                    f.write("10\n")
                    f.write("20\n")

                ret = subprocess.run([executable, "run", "-s", "in.elf", "-i", "in.txt", "-o", "-"], cwd=cwd, capture_output=True)
                self.assertEqual(ret.returncode, 0)
                self.assertEqual(ret.stdout, b"score = 25\n\n")

    def test_0527_positionals(self):
        with tempfile.TemporaryDirectory() as cwd:
                ret = subprocess.run([executable, "get", "--ch", "05", "--fig", "27"], cwd=cwd, capture_output=True)
                self.assertEqual(ret.returncode, 0)
                self.assertNotEqual(len(ret.stdout), 0)
                with open(f"{cwd}/in.pep", "wb") as f:
                        f.write(ret.stdout)

                ret = subprocess.run([executable, "asm", "in.pep", "--elf", "in.elf"], cwd=cwd, capture_output=True)
                self.assertEqual(ret.returncode, 0)
                with open(f"{cwd}/in.txt", "wt") as f:
                    f.write("10\n")
                    f.write("20\n")

                ret = subprocess.run([executable, "run", "in.elf", "-i", "in.txt", "-o", "-"], cwd=cwd, capture_output=True)
                self.assertEqual(ret.returncode, 0)
                self.assertEqual(ret.stdout, b"score = 25\n\n")

if __name__ == "__main__":
        parser = argparse.ArgumentParser(prog="pepp-term-test")
        parser.add_argument("executable")
        args = parser.parse_args()
        executable = args.executable
        suite = unittest.defaultTestLoader.loadTestsFromTestCase(TestCase)
        result = unittest.TextTestRunner(verbosity=2).run(suite)
        sys.exit(len(result.failures))
