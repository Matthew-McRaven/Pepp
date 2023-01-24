import argparse
from pathlib import Path
import os

from .gen import create_book


def main(dest_dir):
    script_dir = Path(__file__).parent.absolute()
    parent = script_dir.parent
    # Start looking for books
    [root, book_dirs, __] = next(os.walk(parent / "books"))
    for book in book_dirs:
        create_book(Path(root) / book, Path(dest_dir))
    # toc = json.load()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Tool to create databases from book figures.")
    parser.add_argument("dest", help="Directory in which to write the databases")
    args = parser.parse_args()
    main(args.dest)
