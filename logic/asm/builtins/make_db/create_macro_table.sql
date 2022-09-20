/*Create a table us what macros are used in the book.*/
CREATE TABLE macros
(
    id   INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT not null UNIQUE,
    arch TEXT not null,
    body TEXT
) STRICT;
