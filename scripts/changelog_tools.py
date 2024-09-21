import pathlib
import argparse
import sqlite3

data_dir = pathlib.Path(__file__).parent.parent / "data" / "changelog"

def to_sqlite(args):
    create_ver_table ="""
CREATE TABLE "versions" (
        "id"	INTEGER UNIQUE, 
        "major"	INTEGER NOT NULL,
        "minor"	INTEGER NOT NULL,
        "patch"	INTEGER NOT NULL,
        "ref"	TEXT,
        "date"	TEXT,
        "blurb"	TEXT,
        "version" TEXT GENERATED ALWAYS AS (major || '.' || minor || '.' || patch) STORED,
        PRIMARY KEY("id" AUTOINCREMENT),
        UNIQUE(major, minor, patch)
);
"""
    create_types_table="""
CREATE TABLE "types" (
        "name"	TEXT UNIQUE COLLATE NOCASE,
        PRIMARY KEY("name")
);
"""
    create_changes_table="""
CREATE TABLE "changes" (
        "version"	INTEGER,
        "ref"           INTEGER,
        "type"	INTEGER NOT NULL,
        "priority"	INTEGER,
        "message"	TEXT,
        FOREIGN KEY("type") REFERENCES "types"("rowid"),
        FOREIGN KEY("version") REFERENCES "versions"("id")
);
"""
    import csv
    conn = sqlite3.connect(args.db)
    cursor = conn.cursor()

    # Create table with info about versions
    cursor.execute("DROP TABLE IF EXISTS versions;")
    cursor.execute(create_ver_table)
    with open(data_dir/"versions.csv", "r") as f:
        reader = csv.reader(f)
        next(reader)
        for vernum,ref,date,blurb in reader: # Skip first line, which contains headers
            major,minor,patch=vernum.split(".")
            cursor.execute("INSERT INTO versions(major, minor, patch, ref, date, blurb) VALUES (?,?,?,?,?,?);", (major, minor, patch, ref, date, blurb))
    conn.commit()

    # Create table with info about change types
    cursor.execute("DROP TABLE IF EXISTS types;")
    cursor.execute(create_types_table)
    for name in ["Added", "Changed", "Fixed", "Optimization", "Security", "Deprecated", "Removed"]:
        cursor.execute("INSERT INTO types(name) VALUES(?)", (name,))
    conn.commit()

    # Create table with info about changes
    cursor.execute("DROP TABLE IF EXISTS changes;")
    cursor.execute(create_changes_table)
    with open(data_dir/"changes.csv") as f:
        reader = csv.reader(f)
        next(reader) # Skip first line, which contains headers
        for type, priority, ver, ref, msg in reader:
            # Convert version string into row reference
            cursor.execute("SELECT id FROM versions WHERE version = ?", (ver,))
            if (ver_row := cursor.fetchone()): ver_id = ver_row[0]
            else: ver_id = None

            # Convert change type into row reference
            cursor.execute("SELECT rowid FROM types WHERE name = ?", (type,))
            if (type_row := cursor.fetchone()): type_id = type_row[0]
            else: type_id = None

            # Convert string priority to int
            priority_int = 0
            if priority.lower() == "major": priority_int = 2
            elif priority.lower() == "minor": priority_int = 1

            cursor.execute("INSERT INTO changes(version, type, priority, message, ref) VALUES(?,?,?,?,?)",
                           (ver_id, type_id, priority_int, msg,ref))
    conn.commit()


def to_text(args): raise NotImplemented()

def normalize(args): raise NotImplemented()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Command Line Tool")
    subparsers = parser.add_subparsers(dest='command', help='Available subcommands')

    # Subcommand: to_sql
    parser_to_sql = subparsers.add_parser('to_sql', help='Convert to SQL')
    parser_to_sql.add_argument('db', help='Path to output SQLite DB')
    parser_to_sql.set_defaults(func=to_sqlite)

    # Subcommand: to_text
    parser_to_text = subparsers.add_parser('to_text', help='Create a MD changelog for a range of versions')
    parser_to_text.add_argument('changelog', help='File into which to write changelog')
    parser_to_text.add_argument('--from', dest='_from', help='First version to include in changelog', default=None)
    parser_to_text.add_argument('--to', dest='to', help='Last version to include in changelog', default=None)
    parser_to_text.set_defaults(func=to_text)

    # Subcommand: null_to_latest
    parser_normalize = subparsers.add_parser('null_to_latest', help='Convert NULL versions to use latest version')
    parser_normalize.set_defaults(func=normalize)

    args = parser.parse_args()
    if hasattr(args, 'func'): args.func(args)
    else: parser.print_help()


