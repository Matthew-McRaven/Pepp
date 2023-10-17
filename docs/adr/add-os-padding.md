# Add Padding Bytes to OS

## Issue

We would like to design the Pep/10 OS with better facilities for being patched after CS6E is released, assuming a major bug is discovered.

## Decision
We will add 64 bytes of padding at the end of each major section of the OS which has an explicitly assigned address in the memory map: the system entry point, the dispatcher, and the trap handler.

If a bug is uncovered, the relevant section will be patched, and the ROM block will be decreased in size to keep the memory map properly aligned.

## Status
Accepted

## Constraints
After the book is released, the memory map must not change.

While it is preferable that the OS text does not change, we can publish an erata should a change be necessary.

## Positions
We could fix OS behaviors in the CPU implementation [like in Pep/9](#notes)?
> This can only address simple changes, like forgetting to clear the high order byte of a register.
> However, this may not cover egregious flaws. 
> For example, if there's a bug in the out-of-bounds computation code for the trap handler, this could not be fixed with a single instruction.
> Any "hardware" workaround, if any were possible, would be complex.

Why don't we clear set every register & status bit to some defined value on entering the operating system?
We could formalize this change into the RTL.
> This would fix the class of bugs that impacted CS5E, but again would not handle larger bugs.
> Additionally, this RTL change would push SCALL to have 3 lines of RTL, further complicating the instruction.

We could add one large block of data somewhere in the OS and write any necessary code there?
The old bits of code could be patched to branch into the "new" implementation.
> There is no reason to keep the old implementation around if a bug is discovered in it.
> Additionally, the implementation would be confusing to a student, since the "real" implementation could be distant in memory from the rest of the function.

## Argument
In CS6E, we will not be showing the full text of the OS.
By adding padding between sections, a bug in the OS would affect **at most** the relevant section of the OS, but remaining sections would be undisturbed.
If we discover a minor bug, we may still opt to fix the bug in hardware like in Pep/9.
However, the addition of padding bits allows us to tackle larger classes of bugs without noticable changes from the user-program's perspective.


## Implications
An additional 192 bytes of memory will be occupied by the OS at all times.

In the event of a major OS bug, the published text and actual OS implementation may differ.
Additionally, we may need some component in the help documentation to indicate that the OS has been modified since the book was published.
## Notes 
Computer Systems, 5th Edition shipped with a bug in the OS.

```pep
oldIR:   .EQUATE 9           ;Stack address of IR on trap
;
trap:    LDBX    oldIR,s     ;X <- trapped IR
         CPBX    0x0028,i    ;If X >= first nonunary trap opcode
         BRGE    nonUnary    ;  trap opcode is nonunary
;
unary:   ANDX    0x0001,i    ;Mask out all but rightmost bit
         ASLX                ;Two bytes per address
         CALL    unaryJT,x   ;Call unary trap routine
         RETTR               ;Return from trap
;
unaryJT: .ADDRSS opcode26    ;Address of NOP0 subroutine
         .ADDRSS opcode27    ;Address of NOP1 subroutine
;
nonUnary:ASRX                ;Trap opcode is nonunary
         ASRX                ;Discard addressing mode bits
         ASRX
         SUBX    5,i         ;Adjust so that NOP opcode = 0
         ASLX                ;Two bytes per address
         CALL    nonUnJT,x   ;Call nonunary trap routine
return:  RETTR               ;Return from trap
```

If the high order bits of the X register had been set by the user program, in the non-unary case the trap handler will perform arithmetic on "junk" data.
The result is then used to select the correct trap implementation, possibly branching to a random memory address.

Since the book was already published, and every line of OS code was covered in CS5E, the only possible option was to modify the trap instructions to clear the X register.
While this fixes the user-observable behavior of the bug, the implementation and formal RTL have diverged.
