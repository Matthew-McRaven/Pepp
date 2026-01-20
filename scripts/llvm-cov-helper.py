import os
import stat
import sys

from pathlib import Path

profraws = [str(profraw.absolute()) for profraw in Path('profraw').rglob('*.profraw')]

print(sys.argv)
print(profraws)
os.system(f'llvm-profdata-17 merge {" ".join(profraws)} --output coverage.profdata')

executable = stat.S_IEXEC | stat.S_IXUSR

visited = set()
tests = []


def walk(root, dirs, files):
    # Prevent infinite recursion by keeping track of visited folders.
    if root in visited: return
    visited.add(root)

    if "CMake" in root:
        return
    elif "3rd" in root:
        return
    for filename in files:
        filename = root + "/" + filename
        if os.path.isfile(filename):
            st = os.stat(filename)
            if not st.st_mode & executable:
                continue
            elif ".sh" in filename:
                continue
            # Project re-organization caused more files to be detected by this script, so we must filter them out.
            elif ".py" in filename:
                continue
            elif ".txt" in filename:
                continue
            elif ".pep" in filename:
                continue
            elif ".in" in filename:
                continue
            elif '.node' in filename:
                continue
            elif ".js" in filename:
                continue
            else:
                tests.append(filename)
    for d in dirs:
        for iroot, idirs, ifiles in os.walk(d):
            # Prevents infinite loop where directory enumerates itself.
            if iroot == root: continue
            walk(iroot, idirs, ifiles)


for root, dirs, files in os.walk(sys.argv[1]):
    walk(root, dirs, files)

regex = "\"(catch)|(elfio)|(ngraph)|(magic_enum)|(fmt)|(outcome)|(cereal)|(.*/test)|(3rd)|(build/.*)\""
os.system(
    f'llvm-cov-17 export --ignore-filename-regex={regex} --instr-profile coverage.profdata --format=lcov {" --object ".join(tests)}> coverage.lcov')
os.system(f'llvm-cov-17 show  --instr-profile coverage.profdata{" --object ".join(tests)} --format=html --output-dir coverage-html')
