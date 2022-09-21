/*Store input:output pairs for a particular figure.*/
CREATE TABLE sample_io
(
    figure_id INTEGER not null,
    input     TEXT,
    output    TEXT,
    FOREIGN KEY (figure_id) REFERENCES macros (id)
);
