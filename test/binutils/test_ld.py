# Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

import os
import pathlib
import subprocess
import tempfile
import unittest

DATA = pathlib.Path(__file__).parent / "data/ld-lazy-member"

# Each case is the command-line arguments and whether the command should succeed. {out} is a scratch output file.
CASES = {
    # Every option is accepted, though the linker ignores most of them for now.
    "every option": (["-o", "{out}", "-e", "_start", "--section-start=.text=0x100", "--exclude-libs", "a.a,b.a",
                      "--exclude-libs", "ALL", "--retain-symbols-file", "retained.txt", "-R", "lib.a", "main.o",
                      "lib.a"], True),
    "a link error": (["-o", "{out}", "main.o"], False),
    "a malformed option": (["-o", "{out}", "--section-start=.text", "main.o", "lib.a"], False),
}


class LdExitStatus(unittest.TestCase):
    def test_cases(self):
        for name, (arguments, succeeds) in CASES.items():
            with self.subTest(name), tempfile.TemporaryDirectory() as scratch:
                arguments = [argument.format(out=os.path.join(scratch, "out")) for argument in arguments]
                out = subprocess.run([os.environ["PEPP_TERM"], "ld", *arguments], cwd=DATA,
                                     capture_output=True, text=True, timeout=60)
                self.assertEqual(out.returncode == 0, succeeds, out.stdout + out.stderr)
                self.assertEqual(os.path.exists(os.path.join(scratch, "out")), succeeds)


if __name__ == "__main__":
    unittest.main(verbosity=2)
