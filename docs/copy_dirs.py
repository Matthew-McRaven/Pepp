from sphinx.util import logging
import shutil, pathlib

logger = logging.getLogger(__name__)

def setup(app):
    # a?b means copy a to b
    # a can either be relative to the input directory or an absolute path
    # (relative if in relPairs, absolute if in absPairs)
    # b must always be relative to the output directory.

    app.add_config_value('absPairs', [], 'html')
    app.add_config_value('relPairs', [], 'html')
    # Globbing expression for files to copy. Defaults to all files.
    app.add_config_value('exts', ['*'], 'html')
    app.connect('build-finished', copy_dirs)

    # Absolutely not parallel write safe, since multiple source files may map to the same dest file.\
    return {
        'parallel_read_safe': True,
        'parallel_write_safe': False,
    }

# Helper to copy all files matching the glob list ext from
# the src dir into the dest dir
def helper(exts, absSrcDir, absDst):
  # logger.info(f"Copying {absSrcDir} to {absDst}")
  # logger.info(f'Copying {file} to {dir}')#insert into innermost loop.
  if not absDst.exists(): return
  for ext in exts:
    for file in pathlib.Path(absSrcDir).glob(f"{ext}"): shutil.copy(file, absDst)

def copy_dirs(app, exception):
    pairs = []

    # Convert all pairs to sets of absolute paths.
    for pair in app.config.absPairs:
      pair = pair.split("?")
      pairs.append([pair[0], pathlib.Path(app.outdir)/pair[1]])
    for pair in app.config.relPairs:
      pair = pair.split("?")
      pairs.append([pathlib.Path(app.srcdir)/pair[0], pathlib.Path(app.outdir)/pair[1]])

    for pair in pairs: helper(app.config.exts, *pair)
