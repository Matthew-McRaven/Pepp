import pathlib, shutil, os, sys
os.makedirs(f"{sys.argv[2]}", exist_ok=True)
srcFile = pathlib.Path(f"{sys.argv[1]}")
dstDir = pathlib.Path(f"{sys.argv[2]}")

# Copy all files with the same stem as $1 to the directory $2 if the extension is a WASM-related extension.
os.makedirs(dstDir, exist_ok=True)
for ext in [".js", ".wasm"]:
  if (src:=srcFile.with_suffix(ext)).exists(): shutil.copyfile(src.absolute(), dstDir/src.name)

# TODO: Only copy one instance of these files globally.
# We only need one copy of the "new" loader, but the project will fail if there are 0 copies.
if(qtloader:=srcFile.parent/"qtloader.js").exists(): shutil.copyfile(qtloader.absolute(), dstDir/qtloader.name)
# Must copy a logo for thee app to not 404.
# TODO: Use our own logo.
if(logo:=srcFile.parent/"qtlogo.svg").exists(): shutil.copyfile(logo.absolute(), dstDir/logo.name)
