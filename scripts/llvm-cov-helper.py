import os, sys, shutil
import stat
import re

from pathlib import Path

profraws = [str(profraw.absolute()) for profraw in Path('profraw').rglob('*.profraw')]

print(sys.argv)
print(profraws)

def llvm(name: str) -> str:
    """
    Return the best available LLVM tool on PATH, preferring version-suffixed
    binaries (llvm-profdata-18, -17, ...) and falling back to unversioned.
    """
    for v in range(25, 9, -1):  # adjust upper bound if you like
        p = shutil.which(f"llvm-{name}-{v}")
        if p:
            return p
    p = shutil.which(f"llvm-{name}")
    if p:
        return p
    raise FileNotFoundError(f"llvm-{name} not found on PATH (tried [llvm-{name}-10 , llvm-{name}-25] and llvm-{name})")

os.system(f'{llvm("profdata")} merge --sparse {" ".join(profraws)} --output coverage.profdata')

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
            elif ".o" in filename:
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

glob = "(catch)|(elfio)|(fmt)|(.*/test)|(3rd)|(build)|(spd)|(lexilla)|(bin/ide)|(bin/term)"

tests = f'{" --object ".join(tests)}'
print(tests)
os.system(f'{llvm("cov")} show  --ignore-filename-regex="{glob}" --instr-profile coverage.profdata {tests} --format=html --output-dir coverage-html')
os.system(f'{llvm("cov")} show  --show-branches=count --ignore-filename-regex="{glob}" --instr-profile coverage.profdata {tests} > coverage.txt')
