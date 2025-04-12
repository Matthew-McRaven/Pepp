https://media.ccc.de/v/38c3-demystifying-common-microcontroller-debug-protocols

Hijack existing trace tags snytax for debugger actions.
That use syntax akin to `@params #myNum`.

@params is a function, and arguments are delimited by #.
 
Some functions modify the previously called function.
For example `@bp@cond#ifOSDebug` creates a BP who is conditioned on if the OS is being debgged.

@params and @locals are actions too.
When executed, they update the stack trace to account for the current instructions.
If only trace tags are provided (#2d), then we need to user a heuristic to determine if we are locals, globals, or params.

Some conditions change how or when a function is evaluated.
@onload causes the "thing" to be evaluated when the program begins debugging, not when the associated line is hit
Modifiers are left-associative to the nearest function.

Modifiers I will need
- @onload
- @cond
- @once

Functions I will need
- @bp(#address)
- @view(#file(#line))
- @watch#expression
- @params(#argList)+
- @locals(#argList)+
- @type#traceTag
- @struct#list#of#members

Not all commands allow all modifiers.
e.g., @struct does not support @cond.

Arguments without commands will use Pep/10 heuristics to guess the command
- If .block, it's a global declaration
- If it's an equate, its a variable or struct
- If it's a line of code
  - and the current line is a target of a branch, we are locals
  - otherwise, we are params
  

@bp@once@onload@cond#ifDebug
@bp#0000@once@onload@cond#!ifDebug
@view#user
@watch#some + complex - expression
