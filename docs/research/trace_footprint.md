# Trace footprint: TVM programs vs. sim3 packets

I measured the trace format on `hwdebug` (TVM programs in a `tvm::TraceBuffer`) against the format it replaces (`lib/sim3`, variant fragments serialized with zpp_bits into an `InfiniteBuffer`).
The goal is to figure out if the new trace format has the expected memory savings.

These numbers are from before I started optimization, as a sanity check to see if it was worth continuing in this direction.
Prior to optimization, the results are a bit of a toss up.
With optimization, I definitely favor the new format.

## Method

There is a test (`test/core/sim/hwdbg/trace_footprint.cpp`) which contains a Pep/10 CPU, a 64 KiB `Dense` main memory, and a `trace::BufferDevice`, with tracing enabled on main memory, the CPU register bank, and the CSRs.
There are two loops of 3k instructions each, differing in only one respect:

| loop | inner body | why |
|---|---|---|
| **fixed** | `ADDA 1,i` / `STWA 0x9000,d` / `BR` | store address constant, so every instructions touch the same set of device addresses |
| **walking** | `ADDA 1,i` / `STWA 0x9000,x` / `ADDX 2,i` / `BR` | store address advances, so instructions touch different addresses |

Numbers come from `tvm::TraceBuffer::footprint()`, which records what was actually written to the code and data chains alongside a counter which assumes that no form of compression is enabled.
Both figures describe the *same* execution.

sim3 numbers are derived via source inspection to avoid instantiating an the old system in our new code.
The derivation is shown below so it can be checked or falsified.

## Measured

```
before optimization:
fixed-address store  : 33.5 B/instr (inlined: 112.0) | ratio 3.344 | code 66236  templates 260 data 34004
                       3 templates,   0 pending | 512 KiB reserved
walking-address store: 49.2 B/instr (inlined: 122.0) | ratio 2.480 | code 111292 templates 316 data 36004
                       3 templates, 750 pending | 512 KiB reserved
 
with-DP-relative-addressing:
fixed-address store  : 28.8 B/instr (inlined:  98.0) | ratio 3.401 | code 48208  templates 232 data 38004
                       3 templates,   0 pending | 256 KiB reserved
walking-address store: 29.2 B/instr (inlined: 101.0) | ratio 3.459 | code 48288  templates 320 data 39004
                       4 templates,   0 pending | 256 KiB reserved
 
with-PC-coalescing:
fixed-address store  : 26.1 B/instr over 3000 (inlined: 83.3) | ratio 3.190
                       code 24172  templates 196  data 30006  locations 24000 | 3 templates, 0 pending
walking-address store: 26.7 B/instr over 3000 (inlined: 87.5) | ratio 3.281
                       code 24244  templates 276  data 31506  locations 24000 | 4 templates, 0 pending
                        
STEPMEM-for-PC
fixed-address store :  24.8 B/instr over 3000 instrs (inlined: 82.7) | ratio 3.335 
                       code 24174 stencils 198 data 26004 locations 24000 | 3 stencils promoted, 0 hashes pending | 256 KiB reserved
walking-address store: 25.2 B/instr over 3000 instrs (inlined: 87.0) | ratio 3.456 
                       code 24248 stencils 280 data 27004 locations 24000 | 4 stencils promoted, 0 hashes pending | 256 KiB reserved
PACK-NZVC               
fixed-address store :  23.8 B/instr over 3000 instrs (inlined: 81.7) | ratio 3.433 
                       code 24174 stencils 198 data 23001 locations 24000 | 3 stencils promoted, 0 hashes pending | 256 KiB reserved
walking-address store: 23.7 B/instr over 3000 instrs (inlined: 85.5) | ratio 3.612 
                       code 24248 stencils 280 data 22501 locations 24000 | 4 stencils promoted, 0 hashes pending | 256 KiB reserved
                        
Remove redundant HALTs
fixed-address store :  22.4 B/instr over 3000 instrs (inlined: 67.7) | ratio 3.015
                       code 24174 stencils 160 data 18998 locations 24000 | 3 stencils promoted, 1 hashes pending | 256 KiB reserved
walking-address store: 22.2 B/instr over 3000 instrs (inlined: 68.5) | ratio 3.090 
                       code 24286 stencils 218 data 17998 locations 24000 | 4 stencils promoted, 2 hashes pending | 256 KiB reserved
                        
Fused opcode CALLHALT
fixed-address store :  20.4 B/instr over 3000 instrs (inlined: 67.7) | ratio 3.309
                       code 18182 stencils 160 data 18998 locations 24000 | 3 stencils promoted, 1 hashes pending | 256 KiB reserved
walking-address store: 20.2 B/instr over 3000 instrs (inlined: 68.5) | ratio 3.396      
                       code 18298 stencils 218 data 17998 locations 24000 | 4 stencils promoted, 2 hashes pending | 256 KiB reserved
 
Stencil-indexed CALL (STCALL)
fixed-address store :  18.4 B/instr over 3000 instrs (inlined: 67.7) | ratio 3.668
                       code 12190 stencils 160 data 18998 locations 24000 | 3 stencils promoted, 1 hashes pending | 256 KiB reserved
walking-address store: 18.2 B/instr over 3000 instrs (inlined: 68.5) | ratio 3.769
                       code 12310 stencils 218 data 17998 locations 24000 | 4 stencils promoted, 2 hashes pending | 256 KiB reserved 
 
Compress location buffer (v1)
fixed-address store :  13.7 B/instr over 3000 instrs (inlined: 63.0) | ratio 4.582
                       code 12190 stencils 160 data 18998 locations 9874 | 3 stencils promoted, 1 hashes pending | 256 KiB reserved
walking-address store: 13.5 B/instr over 3000 instrs (inlined: 63.8) | ratio 4.737
                       code 12310 stencils 218 data 17998 locations 9875 | 4 stencils promoted, 2 hashes pending | 256 KiB reserved

Compress location buffer (as committed, v2)
fixed-address store :  12.8 B/instr over 3000 instrs (inlined: 62.0) | ratio 4.846 
                       code 12190 stencils 160 data 18998 locations 7040 | 3 stencils promoted, 1 hashes pending | 256 KiB reserved
walking-address store: 12.5 B/instr over 3000 instrs (inlined: 62.9) | ratio 5.019 
                       code 12310 stencils 218 data 17998 locations 7041 | 4 stencils promoted, 2 hashes pending | 256 KiB reserved
```

