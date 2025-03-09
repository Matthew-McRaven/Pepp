import fnmatch
import pathlib
import os
import re
import shutil
import subprocess
from typing import Set, Literal

from black import Sequence

current_dir = os.path.dirname(os.path.realpath(__file__))
solution_dir = f"solutions"
figure_dir = f"figures"


def matches(ignores, item):
    for s in ignores:
        if re.match(s, item, re.I):
            return False
    return not pathlib.Path(item).is_dir()


def get_ignores():
    with open(f"{current_dir}/.gitignore", "r") as f:
        return [l.rstrip() for l in f.readlines()] + [
            solution_dir,
            figure_dir,
            "split.py",
        ]


solution_start = "# SOLUTION START"
solution_end = "# SOLUTION END"
figure_start = "# FIGURE START"
figure_end = "# FIGURE END"


def update_in_place(
    path: str, update_type=Literal["figure"] | Literal["solution"]
):
    start = solution_start if update_type == "figure" else figure_start
    end = solution_end if update_type == "figure" else figure_end
    with open(path, "r") as f:
        lines = f.readlines()
    output, in_block = [], False
    for line in lines:
        stripped = line.lstrip().rstrip()
        comments = {
            solution_start,
            solution_end,
            figure_start,
            figure_end,
        }
        if stripped.startswith(end):
            in_block = False
        elif stripped.startswith(start):
            in_block = True
        if "# FIGURE ONLY" in line:
            line = line.replace("# FIGURE ONLY", "")
            if update_type == "figure":
                output.append(line)
        elif "# SOLUTION ONLY" in line:
            line = line.replace("# SOLUTION ONLY", "")
            if update_type == "solution":
                output.append(line)
        elif in_block:
            continue
        elif not stripped in comments:
            output.append(line)
    with open(path, "w") as f:
        f.writelines(output)


def main():
    files = set(pathlib.Path().rglob("*"))
    ignores = get_ignores()
    filtered = list(filter(lambda x: matches(ignores, str(x)), files))
    # Copy solutions
    try:
        shutil.rmtree(solution_dir, ignore_errors=True)
    except FileNotFoundError:
        pass
    for src in filtered:
        dest = pathlib.Path(src)
        dest = pathlib.Path(solution_dir) / dest
        dest.parents[0].mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dest)
        update_in_place(dest, update_type="solution")
    subprocess.run(["black", solution_dir])
    
    # Copy figures
    try:
        shutil.rmtree(figure_dir, ignore_errors=True)
    except FileNotFoundError:
        pass
    for src in filtered:
        dest = pathlib.Path(src)
        dest = pathlib.Path(figure_dir) / dest
        dest.parents[0].mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dest)
        update_in_place(dest, update_type="figure")
    subprocess.run(["black", figure_dir])


if __name__ == "__main__":
    main()
