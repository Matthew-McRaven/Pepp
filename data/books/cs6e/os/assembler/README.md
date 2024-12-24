# Execution Model
Why no (in)direct threaded code? Where is `DOCOL` or `EXIT`?

At a high level, we don't have assembly primitives to move values from normal registers to PC.
Nor do we have primitives perform non-IX calls.
Therfore, for our architecture, threaded code require one of the following:
1. Self-modifying code (change `BR`/`CALL` target)
  - Does not work for "core" words which reside in RO memory
  - Adds ~5-7 bookkeeping instructions per JT call
  - How do I make the words reentrant?
2. Abuse of RET. That is, push the normal return address on the stack *and* push the target address onto the stack, followed by RET.
  - Adds ~7-12 bookkeeping instructions per JT call
  + With some hackery, it only requires inserting a single 3-byte CALL as the codeword for any new FORTH definition.
  - Which can be viewed as a minus, because of the non-standard "`CALL` via `RET`" it encourages.
3. Insert a CALL,x loop to start each FORTH definition
  - Each native word would be forced to save/restore PSP, costing 6 bytes per definition.
  - Adds ~18B in native code to each FORTH definition.
  - Adds ~5-7 bookkeeping instructions per JT call
  + No unconventional abuse of normal opcodes.
4. CALL,sfx. This would let us push a jump table onto the stack and centralize the jump machinery describer in the previous line
  - Requires modifying instruction set
  - Adds ~5-7 bookkeeping instructions per JT call
  + Only ~4B overhead (`CALL` + `RET`) in FORTH definition

By eschewing threaded code and emitting `CALL ADDRESS` instead, we incur a ~33% code size penalty to FORTH definitions.
Due to extreme register pressure in our register set and lack of instruction like LODSL, our next word pointer cannot remain resident in a register.
Fetching and updating this value from memory will incur a ~3-cycle penalty per access.
Lack of non-IX calls would also incur further overhead transfering the value to PC.

So, our FORTH words will technically just be a series of `CALL`s followed by a single `RET`.
