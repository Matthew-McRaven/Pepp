import statistics as stats, pathlib, re, shutil
import tempfile, sys, os, time, sqlite3
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


def term_path(build_dir): return str((pathlib.Path(build_dir)/term_name()).absolute())


def mit(build_dir):
    term_path(build_dir)
    term = Term(term_path(build_dir))
    vals = [x for x in map(lambda _: term.mit(), range(10))]
    return stats.mean(vals), stats.stdev(vals)


@cli.command("run-once")
@click.option("-b","--build-dir",type=str)
def run_once(build_dir):
    mean, stdev = mit(build_dir)
    print(f"{mean}±{round(stdev, 0)}")


class REMatch(str):
    def __eq__(self, regex): return re.fullmatch(regex, self)
def commit_filter(commit):
    first_line = commit.message.split("\n")[0]
    match REMatch(first_line):
        case "docs.*": return False  # Docs do not affect perf
        case "style.*": return False  # Reformatting shouldn't affect perf
        case "ci.*": return False  # YML shouldn't affect perf
        case "test.*": return False  # Tests should not affect perf
        case ".*\\(perf\\).*": return False # Skip tests related to perf testing tool
        case _: return True  # Everything else might


def cleanup_worktree(repo, name):
    # Delete worktree and is supporting branch if it already exists
    if name in repo.list_worktrees() and (tree := repo.lookup_worktree(name)) is not None: tree.prune(True)
    if name in repo.listall_branches() and (br := repo.lookup_branch(name)) is not None: br.delete()


def create_tables(conn, cur):
    make_experiments = """
    CREATE TABLE IF NOT EXISTS "experiments"  (
	"exp_id"	INTEGER PRIMARY KEY,
	"description"	TEXT
    ) STRICT;"""
    make_results="""
    CREATE TABLE IF NOT EXISTS "results" (
	"commit_sha"	TEXT NOT NULL,
	"exp_id"	INTEGER NOT NULL,
	"mean"	    REAL,
	"stddev"	REAL,
	PRIMARY KEY("commit_sha","exp_id"),
	FOREIGN KEY("exp_id") REFERENCES "experiments"("exp_id")
    ) STRICT;"""
    cur.execute("DROP TABLE IF EXISTS results")
    cur.execute("DROP TABLE IF EXISTS experiments")
    cur.execute(make_experiments)
    cur.execute(make_results)
    conn.commit()


def add_experiment(conn, cur, description):
    cur.execute("INSERT INTO experiments (description) VALUES (?) RETURNING exp_id;", (description,),)
    (row_id,) = row if (row:=cur.fetchone()) else None
    conn.commit()
    return row_id


def add_result(conn, cur, commit, exp_id, mean, stddev):
    cur.execute("INSERT INTO results (commit_sha, exp_id, mean, stddev) VALUES (?,?,?,?)", (commit, exp_id, mean, stddev))
    conn.commit()


@cli.command("do-commits")
@click.option("-b","--build-dir", type=str)
@click.option("-l","--log-dir", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parent/"logs")
@click.option("--db", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parent/"result.db")
def run(build_dir, log_dir, db):
    path = pathlib.Path(__file__).resolve().parent / "../../.git"
    repo = Repository(path.resolve())
    walker = repo.walk(repo.head.target, SortMode.TOPOLOGICAL | SortMode.REVERSE)
    hide = {
        "10238cae2735455dc9007c19246431f7cf8169e5",  # First revision to abandon boost and not require VCPKG.
        "7f39472a7c4029255a85ad5e845d2e3c37157190",  # Oldest commit where all output files are placed in /output.
        "11e95824c5964a8717a515dfa414819337c5f8b1",  # Oldest commit containing mit subcommand.
        "9b58d937f35533527321b3d32e4f5b057c45b1b7",  # Scintilla source subtree.
        "4308f5c7293414602fdc4b4a6186f19dd5b1b4ea",  # Python FORTH work.
        "35949d0af2b645c8cd6bffc51de428cdcde5583e",  # LRU cache subtree.
    }

    for c in hide: walker.hide(c)
    commits = [c for c in walker if commit_filter(c)]
    click.echo(f"Processing {len(commits)} commits")
    tree_tag = "buildzone"
    # Create a path for log files, deleting any existing logs
    shutil.rmtree(log_dir, ignore_errors=True)
    os.mkdir(log_dir)

    with tempfile.TemporaryDirectory() as src_dir, sqlite3.connect(db) as conn:
        cur = conn.cursor()
        src_dir = str((pathlib.Path(src_dir) / "src").absolute())
        create_tables(conn, cur)
        build_time_id = add_experiment(conn, cur, "Build time")
        mit_id = add_experiment(conn, cur, "Instruction throughput")
        try:
            cleanup_worktree(repo, tree_tag)
            wt = repo.add_worktree(tree_tag, src_dir)
            child_repo = Repository(wt.path)
            for idx, commit in enumerate(commits):
                click.echo(f"Checking out commit #{idx + 1} -- {commit.short_id}")
                child_repo.reset(commit.id, ResetMode.HARD)
                child_repo.submodules.update(init=True)
                try:
                    click.echo(f"Building commit #{idx + 1} -- {commit.short_id}")
                    with open(log_dir/f"{commit.short_id}.out", "w") as stdout, open(log_dir/f"{commit.short_id}.err", "w") as stderr:
                        start = time.time()
                        do_build(src_dir, build_dir, stdout=stdout, stderr=stderr)
                        build_duration = time.time() - start
                        add_result(conn, cur, commit.short_id, build_time_id, build_duration, 0)
                    mean, stddev = mit(build_dir)
                    add_result(conn, cur, commit.short_id, mit_id, mean, stddev)
                    click.echo(f"Finished commit #{idx+1} -- {commit.short_id}")
                except CalledProcessError as e:
                    click.echo("    " + str(e))
                    click.echo(f"Failed commit #{idx + 1} -- {commit.short_id}")
        # Ensures that we clean up the worktree if someone hits ctrl+c.
        except KeyboardInterrupt as e: raise e
        finally:
            cleanup_worktree(repo, tree_tag)



if __name__ == '__main__':
    cli()
