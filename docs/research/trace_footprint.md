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
```

Progress on the fixed loop.
Note the `locations` is a per-instruction cost from the beginning that was incorrectly counted on the first two rows.
STEPMEM-for-PC uses a more efficient encoding for non-branch PC updates.
PACK-NZVC adjusts the layout of the CSRs in host memory to fit in 1 byte rather than 4.


### Allocation breakdown

| component | B/instr | share | shareable? |
|---|---:|---:|---|
| data payload | 10.00 | 38% | no |
| location buffer | 8.00 | 31% | no — one `ProgramLocation` per program |
| `CALL` + `HALT` | 8.06 | 31% | — |
| templates, amortized | 0.07 | 0.3% | — |


There is nothing shareable left in the code stream.
Every byte of it is either the per-program data anchor, the call into the shared body, or the terminator.
Bodies averaged 85.3 B/instr inlined and collapsed to a 6-byte `CALL`, with all templates costing 232 bytes total — a **14x reduction** on the shareable portion.

The walking loop promotes 4 templates against the fixed loop's 3, because it has four distinct instruction shapes
rather than three.
Every shape templatized in both.

The `data` term decomposes as (fetch/decode bookkeeping in **bold**):

| instruction | payload bytes |
|---|---|
| `ADDA` | **IS 2, OS 2, PC 2**, A 2, NZVC 4 = 12 |
| `STWA` | **IS 2, OS 2, PC 2**, mem 2 + 4 address = 12 |
| `BR` | **IS 2, OS 2, PC 2** = 6 |
| average | **6.0 bookkeeping**, 4.0 architectural state = 10.0 |


## Derived: sim3 packets

Fragment sizes, from `lib/sim3/api/traced/`:

- **variant tag** — 1 byte. zpp_bits defaults a variant's discriminant to `std::byte{Index}` when no `serialize_id`
  is declared, and none of the sim3 fragment types declare one.
- **`frame::header::Trace`** — tag(1) + `u16 length`(2) + `varint back_offset`(1) = **4 B**
- **`packet::header::Write`** — tag(1) + `varint device`(1) + `varint path`(1) + `VariableBytes` address (1 length
  byte + N address bytes). Per the note in `trace_packets.hpp`, Pep/10 registers use a 1-byte address and main
  memory 2: **5 B** for a register write, **6 B** for a memory write.
- **`payload::Variable`** — tag(1) + length(1) + N bytes: **4 B** for a 2-byte write, **6 B** for a 4-byte one.

So a 2-byte register write costs 9, a 4-byte CSR write 11, a 2-byte memory write 10.

**sim3's cost depends on the CPU's write pattern, and that pattern changed underneath it.** Two of the optimizations
below removed writes rather than shrinking encodings, and sim3 would have collected those savings too. Both
generations are therefore derived:

| | writes/instr | sim3 fixed (bytes)| sim3 walking (bytes) |
|---|---:|---:|---:|
| original CPU (4 separate CSR writes, PC written 2.33x) | 6.33 | 60.0 | 65.2 |
| current CPU (one CSR write, PC written once) | 4.00 | **41.0** | **43.5** |

The fixed loop averages 6.33 writes and 11.33 payload bytes per instruction. 
That matched the measured `data` term of 11.33 B/instr, which is what validates this model.

### sim3 cost

Fixed loop:
- `ADDA` = 5 register writes x (5+4) + 4 CSR writes x (5+3) = 45 + 32 = 77
- `STWA` = 4 x 9 + (6+4) = 46
- `BR` = 5 x 9 = 45
- average 56, plus a 4-byte frame header = **60.0 B/instr**

Walking loop: `ADDA`(77) + `STWA`(46) + `ADDX`(77) + `BR`(45) = 245 / 4 = 61.25, plus frame = **65.3 B/instr**.

Note sim3's cost barely moves between the two loops (60.0 vs 65.3). It pays per write regardless of whether anything repeats.

## Comparing the formats

| | sim3 | new, templated | new, if nothing templated |
|---|---:|---:|---:|
| fixed loop | 41.0 | **26.1** (1.57x better) | 83.3 (2.03x **worse**) |
| walking loop | 43.5 | **26.7** (1.63x better) | 87.5 (2.01x **worse**) |

sim3 also records no timestamps at all, which the new format does — so like-for-like the gap is slightly wider than
1.57x, though `ISYN` is absorbed into templates and no longer has a separable cost to subtract.

### Reusing bodies is required

Per individual write the new encoding is *more* verbose than sim3:

| | bytes |
|---|---:|
| sim3, 2-byte register write | 9 (5 header + 4 payload) |
| TVM, same write | 16 (`SETMEMX<4>` 10 + `ACCDP` 4 + 2 payload) |
This is a 1.78x increase per individual write. The new format is only smaller on average because of compression via calls.
So, we need to support compression on every common sequence if we want comparable memory usage.

### Improvements

| | sim3 | new |
|---|---|---|
| seek to instruction N | O(N) walk, cached backlinks | O(1) via location buffer |
| reverse step | O(frame) worst case | O(1) |
| memory bound | none (`InfiniteBuffer`) | ring + `acknowledge()` + watermarks |


### Regressions
| | sim3 | new |
|---|---|---|
| serializable to disk | yes (zpp_bits) | no |
| reverse address translation | working (`AddressBiMap`) | `TRADDR` unimplemented |
| MMIO / impure reads | `ImpureRead` packets | not yet encoded |
| external deps in trace core | Qt, spdlog, zpp_bits | none |

## What changed, and what each delivered

Projections were made before each change; measured values are what landed.
1. **Constant `ISYN` moved from prefix to body.** The prefix is inlined into every program and never hashed, so a
   constant tick cost its full six bytes forever. *Projected −6.0 B/instr of code; measured **−6.01**.*
2. **Location buffers allocated lazily and returned on `acknowledge()`.** *Measured **512 → 256 KiB** reserved.*
3. **`write_packed_csr` coalesced** from four 1-byte writes to one 4-byte write. Four `SETMEMX`+`ACCDP` pairs for
   four bits of state, and four trips through the recorder. *Measured **−15.3 B/instr** of inlined code.*
4. **`SETMEMDX`** — target offset carried in the payload, selected per device. *Walking loop **49.2 → 29.2**,
   pending hashes **750 → 0**.* Must not be applied unconditionally: the offset costs 4 bytes per *write*, so
   turning it on everywhere would have made the fixed loop ~60% worse.
5. **Data pointer moved into the location buffer.** The absolute `LDP` at the head of every program named a buffer
   that differed every execution, so it could never join a template. Trading 8 code bytes for 4 more location bytes
   left the code stream as pure `CALL` + `HALT`. *Measured **32.8 → 28.8**.*
6. **PC coalesced to one write per instruction.** PC moved two or three times inside one instruction — past the
   opcode, past the operand specifier, and again on a jump — and only the last value means anything to a replay.
   *Projected −2.67 B/instr of data; measured **12.67 → 10.002**, total **28.8 → 26.1**.*

## What is left

The code stream is finished; 8 B/instr of `CALL` + `HALT` is the floor for a randomly-seekable, bidirectionally
replayable record, and no further de-duplication can touch it. Remaining ideas, none yet done:
1. **Fetch/decode bookkeeping is 60% of payload.** `IS`, `OS` and `PC` are 6 of the 10 payload bytes, and all three
   are derivable from the program image plus the previous record's PC. Not recording them would take data 10 → 4 and
   the total to roughly **20 B/instr**. The cost is that replay stops
   being ISA-agnostic, and that a self-modifying program would re-derive the wrong instruction. 
2. **`CALL` is 6 bytes** — a buffer id plus an offset. An indexed reference into the template chain would fit in one
   word. −2 B/instr.
3. **The location entry is 8 bytes.** The data half is almost always in the same buffer as the previous entry's, so
   a "same buffer, small delta" encoding could reach 5–6. Fiddly, and it would complicate random access.

Two costs the harness still cannot see:

- **Throughput.** Six changes optimized bytes while *adding* per-instruction work — an FNV hash over every body on
  every commit. That trade has never been measured.
- **Realistic template counts.** These loops promote 3–4 templates; a real program has ~200 instruction shapes.
  Template lookup is now O(1), but nothing has been run at that scale.

## Reproducing

```
ctest -R "Trace footprint"
```

The harness reports through `SPDLOG_WARN`, so the numbers appear in the test log on a passing run.
