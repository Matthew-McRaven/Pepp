CREATE TABLE table_of_figures
(
    id          INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,
    arch        TEXT not null,
    chapter_num TEXT not null,
    figure_name TEXT not null,
    /*If "assignment", treat as something a student needs to write. Else if "sample", treat as a user program that can be executed.
      Else if "mmap", it's probably some kind of OS that can't be executed directly.*/
    /* I.E., don't run CI on assignments*/
    kind        TEXT DEFAULT "assignment" not null,
    /*What OS does this figure assume?*/
    default_os  INTEGER,
    /*If is_os, then default OS must be null*/
    is_os       INTEGER,
    FOREIGN KEY (default_os) REFERENCES table_of_figures (id),
    /*Don't allow a default os if this is already an OS.*/
    CHECK ((table_of_figures.is_os IS NULL) != (table_of_figures.default_os IS NULL))
) STRICT;
