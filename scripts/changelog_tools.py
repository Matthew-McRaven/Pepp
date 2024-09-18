import pathlib
import argparse
import sqlite3
# This Python file uses the following encoding: utf-8

def generate_changelog(data_dir, output_file, _from=None, to=None): pass

def to_sqlite(data_dir, conn):
  import csv
  cursor = conn.cursor()

  # Create table with info about versions
  cursor.execute("DROP TABLE IF EXISTS versions;")
  cursor.execute(to_sqlite.create_ver_table)
  with open(data_dir/"versions.csv", "r") as f:
    reader = csv.reader(f)
    # Skip first line, which contains headers
    next(reader)
    for vernum,ref,date,blurb in reader:
      major,minor,patch=vernum.split(".")
      cursor.execute("INSERT INTO versions(major, minor, patch, ref, date, blurb) VALUES (?,?,?,?,?,?);", (major, minor, patch, ref, date, blurb) )
  conn.commit()

  # Create table with info about change types
  cursor.execute("DROP TABLE IF EXISTS types;")
  cursor.execute(to_sqlite.create_types_table)
  for name in ["Added", "Changed", "Fixed", "Optimization", "Security", "Deprecated", "Removed"]:
    cursor.execute("INSERT INTO types(name) VALUES(?)", (name,))
  conn.commit()

  # Create table with info about changes
  cursor.execute("DROP TABLE IF EXISTS changes;")
  cursor.execute(to_sqlite.create_changes_table)
  with open(data_dir/"changes.csv") as f:
    reader = csv.reader(f)
    # Skip first line, which contains headers
    next(reader)
    for type, priority, ver, msg in reader:
      # Convert version string into row reference
      cursor.execute("SELECT id FROM versions WHERE version = ?", (ver,))
      if  (ver_row := cursor.fetchone()):  ver_id = ver_row[0]
      else: ver_id = None

      # Convert change type into row reference
      cursor.execute("SELECT name FROM types WHERE name = ?", (type,))
      if  (type_row := cursor.fetchone()):  type_id = type_row[0]
      else: type_id = None

      # Convert string priority to int
      priority_int = 0
      if priority.lower() == "major": priority_int = 2
      elif priority.lower() == "minor": priority_int = 1

      cursor.execute("INSERT INTO changes(version, type, priority, message) VALUES(?,?,?,?)", (ver_id, type_id, priority_int, msg))

  conn.commit()

to_sqlite.create_ver_table="""
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
to_sqlite.create_types_table="""
CREATE TABLE "types" (
	"name"	TEXT UNIQUE COLLATE NOCASE,
	PRIMARY KEY("name")
);
"""
to_sqlite.create_changes_table="""
CREATE TABLE "changes" (
	"version"	INTEGER,
	"type"	INTEGER NOT NULL,
	"priority"	INTEGER,
	"message"	TEXT,
	FOREIGN KEY("type") REFERENCES "types"("name"),
	FOREIGN KEY("version") REFERENCES "versions"("id")
);
"""

def to_qrc(sqlite_db, output_file): pass
def convert_versions(csv_fname): pass
def convert_other(csv_fname): pass
if __name__ == "__main__":
  data_dir = pathlib.Path(__file__).parent.parent / "data" / "changelog"
  conn = sqlite3.connect("here.db")
  to_sqlite(data_dir, conn)
  conn.close()

