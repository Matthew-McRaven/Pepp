# Part 1: Introduction

I’ve been working on and off on a new linker. To my surprise, I’ve discovered in talking about this that some people, even some computer programmers, are unfamiliar with the details of the linking process. I’ve decided to write some notes about linkers, with the goal of producing an essay similar to my existing one about the GNU configure and build system.

As I only have the time to write one thing a day, I’m going to do this on my blog over time, and gather the final essay together later. I believe that I may be up to five readers, and I hope y’all will accept this digression into stuff that matters. I will return to random philosophizing and minding other people’s business soon enough.

## A Personal Introduction

Who am I to write about linkers?

I wrote my first linker back in 1988, for the AMOS operating system which ran on Alpha Micro systems. (If you don’t understand the following description, don’t worry; all will be explained below). I used a single global database to register all symbols. Object files were checked into the database after they had been compiled. The link process mainly required identifying the object file holding the main function. Other objects files were pulled in by reference. I reverse engineered the object file format, which was undocumented but quite simple. The goal of all this was speed, and indeed this linker was much faster than the system one, mainly because of the speed of the database.

I wrote my second linker in 1993 and 1994. This linker was designed and prototyped by Steve Chamberlain while we both worked at Cygnus Support (later Cygnus Solutions, later part of Red Hat). This was a complete reimplementation of the BFD based linker which Steve had written a couple of years before. The primary target was a.out and COFF. Again the goal was speed, especially compared to the original BFD based linker. On SunOS 4 this linker was almost as fast as running the cat program on the input .o files.

The linker I am now working, called gold, on will be my third. It is exclusively an ELF linker. Once again, the goal is speed, in this case being faster than my second linker. That linker has been significantly slowed down over the years by adding support for ELF and for shared libraries. This support was patched in rather than being designed in. Future plans for the new linker include support for incremental linking–which is another way of increasing speed.

There is an obvious pattern here: everybody wants linkers to be faster. This is because the job which a linker does is uninteresting. The linker is a speed bump for a developer, a process which takes a relatively long time but adds no real value. So why do we have linkers at all? That brings us to our next topic.

## A Technical Introduction

What does a linker do?

It’s simple: a linker converts object files into executables and shared libraries. Let’s look at what that means. For cases where a linker is used, the software development process consists of writing program code in some language: e.g., C or C++ or Fortran (but typically not Java, as Java normally works differently, using a loader rather than a linker). A compiler translates this program code, which is human readable text, into into another form of human readable text known as assembly code. Assembly code is a readable form of the machine language which the computer can execute directly. An assembler is used to turn this assembly code into an object file. For completeness, I’ll note that some compilers include an assembler internally, and produce an object file directly. Either way, this is where things get interesting.

In the old days, when dinosaurs roamed the data centers, many programs were complete in themselves. In those days there was generally no compiler–people wrote directly in assembly code–and the assembler actually generated an executable file which the machine could execute directly. As languages liked Fortran and Cobol started to appear, people began to think in terms of libraries of subroutines, which meant that there had to be some way to run the assembler at two different times, and combine the output into a single executable file. This required the assembler to generate a different type of output, which became known as an object file (I have no idea where this name came from). And a new program was required to combine different object files together into a single executable. This new program became known as the linker (the source of this name should be obvious).

Linkers still do the same job today. In the decades that followed, one new feature has been added: shared libraries.

More tomorrow.
