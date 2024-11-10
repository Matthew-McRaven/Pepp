# Part 10: Parallel linking

It is possible to parallelize the linking process somewhat. This can help hide I/O latency and can take better advantage of modern multi-core systems. My intention with gold is to use these ideas to speed up the linking process.

The first area which can be parallelized is reading the symbols and relocation entries of all the input files. The symbols must be processed in order; otherwise, it will be difficult for the linker to resolve multiple definitions correctly. In particular all the symbols which are used before an archive must be fully processed before the archive is processed, or the linker won’t know which members of the archive to include in the link (I guess I haven’t talked about archives yet). However, despite these ordering requirements, it can be beneficial to do the actual I/O in parallel.

After all the symbols and relocations have been read, the linker must complete the layout of all the input contents. Most of this can not be done in parallel, as setting the location of one type of contents requires knowing the size of all the preceding types of contents. While doing the layout, the linker can determine the final location in the output file of all the data which needs to be written out.

After layout is complete, the process of reading the contents, applying relocations, and writing the contents to the output file can be fully parallelized. Each input file can be processed separately.

Since the final size of the output file is known after the layout phase, it is possible to use `mmap` for the output file. When not doing relaxation, it is then possible to read the input contents directly into place in the output file, and to relocation them in place. This reduces the number of system calls required, and ideally will permit the operating system to do optimal disk I/O for the output file.

Just a short entry tonight. More tomorrow.
