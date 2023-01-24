import sqlite3
import json
import os
from pathlib import Path


# Helper to read SQL scripts relative to this file.
def read_relative(path):
    script_dir = Path(__file__).parent.absolute()
    path = script_dir / path
    with open(path) as f:
        contents = ''.join(f.readlines())
    return contents


# Turn a combined name into ch,fig.
def split_name(name):
    return name.split(":")


# Given a colon:separated figure name, find the ID associated with that name, or None if not present in DB
def figure_id_from_name(db, name):
    ch, fig = split_name(name)
    cursor = db.cursor()
    query = cursor.execute("SELECT id FROM table_of_figures WHERE (chapter_num=? AND figure_name=?)", (ch, fig))
    fetched = query.fetchone()
    if fetched: return fetched[0]
    return None


# Create all the tables we need in a new database
def create_tables(db):
    cursor = db.cursor()
    cursor.execute(read_relative("create_metadata_table.sql"))
    cursor.execute(read_relative("create_macro_table.sql"))
    cursor.execute(read_relative("create_tof_table.sql"))
    cursor.execute(read_relative("create_figure_table.sql"))
    cursor.execute(read_relative("create_sample_io.sql"))
    db.commit()


# Set database version, name of the book, authors, etc.
def fill_metadata(db, toc):
    cursor = db.cursor()
    cursor.execute(f"""PRAGMA user_version = {toc["dbVersion"]}""")
    cursor.execute("INSERT INTO about (name) VALUES (?);", (toc["bookName"],))
    db.commit()


# Declare that a figure exists, but do not register the actual text of the figures
def register_with_figure_table(db, manifest):
    ch, fig = split_name(manifest['name'])

    is_os, default_os = None, None
    kind = None

    # Attempt to enforce mutex that is_os and default_os will not both be set
    if "is_os" in manifest and manifest["is_os"]:
        is_os = True
    else:
        default_os = figure_id_from_name(db, manifest['default_os'])

    # Extract if present, otherwise db will set default value.
    if "kind" in manifest: kind = manifest['kind']

    cursor = db.cursor()
    cursor.execute("""INSERT INTO table_of_figures (arch, chapter_num, figure_name, kind, default_os, is_os) VALUES
                      (?,?,?,?,?,?)""", (manifest['arch'], ch, fig, kind, default_os, is_os))
    figure_id = cursor.lastrowid
    db.commit()

    return figure_id


# Insert a figure variant (PEP/C) that is the output of some other figure
def insert_variant_linked(db, figure_id, kind, linked_id):
    cursor = db.cursor()
    cursor.execute("INSERT INTO figures (figure_id, kind, linked) VALUES (?, ?, ?)",
                   (figure_id, kind, linked_id))
    db.commit()


# Insert a figure variant (PEP/C) that stands alone
def insert_variant_unlinked(db, figure_id, kind, body):
    cursor = db.cursor()
    cursor.execute("INSERT INTO figures (figure_id, kind, body) VALUES (?, ?, ?)",
                   (figure_id, kind, body))
    db.commit()


# Insert a single input/output pair
# TODO: Add CHECK that both input/output are not empty
def insert_io_pair(db, figure_id, i, o):
    cursor = db.cursor()
    cursor.execute("""INSERT INTO sample_io (figure_id, input, output) VALUES(?,?,?)""", (figure_id, i, o))
    db.commit()


# Given a manifest for a figure (and its ID), insert all of its variants (PEP/PEPB/PEPH/C/etc.) into the figures table.
def insert_variants(db, directory, figure_id, manifest):
    ch, fig = split_name(manifest['name'])

    # Enumerate variants of
    for (figure_type, body_file) in manifest['items'].items():
        # We allow replacement of {fig}{ch} to make writing manifests less repetitive.
        body_file = body_file.format(**{'fig': fig, 'ch': ch})

        # If the figure contains a ":", then this particular variant is linked to another figure
        if ":" in body_file:
            insert_variant_linked(db, figure_id, figure_type, figure_id_from_name(db, body_file))
        # Otherwise, we just have body text.
        else:
            with open(directory / body_file) as f:
                body = ''.join(f.readlines())
            insert_variant_unlinked(db, figure_id, figure_type, body)

    # Load input/output pairs into sample_io table if the ios field is present.
    if not "ios" in manifest: return
    for io_dir in manifest['ios']:
        # Input or output may not exist
        i, o = None, None
        try:
            with open(directory / io_dir / "input.txt") as f:
                i = ''.join(f.readlines())
        except:
            pass

        try:
            with open(directory / io_dir / "output.txt") as f:
                o = ''.join(f.readlines())
        except:
            pass
        insert_io_pair(db, figure_id, i, o)


