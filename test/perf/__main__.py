import click
from . import CMake

@click.group()
def cli(): pass

@cli.command()
def build():
   cmake = CMake(cmakePath="/Users/matthewmcraven/Qt/Tools/CMake/CMake.app/Contents/bin/cmake")
   cmake.clean()
   cmake.build("/Users/matthewmcraven/Documents/Code/Pepp","/Volumes/RAMDisk/Build")

@cli.command()
def run_once(): pass
if __name__ == '__main__':
    cli()
