import click
import statistics as stats
from . import CMake, Term

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
if __name__ == '__main__':
    cli()
