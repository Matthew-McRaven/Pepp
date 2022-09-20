/* Store the contents of each figure. Each figure may have multiple versions (C/asmb/object code, etc.). */
CREATE TABLE figures
(
    figure_id INTEGER not null,
    kind      TEXT    not null,
    body      TEXT,
    /*Some figures are derived from others. If this field is present, can ignore body*/
    linked    INTEGER,
    FOREIGN KEY (linked) REFERENCES table_of_figures (id),
    FOREIGN KEY (figure_id) REFERENCES table_of_figures (id),
    CHECK ( (body IS NULL) != (linked IS NULL) )
) STRICT;
