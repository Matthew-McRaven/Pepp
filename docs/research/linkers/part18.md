# Part 18: Incremental Linking

Often a programmer will make change a single source file and recompile and relink the application. A standard linker will need to read all the input objects and libraries in order to regenerate the executable with the change. For a large application, this is a lot of work. If only one input object file changed, it is a lot more work than really needs to be done. One solution is to use an incremental linker. An incremental linker makes incremental changes to an existing executable or shared library, rather than rebuilding them from scratch.

I’ve never actually written or worked on an incremental linker, but the general idea is straightforward enough. When the linker writes the output file, it must attach additional information.

* The linker must create a mapping of object files to areas in the output file, so that an incremental link will know what to remove when replacing an object file.
* The linker must retain all the relocations for each input object which refer to symbols defined in other objects, so that it can reprocess them when symbols change. The linker should store the relocations mapped by symbol, so that it can quickly find the relevant relocations.
* The linker should leave extra space in the text and data segments, to allow for object files to grow to a limited extent without requiring rewriting the whole executable. It must keep a map of where this extra space is, as it will tend to move over time over the course of incremental links.
* The linker should keep a list of object file timestamps in the output file, so that it can quickly determine which objects have changed. With this information, the linker can identify which object files have changed since the last time the output file was linked, and replace them in the existing output file. When an object file changes, the linker can identify all the relocations which refer to symbols defined in the object file, and reprocess them.

When an object file gets too large to fit in the available space in a text or data segment, then the linker has the option of creating additional text or data segments at different addresses. This requires some care to ensure that the new code does not collide with the heap, depending upon how the local malloc implementation works. Alternatively, the incremental linker could fall back on doing a full link, and allocating more space again.

Incremental linking can greatly speed up the edit/compile/debug cycle. Unfortunately it is not implemented in most common linkers. Of course an incremental link is not equivalent to a final link, and in particular some linker optimizations are difficult to implement while acting incrementally. An incremental link is really only suitable for use during the development cycle, which is course the time when the speed of the linker is most important.

More on Monday.
