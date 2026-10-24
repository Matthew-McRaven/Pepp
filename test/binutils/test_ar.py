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

"""Runs the same ar commands through GNU ar and `pepp-term ar` and compared the resulting archives.

Each case starts from the same member files in two scratch directories, one per tool, and runs its steps in both.
The tools must agree on each command's exit status and stdout, and on every file left behind, byte for byte.
stderr is not compared, since the two word their messages differently.

The test reads PEPP_TERM (the pepp-term executable) and PEPP_AR_MEMBERS (an archive holding the member files) from
the environment. GNU ar is $PEPP_TEST_AR, or else the first of AR_NAMES on PATH that reports itself as GNU ar. Without
a valid GNU ar, the test is skipped.
"""

import os
import pathlib
import shutil
import subprocess
import tempfile
import unittest

# Apple's /usr/bin/ar is BSD ar, which writes a different format.
AR_NAMES = ["ar", "gar", "riscv64-elf-ar", "riscv64-unknown-elf-ar", "x86_64-linux-gnu-ar"]
LONG = "a_member_with_a_long_name.o"
MEMBERS = ["a.o", "odd.txt", LONG]

# Each step is a list of ar arguments, or a ("write", name, text) / ("copy", source, name) / ("remove", names...) tuple
# that edits the directory the same way for both tools.
DUPLICATE = [["rcsD", "lib.a", *MEMBERS], ("copy", "odd.txt", "dup/a.o"), ["qD", "lib.a", "dup/a.o"]]
CASES = {
    "create without s": [["rcD", "lib.a", *MEMBERS]],
    "create without an index": [["rcSD", "lib.a", *MEMBERS]],
    "quick-append writes an index": [["qcD", "lib.a", *MEMBERS]],
    "quick-append without an index": [["qcSD", "lib.a", *MEMBERS]],
    "append": [["rcsD", "lib.a", "a.o"], ["qsD", "lib.a", "odd.txt", LONG]],
    "replace appends and updates the index": [["rcsD", "lib.a", "odd.txt"], ["rvD", "lib.a", "a.o", LONG]],
    "replace and append at once": [
        ["rcsD", "lib.a", "a.o", "odd.txt"],
        ("write", "odd.txt", "a longer replacement\n"),
        ["rvD", "lib.a", LONG, "odd.txt"],
    ],
    "replace and append in a thin archive": [
        ["rcsDT", "lib.a", "a.o", "odd.txt"],
        ("write", "odd.txt", "a longer replacement\n"),
        ["rvDT", "lib.a", LONG, "odd.txt"],
    ],
    "modify a thin archive without T": [["rcsDT", "lib.a", *MEMBERS], ["rD", "lib.a", "odd.txt"]],
    "modify a regular archive with T": [["rcsD", "lib.a", *MEMBERS], ["rDT", "lib.a", "odd.txt"]],
    "delete the long-named member": [["rcsD", "lib.a", *MEMBERS], ["dD", "lib.a", LONG]],
    "add an index": [["rcSD", "lib.a", *MEMBERS], ["sD", "lib.a"]],
    "list": [["rcsD", "lib.a", *MEMBERS], ["t", "lib.a"], ["t", "lib.a", "odd.txt"]],
    "list verbosely": [["rcsD", "lib.a", *MEMBERS], ["tv", "lib.a"]],
    "print": [["rcsD", "lib.a", *MEMBERS], ["p", "lib.a", "odd.txt"]],
    "extract everything": [["rcsD", "lib.a", *MEMBERS], ("remove", *MEMBERS), ["x", "lib.a"]],
    "list a missing archive": [["t", "missing.a"]],
    # Names.
    "a 15-character name fits in the header, a 16-character one does not": [
        ("copy", "a.o", "fifteen_chars.o"),
        ("copy", "a.o", "sixteen__chars.o"),
        ["rcsD", "lib.a", "fifteen_chars.o", "sixteen__chars.o"],
        ["t", "lib.a"],
    ],
    "a member added by path": [("copy", "a.o", "dir/b.o"), ["rcsD", "lib.a", "dir/b.o"], ["t", "lib.a"]],
    "a thin member added by path": [("copy", "a.o", "dir/b.o"), ["rcsDT", "lib.a", "dir/b.o"], ["t", "lib.a"]],
    "a name with a space": [("copy", "odd.txt", "my file.txt"), ["rcsD", "lib.a", "my file.txt"], ["t", "lib.a"]],
    # Two members named a.o, the second holding odd.txt's bytes.
    "list duplicates": [*DUPLICATE, ["t", "lib.a"]],
    "replace a duplicate": [*DUPLICATE, ["rD", "lib.a", "a.o"]],
    "delete a duplicate": [*DUPLICATE, ["dD", "lib.a", "a.o"]],
    "extract a duplicate": [*DUPLICATE, ("remove", "a.o"), ["x", "lib.a", "a.o"]],
    # Nothing to index or store.
    "create an empty archive": [["rcsD", "lib.a"]],
    "a zero-length member": [("write", "empty.txt", ""), ["rcsD", "lib.a", "empty.txt", "odd.txt"]],
    # Errors, compared by exit status.
    "list a missing member": [["rcsD", "lib.a", *MEMBERS], ["t", "lib.a", "missing.o"]],
    "extract a missing member": [["rcsD", "lib.a", *MEMBERS], ["x", "lib.a", "missing.o"]],
    "delete a missing member": [["rcsD", "lib.a", *MEMBERS], ["dD", "lib.a", "missing.o"]],
    "add a missing file": [["rcsD", "lib.a", "missing.o"]],
}


