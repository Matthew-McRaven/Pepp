#!/usr/bin/env python3
# LexillaGen.py - implemented 2019 by Neil Hodgson neilh@scintilla.org
# Released to the public domain.

"""
Regenerate the Lexilla source files that list all the lexers.
"""

# Should be run whenever a new lexer is added or removed.
# Requires Python 3.6 or later
# Files are regenerated in place with templates stored in comments.
# The format of generation comments is documented in FileGenerator.py.

import os, pathlib, sys, uuid

thisPath = pathlib.Path(__file__).resolve()

sys.path.append(str(thisPath.parent.parent.parent / "scintilla" / "scripts"))

from FileGenerator import Regenerate, UpdateLineInFile, \
    ReplaceREInFile, UpdateLineInPlistFile, UpdateFileFromLines
import LexillaData
import LexFacer

def ciLexerKey(a):
    """ Return 3rd element of string lowered to be used when sorting. """
    return a.split()[2].lower()

def RegenerateAll(rootDirectory):
    """ Regenerate all the files. """

    root = pathlib.Path(rootDirectory)

    lexillaBase = root.resolve()

    lex = LexillaData.LexillaData(lexillaBase)

    lexillaDir = lexillaBase
    srcDir = lexillaDir / "src"
    docDir = lexillaDir / "doc"

    Regenerate(srcDir / "Lexilla.cxx", "//", lex.lexerModules)

    # Discover version information
    version = (lexillaDir / "version.txt").read_text().strip()
    versionDotted = version[0:-2] + '.' + version[-2] + '.' + version[-1]
    versionCommad = versionDotted.replace(".", ", ") + ', 0'

    rcPath = srcDir / "LexillaVersion.rc"
    UpdateLineInFile(rcPath, "#define VERSION_LEXILLA",
        "#define VERSION_LEXILLA \"" + versionDotted + "\"")
    UpdateLineInFile(rcPath, "#define VERSION_WORDS",
        "#define VERSION_WORDS " + versionCommad)
    UpdateLineInFile(docDir / "LexillaDownload.html", "       Release",
        "       Release " + versionDotted)
    ReplaceREInFile(docDir / "LexillaDownload.html",
        r"/www.scintilla.org/([a-zA-Z]+)\d{3,5}",
        r"/www.scintilla.org/\g<1>" +  version,
        0)

    pathMain = lexillaDir / "doc" / "Lexilla.html"
    UpdateLineInFile(pathMain,
        '          <font color="#FFCC99" size="3">Release version',
        '          <font color="#FFCC99" size="3">Release version ' + \
        versionDotted + '<br />')
    UpdateLineInFile(pathMain,
        '           Site last modified',
        '           Site last modified ' + lex.mdyModified + '</font>')
    UpdateLineInFile(pathMain,
        '    <meta name="Date.Modified"',
        '    <meta name="Date.Modified" content="' + lex.dateModified + '" />')
    UpdateLineInFile(lexillaDir / "doc" / "LexillaHistory.html",
        '	Released ',
        '	Released ' + lex.dmyModified + '.')

    lexillaXcode = lexillaDir / "src" / "Lexilla"

    LexFacer.RegenerateAll(root, False)

if __name__=="__main__":
    RegenerateAll(pathlib.Path(__file__).resolve().parent.parent)
