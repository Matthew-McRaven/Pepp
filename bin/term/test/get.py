import argparse
import unittest
import subprocess
import tempfile
import sys
executable = ""
class TestCase(unittest.TestCase):
    def test_get_pep(self):
        with tempfile.TemporaryDirectory() as cwd:
                ret = subprocess.run([executable, "get", "--ch", "05", "--fig", "27"], cwd=cwd, capture_output=True)
                self.assertEqual(ret.returncode, 0)
                self.assertNotEqual(len(ret.stdout), 0)

if __name__ == "__main__":
        parser = argparse.ArgumentParser(prog="pepp-term-test")
        parser.add_argument("executable")
        args = parser.parse_args()
        executable = args.executable
        suite = unittest.defaultTestLoader.loadTestsFromTestCase(TestCase)
        result = unittest.TextTestRunner(verbosity=2).run(suite)
        sys.exit(len(result.failures))
