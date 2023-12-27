# Notes on ANTLR generated sources

While it is usually bad form to include generated files, I have made an exception here.
I don't want everyone to have to download Java 11 and the ANTLR tools to build this application.

If you want to regenerate the grammar's files, execute `antlr4  -Dlanguage=Cpp Expr.g4` in the antlr directory.
Commit the updated sources.

At some point in the future, I may auto-detect antlr tools being installed, but will pass on this for now.
