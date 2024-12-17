import click
import statistics as stats, pathlib, re, shutil
from . import CMake, Term
from pygit2 import Repository
from pygit2.enums import SortMode

@click.group()
def cli(): pass

@cli.command()
def build():
    cmake = CMake(cmakePath="/Users/matthewmcraven/Qt/Tools/CMake/CMake.app/Contents/bin/cmake")
    shutil.rmtree("/Volumes/RAMDisk/Build")
    cmake.build("/Users/matthewmcraven/Documents/Code/Pepp", "/Volumes/RAMDisk/Build")

@cli.command("run-once")
def run_once():
    term = Term("/Volumes/RAMDisk/Build/output/pepp-term")
    vals = [x for x in map(lambda _: term.mit(), range(10))]
    print(f"{stats.mean(vals)}±{round(stats.stdev(vals), 0)}")


class REMatch(str):
    def __eq__(self, regex): return re.fullmatch(regex, self)


def filter(commit):
    first_line = commit.message.split("\n")[0]
    match REMatch(first_line):
        case "docs.*": return False  # Docs do not affect perf
        case "style.*": return False  # Reformatting shouldn't affect perf
        case "ci.*": return False  # YML shouldn't affect perf
        case "test.*": return False  # Tests should not affect perf
        case _: return True  # Everything else might


@cli.command("do-commits")
def run():
    path = pathlib.Path(__file__).resolve().parent / "../../.git"
    repo = Repository(path.resolve())
    walker = repo.walk(repo.head.target, SortMode.TOPOLOGICAL | SortMode.REVERSE)
    hide = {
        "10238cae2735455dc9007c19246431f7cf8169e5",  # First revision to abandon boost and not require VCPKG.
        "9b58d937f35533527321b3d32e4f5b057c45b1b7",  # Scintilla source subtree
        "4308f5c7293414602fdc4b4a6186f19dd5b1b4ea",  # Python FORTH work
        "35949d0af2b645c8cd6bffc51de428cdcde5583e",  # LRU cache subtree
    }

    for c in hide: walker.hide(c)
    commits = [c for c in walker if filter(c)]
    print(len(commits))


if __name__ == '__main__':
    cli()
