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

# Division

I've implemented the following bit-masking divison algorithm for computing the quotient and remainder for signed 16 bit integers.
I've modified existing algorithms described in the reference section to avoid any expensive CLZ instructions and explicit counters.
It returns both the quotient and remainder.
Usually we only want one or the other, but downstream functions can discard whicher result is not needed.
It is impossible for me to calculate one without the other, so I feel no need to hide that fact.

Future modifications would compute the mask to be ceil(log2(dividend), avoiding initial loops which perform shifts on 0s.

The algorithm is as follows:

```python
function divide(dividend, divisor):
    if divisor == 0: throw "Divide by 0"

    # Determine sign of the quotient
    sign = (dividend < 0) ^ (divisor < 0) ? 1 : 0

    quotient, remainder = 0, 0
    abs_dividend, abs_divisor = abs(dividend), abs(divisor)
    
    
    mask = 1 << 15  # Start with the most significant bit

    while mask != 0:
        remainder = remainder << 1
        # Extract the current bit of abs_dividend using the mask
        if abs_dividend & mask: remainder |= 1
        quotient =<< 1 # Set current bit to 0
        if remainder >= abs_divisor:
            remainder -= abs_divisor
            quotient |= 1  # Set the current bit to 1
        mask = mask >> 1

    if sign: quotient = -quotient
    if dividend < 0: remainder = -remainder

    return (quotient, remainder)
```

#### References:
- [Algorithms for division – part 2 – Classics](https://blog.segger.com/algorithms-for-division-part-2-classics)
- Wikipedia: [Division Algorithm](https://en.wikipedia.org/wiki/Division_algorithm).
