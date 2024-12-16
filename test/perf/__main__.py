import click
import statistics as stats, pathlib
from . import CMake, Term
from pygit2 import Repository
from pygit2.enums import SortMode

@click.group()
def cli(): pass

@cli.command()
def build():
   cmake = CMake(cmakePath="/Users/matthewmcraven/Qt/Tools/CMake/CMake.app/Contents/bin/cmake")
   cmake.clean()
   cmake.build("/Users/matthewmcraven/Documents/Code/Pepp","/Volumes/RAMDisk/Build")

@cli.command("run-once")
def run_once():
    term = Term("/Volumes/RAMDisk/Build/output/pepp-term")
    vals = [x for x in map(lambda _: term.mit(), range(10))]
    print(f"{stats.mean(vals)}±{round(stats.stdev(vals),0)}")

@cli.command("do-commits")
def run():
    path = pathlib.Path(__file__).resolve().parent/"../../.git"
    repo = Repository(path.resolve())
    for commit in repo.walk(repo.head.target, SortMode.TOPOLOGICAL | SortMode.REVERSE):
        print(commit.message.split("\n")[0])
if __name__ == '__main__':
    cli()
