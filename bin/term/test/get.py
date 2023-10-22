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
executable = ""


class TestCase(unittest.TestCase):
    def test_get_pep(self):
        with tempfile.TemporaryDirectory() as cwd:
            ret = subprocess.run(
                [executable, "get", "--ch", "05", "--fig", "27"], cwd=cwd, capture_output=True)
            self.assertEqual(ret.returncode, 0)
            self.assertNotEqual(len(ret.stdout), 0)

    def test_get_not_exist(self):
        with tempfile.TemporaryDirectory() as cwd:
            ret = subprocess.run(
                [executable, "get", "--ch", "00", "--fig", "x0"], cwd=cwd, capture_output=True)
            self.assertNotEqual(ret.returncode, 0)
            self.assertEqual(ret.stderr.replace(b"\r", b""), b"Figure 00.x0 does not exist.\n")

    def test_get_no_var(self):
        with tempfile.TemporaryDirectory() as cwd:
            ret = subprocess.run([executable,
                                  "get",
                                  "--ch",
                                  "05",
                                  "--fig",
                                  "27",
                                  "--type",
                                  "not"],
                                 cwd=cwd,
                                 capture_output=True)
            self.assertNotEqual(ret.returncode, 0)
            self.assertEqual(
                ret.stderr.replace(b"\r", b""),
                b"Figure 05.27 does not contain a \"not\" variant.\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog="pepp-term-test")
    parser.add_argument("executable")
    args = parser.parse_args()
    executable = args.executable
    with contextlib.redirect_stdout(sys.stderr):
        suite = unittest.defaultTestLoader.loadTestsFromTestCase(TestCase)
        result = unittest.TextTestRunner(stream=sys.stderr, verbosity=2, buffer=True).run(suite)
        assert len(result.failures) == 0