def is_gnu_ar(path):
    try:
        out = subprocess.run([path, "--version"], capture_output=True, text=True, timeout=10)
    except (OSError, subprocess.TimeoutExpired):
        return False
    return "GNU ar" in out.stdout


def find_gnu_ar():
    if override := os.environ.get("PEPP_TEST_AR"):
        return override if is_gnu_ar(override) else None
    return next((path for name in AR_NAMES if (path := shutil.which(name)) and is_gnu_ar(path)), None)


def run_steps(command, steps, directory):
    """Runs steps in directory, returning each ar command's (arguments, failed, stdout) and every file left behind."""
    transcript = []
    for step in steps:
        match step:
            case ("write", name, text):
                (target := directory / name).parent.mkdir(parents=True, exist_ok=True)
                target.write_text(text)
            case ("copy", source, name):
                (target := directory / name).parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(directory / source, target)
            case ("remove", *names):
                for name in names:
                    (directory / name).unlink()
            case _:
                out = subprocess.run([*command, *step], cwd=directory, capture_output=True, timeout=60)
                transcript.append((step, out.returncode != 0, out.stdout))
    files = {p.relative_to(directory).as_posix(): p.read_bytes() for p in directory.rglob("*") if p.is_file()}
    return transcript, files


class ArMatchesGnu(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.gnu_ar = find_gnu_ar()
        if cls.gnu_ar is None:
            raise unittest.SkipTest("no GNU ar found. Set PEPP_TEST_AR or add an ar instance to your path")
        cls.scratch = tempfile.TemporaryDirectory()
        # The member files come out of a GNU-built archive, so both tools start from the same bytes.
        cls.seed = pathlib.Path(cls.scratch.name) / "seed"
        cls.seed.mkdir()
        members = pathlib.Path(os.environ["PEPP_AR_MEMBERS"]).resolve()
        subprocess.run([cls.gnu_ar, "x", members], cwd=cls.seed, check=True)

    @classmethod
    def tearDownClass(cls):
        cls.scratch.cleanup()

    def test_cases(self):
        tools = {"gnu": [self.gnu_ar], "pepp": [os.environ["PEPP_TERM"], "ar"]}
        for index, (name, steps) in enumerate(CASES.items()):
            with self.subTest(name):
                results = {}
                for tool, command in tools.items():
                    directory = pathlib.Path(self.scratch.name) / f"{index}-{tool}"
                    shutil.copytree(self.seed, directory)
                    results[tool] = run_steps(command, steps, directory)
                (gnu_transcript, gnu_files), (pepp_transcript, pepp_files) = results["gnu"], results["pepp"]
                self.assertEqual(pepp_transcript, gnu_transcript)
                self.assertEqual(sorted(pepp_files), sorted(gnu_files))
                for file, expected in gnu_files.items():
                    if (actual := pepp_files[file]) != expected:
                        first = next((i for i, (a, b) in enumerate(zip(actual, expected)) if a != b),
                                     min(len(actual), len(expected)))
                        self.fail(f"{file} differs at byte {first} (GNU: {len(expected)} bytes, pepp: {len(actual)})")


if __name__ == "__main__":
    unittest.main(verbosity=2)
