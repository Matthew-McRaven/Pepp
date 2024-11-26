Commit history starts with [Lexilla](https://github.com/ScintillaOrg/lexilla/commit/75b7a21e161f91e77a8c84a730301e0af88e4bd1), and puts all graphics assets in LFS after rewriting history.
I merged in the released code for Scintilla 5.5.0 rather than attempting to merge in its history since it was ~400MB.
I've ported both to build using CMake and have removed the files for other build systems.
Lastly, I've integrated [SciTEQt](https://github.com/mneuroth/SciTEQt/commit/ba7c1df931e3e418e7dce657003000823bf030f7)'s modification to adapt Scintilla to QML.
