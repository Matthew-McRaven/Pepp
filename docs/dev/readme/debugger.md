# Add instruction to set SP
* Status: Accepted
* Date: 2025-03-25
* Deciders: Stan, Matthew

## Issue
In Pep/10, a significant number of instructions are executed in the OS prior to calling 0x0000.
At the end of execution, the debugger will show the contents of the OS, not the last line of the user program.

It is confusing to the student that they are consistently confronted with OS details when they have not yet learned about the topic.


## Decision

Create our own expression evaluation library which can grow to fit our needs.


## Constraints

We would like to minimize the amount of unexplainable magic inside the simulator and debugger.
For example, depending on what button was pressed, we arbitrarily execute some number of instructions to skip to the user program.
This state would need to be persisted throughout execution, so that we can tab back to the user program if we are not debugging the OS.

We would like to have conditional breakpoints, because they are a useful debugging tool for students and myself.

We would like to express the "magic" instructions to the debugger using a mechanism similar to our trace tags.
This would allow *debugger actions* to be written in comments within the OS.

We would like the "backend" / AST of the debugger actions to share an implementation with our expression evaluator.

### Constraints on Conditional Breakpoints
Conditional breakpoints are expected to be executed potentially many times.
We can build conditional breakpoints on top of unconditional breakpoints.
If a line has a conditional breakpoint, we pause execution and evaluate the expression(s) attached to the current PC address.
If any expression(s) evaluates to a truthy value, we pause execution.
Otherwise, execution continues AS IF no breakpoint was encountered.
Today's unconditional breakpoints can be expressed as a conditional breakpoint whose expression is a constant.

## Constraints on Expressions
Our expressions should be "cheap" to re-evaluate if the underlying values are unchanged.
This likely means caching the last evaluated value, and clearing that cache on changes in nested terms.

If a nested term changes (e.g., `x` in `(m*x +b) %2 == 0`), all dependent terms should be marked as dirty.

To limit the runtime of this invalidation scheme, we need to de-duplicate identical terms.
That is, all `x` refer to an object at the same address across all expressions.
Otherwise, invalidating a cached value will have a time complexity that is linear in the number of total subterms rather than linear in the number of dependent expressions.
The dirty flag can be used to indicate that we should higlight watch expressions which changed since the last step.

Evaluating terms should be as lazy as possible -- only evaluate terms whose value has been explicitly requested.
This will prevent hangups on breakpoints caused by complex watch expressions.

## Constraints on Debugger Actions
Debugging actions need the ability to place breakpoints with conditions that may be temporary/one-shot.
Other possible actions are to tab back to the user program, to associate a memory address with a type, or to trigger another step.

Debugging actions need to conform in syntax to trace tags.
Furthermore, trace tags should be a form of assembly-time debugging action.

Debugging actions must be optional for the correct execution of the simulator -- they only exist to make the debugging experience easier

Debugging actions must be collected at assembly time and placed in a special NOTE section of the ELF output.

## Positions
Use an existing expression evaluation library, like MYLINK HERE
> Lorem ipsum dolor

Roll our own
> Lorem ipsum dolor, but with reuse between debugger actions

Don't include this feature
> Lorem ipsum dolor, but with simulator magic

## Argument


## Implications