# Given a macro manifest, insert all of its items into the macro table
def insert_macro(db, directory, manifest):
    arch = manifest['arch']
    cursor = db.cursor()
    for (name, value) in manifest["items"].items():
        # We allow {name} to be replaced, to make macros easier to write.
        body_file = value.format(name=name)
        # Macros are just text, so open and insert into DB.
        with open(directory / body_file) as f:
            body = ''.join(f.readlines())
        cursor.execute("""INSERT INTO macros (name, arch, body) VALUES (?,?,?)""", (name, arch, body))
    db.commit()
    pass


# Helper function that recursively enumerates folders inside of the book_root looking for manifests.
# Registers every figure before inserting any of the figure variants.
# This makes the process of linking between figures much easier.
def discover_content(db, book_root):
    # name:Array<name> pairs. Values in the array require that the key be registered before they can be processed.
    pending_figure_registrations = {}
    # Due to our 2-pass algorithm, we need to track what figures are still pending insertion into variant table
    # is an Array<figure manifest>
    pending_variants = []
    # We need some way to pass directory information from the first pass to the second.
    # name:directory pairs where name is the name field of the manifest and directory is the directory
    # where manifest was loaded from.
    name_dir_mapping = {}

    # Sometimes when we register a figure, it unblocks other figures.
    # This algorithm will recursively find and "do" figure registration for pending figures.
    def walk_registration_backlog(latest_name):
        # used to recursively process all figures at the end, rather than in a depth-first fashion.
        # Depth-first iteration could invalidate iterators below, which would make the iteration algorithm more complex.
        unfrozen = []
        if latest_name not in pending_figure_registrations: return
        # Extract all manifests that depended on last_name, and perform registration steps
        for manifest in pending_figure_registrations[latest_name]:
            figure_id = register_with_figure_table(db, manifest)
            pending_variants.append((figure_id, manifest))
            unfrozen.append(manifest['name'])

        # Clear backlog for any figures that depended on one of the children of last_name
        for name in unfrozen:
            walk_registration_backlog(name)
            # Remove items that have been registered
            pending_figure_registrations[latest_name] = [x for x in pending_figure_registrations[latest_name] if x['name'] != name]
        # If no items are pending registration, then there's no point in keeping key around.
        for key in list(pending_figure_registrations.keys()):
            if len(pending_figure_registrations[key]) == 0: del pending_figure_registrations[key]

    # Helper function to load manifest, and either insert as figure or macro.
    # Figure mode does some cleverness with the pending_figure_regitration if the manifest depends on an OS that has not
    # been loaded yet. Anytime a figure is registered, must attempt to clear backlog.
    def insert_object(db, dir):
        with open(dir / "manifest.json") as f:
            manifest = json.load(f)
        # Do one pass where we gather all figure names
        # Then do a second pass where we fill in links (with possible endless loops!)

        if manifest["type"] == "figure":
            # Case where figure depends on an OS
            if "default_os" in manifest:
                # We must track the directory, or backlog/2nd pass will be unsure where files should be loaded from
                name_dir_mapping[manifest['name']] = dir

                # If os/os_id are empty, then we haven't encountered the OS yet, and we need to defer registation
                os = manifest['default_os']
                os_id = figure_id_from_name(db, os)
                if os_id is not None:
                    figure_id = register_with_figure_table(db, manifest)
                    pending_variants.append((figure_id, manifest))
                else:
                    if os not in pending_figure_registrations: pending_figure_registrations[os] = []
                    pending_figure_registrations[os].append(manifest)
            # We are likely an OS, so we can be registered immediately
            else:
                name_dir_mapping[manifest['name']] = dir
                figure_id = register_with_figure_table(db, manifest)
                pending_variants.append((figure_id, manifest))
                walk_registration_backlog(manifest['name'])
        elif manifest["type"] == "macro":
            insert_macro(db, dir, manifest)

    # Initiate first pass
    for [root, _, content] in os.walk(book_root):
        if "manifest.json" in content:
            insert_object(db, Path(root))
    assert (len(pending_figure_registrations) == 0 and "Some links are unresolved!")

    # Perform 2nd pass where we insert
    for (figure_id, manifest) in pending_variants:
        insert_variants(db, name_dir_mapping[manifest["name"]], figure_id, manifest)


# Create a single book DB by creating tables and inserting figures+macros.
def create_book(book_root_dir, dest_dir=""):
    if dest_dir == "": dest_dir = book_root_dir
    toc = {}
    try:
        with open(book_root_dir / "toc.json") as f:
            toc = json.load(f)
    except:
        return

    os.makedirs(dest_dir, exist_ok=True)
    db_path = dest_dir / toc["dbName"]

    # Purge existing DB so that we don't have to add DROP IF EXISTS ... commands to our SQL.
    try:
        os.remove(db_path)
    except:
        print("No existing DB")
    db = sqlite3.connect(db_path)
    create_tables(db)
    fill_metadata(db, toc)
    discover_content(db, book_root_dir)
    db.close()
