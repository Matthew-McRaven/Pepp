# Part 20: Update on gold's status

This will be my last blog posting on linkers for the time being. Tomorrow my blog will return to its usual trivialities. People who are specifically interested in linker information are warned to stop reading with this post.

I’ll close the series with a short update on gold, the new linker I’ve been working on. It currently (September 25, 2007) can create executables. It can not create shared libraries or relocateable objects. It has very limited support for linker scripts–enough to read /usr/lib/libc.so on a GNU/Linux system. It doesn’t have any interesting new features at this point. It only supports x86. The focus to date has been entirely on speed. It is written to be multi-threaded, but the threading support has not been hooked in yet.

By way of example, when linking a 900M C++ executable, the GNU linker (version 2.16.91 20060118 on an Ubuntu based system) took 700 seconds of user time, 24 seconds of system time, and 16 minutes of wall time. gold took 7 seconds of user time, 3 seconds of system time, and 30 seconds of wall time. So while I can’t promise that it will stay as fast as all features are added, it’s in a pretty good position at the moment.

I’m the main developer on gold, but I’m not the only person working on it. A few other people are also making improvements.

The goal is to release gold as a free program, ideally as part of the GNU binutils. I want it to be more nearly feature complete before doing this, though. It needs to at least support -shared and -r. I doubt gold will ever support all of the features of the GNU linker. I doubt it will ever support the full GNU linker script language, although I do plan to support enough to link the Linux kernel.

Future plans for gold, once it actually works, include incremental linking and more far-reaching speed improvements.
