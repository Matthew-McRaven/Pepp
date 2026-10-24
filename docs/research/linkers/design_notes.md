# Re-use
A lot of smart people have spent years writing linkers and I don't pretend to be smarter than them.
Re-use will be the name of the game.
Fortunately, mold is an MIT licensed linker which I can extract a lot of useful code and tests from, with attribution.

Over the last 2-3 years, I have evaluated modifying mold for Pep/10 support and bundling mold with our application.
I don't think this is a good strategy, for several reasons:
- mold performs no memory cleanup! mallocs are not followed by frees.
  mold's design uses a double-fork and detach so that as soon as linking is done that outer process exits.
  Reclaiming memory is delegated to the OS.
  I don't have that luxury if mold is running inside our process, and mold has to run inside our process because we target wasm
- The explicitly parallel design makes linking fast, but WASM thread support is...poor.
  At best, we can have "opt in" threading, where desktop targets are threaded but WASM targets remain serial.
  In 2023 I was able to make a fork of mold with all parallelism removed.
  In the time it took me to make those changes, mold had gathered another few dozen commits.
  My changes were so invasive I'd never be able to consume those upstream improvements.
- mold is now written in rust.
  Pepp's build process is already slow and pulls in many dependencies.
  I don't want to add a whole new language as a maintenance burden.
  I can work off of the last C++ version (v2.42.1), which partially solves the upstream improvement problem.

What I think I can re-use are a lot of library implementation details:
- globbing utilities for sections
- unit tests over linker outputs
- symbol resolution strategies
- Algorithms powering ICF / garbage collection / etc

## Dependencies of Mold
BLAKE3 seems to only be used for computing build IDs, so we may be able to avoid compiling that dependency for some time.

mimalloc seems to be only used to globally replace the allocators. We can't do that--I don't want to muck with Qt's default allocator.

Mold is explicitly parallel, implemented via tbb. I like the idea of parallelizing this process, but tbb doesn't compile in WASM.
I should try to keep a similar coding style so that one day I /could/ parallelize the whole thing, but that will require new thread-safe data structures

# Priorities
Based on the commit log, it took Rui Ueyama a good few months to get a working version of Mold.
I'm trying to have this done by thanksgiving.
Clearly I will make compromises on the implementation to reach my goal.

I do not intend to support dynamic linking, ICF, or implement architectures outside of RV and Pep/N for now.
Features like relaxation will be skipped initially too due to their complexity.
Compressed readers+writers are also skippable.
The goal is really to support the minimum set of features needed to link pep/10 programs as soon as possible.

# Design Ideas
I want to re-use my custom ELF library as much as possible.
I may need new section types, storage types, or algorithms over pure ELF files.

As much as possible, I want to avoid redundant, temporary allocations where in-place algorithms would do.
This likely means that I need to "fix" the size of the output object file ASAP, so that I can use that object file as scratch space.

I want all linking allocations (other than the memory-mapped files) to belong to the same mimalloc heap.
Inspecting the memory usage of a link separate from the rest of the application will give me ideas for how to optimize it.
The assembler already has a TODO on switching to a private heap; the linker is just the first victim.

# Getting started
As always in Pepp, I want to deliver incremental updates that are testable.
To that end, I've identified the following features which I should be able to work on independently:
- Add `.a` support for thick/thin archives in my ELF library.
- Import mold's globbing utilities, which I will use to match the REs for section names in a linker script
- Refactor mold's unit tests so they do not include a large number of sh files.
  I'd rather them be expressed in pure cmake, somehow.
  This likely means comitting object code inputs + golden outputs.
  Probably requires a separate repo so that I can cache those inputs without massively bloating the size of this repo.
  That repo can be shallow-cloned and those tests gated behind a cmake flag.
- Some kind of "linker context", which lets me group together all of the input elf files into a single class.
  A pair of integers would let me identify any section within that input.
  Basically every linking algorithm would be an impure function over that section group.
- Symbol resolution over a linker context.
  As the name says, extract the undefined symbols and identify the best matches.