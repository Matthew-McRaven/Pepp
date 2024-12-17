import statistics as stats, pathlib, re, shutil
import tempfile, sys, os
from subprocess import CalledProcessError

import click
from dotenv import load_dotenv
from pygit2 import Repository
from pygit2.enums import SortMode, ResetMode

from . import CMake, Term

@click.group()
def cli():
    # Load environment variables from file (if present)
    load_dotenv()

def do_build(src_dir, build_dir, cmake_path=None, stdout=None, stderr=None):
    if cmake_path is None: cmake_path=os.environ["CMAKE_PATH"]
    cmake = CMake(cmake_path=cmake_path)
    shutil.rmtree(build_dir, ignore_errors=True)
    cmake.build(src_dir, build_dir, stdout=stdout, stderr=stderr)

@cli.command()
@click.option("-b","--build-dir",type=str)
def build(build_dir):
    repo_root = pathlib.Path(__file__).resolve().parent / "../.."
    do_build(str(repo_root.absolute()), build_dir)

def term_name():
    match sys.platform:
        case 'linux': return "output/pepp-term"
        case 'darwin': return "output/pepp-term"
        case 'win32': return "output/pepp-term.exe"

@cli.command("run-once")
@click.option("-b","--build-dir",type=str)
def run_once(build_dir):
    term_path = str((pathlib.Path(build_dir)/term_name()).absolute())
    term = Term(term_path)
    vals = [x for x in map(lambda _: term.mit(), range(10))]
    print(f"{stats.mean(vals)}±{round(stats.stdev(vals), 0)}")


class REMatch(str):
    def __eq__(self, regex): return re.fullmatch(regex, self)
def commit_filter(commit):
    first_line = commit.message.split("\n")[0]
    match REMatch(first_line):
        case "docs.*": return False  # Docs do not affect perf
        case "style.*": return False  # Reformatting shouldn't affect perf
        case "ci.*": return False  # YML shouldn't affect perf
        case "test.*": return False  # Tests should not affect perf
        case _: return True  # Everything else might


def cleanup_worktree(repo, name):
    # Delete worktree and is supporting branch if it already exists
    if name in repo.list_worktrees() and (tree := repo.lookup_worktree(name)) is not None: tree.prune(True)
    if name in repo.listall_branches() and (br := repo.lookup_branch(name)) is not None: br.delete()


@cli.command("do-commits")
@click.option("-b","--build-dir", type=str)
@click.option("-l","--log-dir", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parent/"logs")
def run(build_dir,log_dir):
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
    commits = [c for c in walker if commit_filter(c)]
    tree_tag = "buildzone"
    # Create a path for log files, deleting any existing logs
    shutil.rmtree(log_dir, ignore_errors=True)
    os.mkdir(log_dir)

    with tempfile.TemporaryDirectory() as src_dir:
        src_dir = str((pathlib.Path(src_dir) / "src").absolute())
        try:
            cleanup_worktree(repo, tree_tag)
            wt = repo.add_worktree(tree_tag, src_dir)
            child_repo = Repository(wt.path)
            for commit in commits:
                child_repo.reset(commit.id, ResetMode.HARD)
                child_repo.submodules.update(init=True)
                try:
                    with open(log_dir/f"{commit.short_id}.out", "w") as stdout, open(log_dir/f"{commit.short_id}.err", "w") as stderr:
                        do_build(src_dir, build_dir, stdout=stdout, stderr=stderr)
                except CalledProcessError: pass
        # Ensures that we clean up the worktree if someone hits ctrl+c.
        except KeyboardInterrupt as e: raise e
        finally:
            cleanup_worktree(repo, tree_tag)



if __name__ == '__main__':
    cli()
