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

"""Runs `pepp-term ld` and checks its exit status, then compares an executable it links against GNU ld's.

The test reads PEPP_TERM (the pepp-term executable) from the environment. GNU ld is $PEPP_TEST_LD, or else the first of
LD_NAMES on PATH. If neither is found, the test is skipped.
"""

import os
import pathlib
import re
import shutil
import subprocess
import tempfile
import unittest

DATA = pathlib.Path(__file__).parent / "data/ld-lazy-member"
EXEC = pathlib.Path(__file__).parent / "data/ld-exec"
LD_NAMES = ["riscv64-elf-ld", "riscv64-unknown-elf-ld"]
# Sections whose headers and contents must match GNU ld's. GNU ld merges the others (e.g., .riscv.attributes), while pld
# concatenates them.
COMPARED = [".text", ".data", ".mine", ".bss"]

# Each case is the command-line arguments and whether the command should succeed. {out} is a scratch output file.
CASES = {
    # Every option is accepted, though the linker ignores most of them for now. main.o needs an R_RISCV_32.
    "every option": (["-o", "{out}", "-e", "_start", "--section-start=.text=0x100", "--exclude-libs", "a.a,b.a",
                      "--exclude-libs", "ALL", "--retain-symbols-file", "retained.txt", "-R", "lib.a", "main.o",
                      "lib.a"], True),
    "a link error": (["-o", "{out}", "main.o"], False),
    "a malformed option": (["-o", "{out}", "--section-start=.text", "main.o", "lib.a"], False),
}


def gnu_ld():
    for name in [os.environ.get("PEPP_TEST_LD"), *LD_NAMES]:
        if name and shutil.which(name):
            return shutil.which(name)
    return None


def run(*command):
    out = subprocess.run(command, cwd=EXEC, capture_output=True, text=True, timeout=60)
    if out.returncode != 0:
        raise AssertionError(" ".join(map(str, command)) + "\n" + out.stdout + out.stderr)
    return out.stdout


def describe(readelf, path):
    """Returns the entry point, compared section headers and contents, and PT_LOADs of an executable."""
    headers = run(readelf, "-hSlW", path)
    entry = re.search(r"Entry point address:\s+(\S+)", headers).group(1)
    # Name, type, address, size, and flags. The file offset is skipped, since the tools may lay the file out differently.
    section = re.compile(r"\[\s*\d+\]\s+(\S+)\s+(\S+)\s+([0-9a-f]+)\s+[0-9a-f]+\s+([0-9a-f]+)\s+[0-9a-f]+\s+([A-Z]*)")
    sections = [match.groups() for match in section.finditer(headers) if match.group(1) in COMPARED]
    # Virtual address, file size, memory size, and flags.
    load = re.compile(r"LOAD\s+\S+\s+(\S+)\s+\S+\s+(\S+)\s+(\S+)\s+([RWE ]+?)\s+0x")
    loads = [match.groups() for match in load.finditer(headers)]
    contents = [run(readelf, "-x", name, path).split("\n", 2)[2] for name, kind, *_ in sections if kind != "NOBITS"]
    return entry, sections, loads, contents


class LdExitStatus(unittest.TestCase):
    def test_cases(self):
        for name, (arguments, succeeds) in CASES.items():
            with self.subTest(name), tempfile.TemporaryDirectory() as scratch:
                arguments = [argument.format(out=os.path.join(scratch, "out")) for argument in arguments]
                out = subprocess.run([os.environ["PEPP_TERM"], "ld", *arguments], cwd=DATA,
                                     capture_output=True, text=True, timeout=60)
                self.assertEqual(out.returncode == 0, succeeds, out.stdout + out.stderr)
                self.assertEqual(os.path.exists(os.path.join(scratch, "out")), succeeds)


class LdExecutable(unittest.TestCase):
    def test_matches_gnu_ld(self):
        arguments = ["-m", "elf32lriscv", "-T", "exec.ld", "a.o", "b.o", "-o"]
        with tempfile.TemporaryDirectory() as scratch:
            ours = os.path.join(scratch, "ours")
            run(os.environ["PEPP_TERM"], "ld", *arguments, ours)
            with open(ours, "rb") as file:
                header = file.read(18)
            # ET_EXEC, little-endian.
            self.assertEqual(header[:4] + header[16:18], b"\x7fELF\x02\x00")
            self.assertTrue(os.access(ours, os.X_OK))

            ld = gnu_ld()
            readelf = shutil.which(ld[:-2] + "readelf") if ld else None
            if not readelf:
                self.skipTest("no GNU ld and readelf found")
            theirs = os.path.join(scratch, "theirs")
            run(ld, *arguments, theirs)
            self.assertEqual(describe(readelf, ours), describe(readelf, theirs))


if __name__ == "__main__":
    unittest.main(verbosity=2)