Note the `locations` is a per-instruction cost incorrectly accounted for in the first 2 rows.



## Deriving the size of sim3 packets

Fragment sizes, from `lib/sim3/api/traced/`:

- **variant tag** — 1 byte. zpp_bits defaults a variant's discriminant to `std::byte{Index}` when no `serialize_id`
  is declared, and none of the sim3 fragment types declare one.
- **`frame::header::Trace`** — tag(1) + `u16 length`(2) + `varint back_offset`(1) = **4 B**
- **`packet::header::Write`** — tag(1) + `varint device`(1) + `varint path`(1) + `VariableBytes` address (1 length
  byte + N address bytes). Per the note in `trace_packets.hpp`, Pep/10 registers use a 1-byte address and main
  memory 2: **5 B** for a register write, **6 B** for a memory write.
- **`payload::Variable`** — tag(1) + length(1) + N bytes: **4 B** for a 2-byte write, **6 B** for a 4-byte one.

So a 2-byte register write costs 9, a 4-byte CSR write 11, a 2-byte memory write 10.

The PC- and NZVC-write-coalescing could have benefitted the sim3 as well, so I will subtract it out for fairness.

| | writes/instr | sim3 fixed (bytes)| sim3 walking (bytes) |
|---|---:|---:|---:|
| original CPU (4 separate CSR writes, PC written 2.33x) | 6.33 | 60.0 | 65.2 |
| assuming coalesed writes (one CSR write, PC written once) | 4.00 | **41.0** | **43.5** |

### sim3 cost

Fixed loop:
- `ADDA` = 5 register writes x (5+4) + 4 CSR writes x (5+3) = 45 + 32 = 77
- `STWA` = 4 x 9 + (6+4) = 46
- `BR` = 5 x 9 = 45
- average 56, plus a 4-byte frame header = **60.0 B/instr**

Walking loop: `ADDA`(77) + `STWA`(46) + `ADDX`(77) + `BR`(45) = 245 / 4 = 61.25, plus frame = **65.3 B/instr**.


## Comparing the formats

| | sim3 | new, with stencils | new, without stencils |
|---|---:|---:|---:|
| fixed loop (B/instr)| 41.0 | 13.7  | 63.0  |
| walking loop (B/instr)| 43.5 | 13.5  | 63.8  |

sim3 also records no timestamps so this comparison likely underestimates the true savings.


## Previous format improvements

Projections were made before each change; measured values are what landed.
1. **Constant `ISYN` moved from prefix to body.** The prefix is inlined into every program and never hashed, so a
   constant tick cost its full six bytes forever.
2. **Location buffers allocated lazily and returned on `acknowledge()`**. Reduced reservation from 512 → 256 KiB.
3. **`write_packed_csr` coalesced** from four 1-byte writes to one 4-byte write.
4. **`SETMEMDX`** — target offset carried in the payload, selected per device. Must not be applied unconditionally: the offset costs 4 bytes per *write*, so
   turning it on everywhere would have made the fixed loop ~60% worse.
5. **Data pointer moved into the location buffer.** The absolute `LDP` at the head of every program named a buffer
   that differed every execution, so it could never join a template. Trading 8 code bytes for 4 more location bytes
   left the code stream as pure `CALL` + `HALT`. 
6. **PC coalesced to one write per instruction.** PC updates two or three times inside one instruction but only the last value is visible at the ISA level.
7. **CALLHALT** removed a 2-byte HALT instruction in the common case where the postfix is empty.
8. **STCALL** compresses a 6-byte call into a 4-byte call with extra assistance from the trace buffer
9. Rather than using fixed 8-byte `Location` values, use compressed, variable-size integer offsets.
   Insert a checkpoint ever *N* bytes to make random seeking easier.

## Future Work

1. **Create architecture-specific fetch/decode instruction(s)**.
   Fetch/decode bookkeeping is 60% of data payload.
   `IS`, `OS` and `PC` are 6 of the 10 payload bytes, and all three are derivable from the program image plus the previous record's PC.
   Not recording them would take data 10 → 4 at the cost of having to issue target memory reads on decode.
   These changes would need to be implemented per-architecture
2. With independent per-inititiator data chains, we could actually combine the initiator's code + data.
   This would reduce trace overhead by ~25% in the average case by reducing internal fragmentation.
   Data would still be eagerly committed to the chain, with the program append after that data.
   Compressed locations would be able to have the two members be relative to each other.
   With most traces being <20B/instr, this would likely allow most trace locations to be encoded with 2B.

## Reproducing

```
./test-core 
```

The harness reports through `SPDLOG_WARN`, so the numbers appear in the test log on a passing run.
