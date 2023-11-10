# Generate the index.rst file for the wasm demo applications
# Add an entry to the index.rst file for each wasm demo included in $1
# Write the index (and an rst wrapper for the demo) back to $1.
import glob, sys, os, pathlib

# Placeholder for the index
parts= [f"""\
Demo Applications
==================

.. toctree::
   :maxdepth: 1
   :caption: Contents:

"""]
# Placeholder for the wrapper RST for each demo.
doc_placeholder = """\
{}
===============================

.. raw:: html
   :file: ./{}.html

"""
# Ensure the input/output directory exists
os.makedirs(f"{sys.argv[1]}", exist_ok=True)
destDir = pathlib.Path(f"{sys.argv[1]}")

# Find all HTML files in that directory, and generate a RST wrapper which embeds that HTML
# Add a toc entry from the index to the demo app.
for filename in destDir.glob("*.html"):
  path = filename.stem
  # Add -gen
  parts.append(f"   {path}-gen")
  with open(destDir/f"{path}-gen.rst", 'w') as f: f.write(doc_placeholder.format(path, path))
with open(destDir/"index.rst", 'w') as index: index.write("\n".join(parts))
