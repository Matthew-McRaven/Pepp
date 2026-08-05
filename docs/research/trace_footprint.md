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

## Measured: new system

```
fixed-address store  : 33.5 B/instr over 3000 instrs (inlined: 112.0) | ratio 3.344
                       code 66236  templates 260  data 34004 | 3 templates, 0 pending | 512 KiB reserved
walking-address store: 49.2 B/instr over 3000 instrs (inlined: 122.0) | ratio 2.480
                       code 111292 templates 316  data 36004 | 3 templates, 750 pending | 512 KiB reserved
```

### Where the 33.5 bytes go (fixed loop)

`code` is 66,236 / 3000 = **22.08 B/instr**, which is exactly the non-shareable scaffolding:

| component | B/instr | share | shareable? |
|---|---:|---:|---|
| data payload | 11.3 | 34% | no |
| `LDP` (DP anchor) | 8.0 | 24% | no — names a per-program buffer id + offset |
| `ISYN` (constant tick) | 6.0 | 18% | not today; it is a constant, so it could be |
| `CALL` into template | 6.0 | 18% | — |
| `HALT` | 2.0 | 6% | no |
| templates, amortized | 0.09 | 0.3% | — |

Bodies averaged **84.7 B/instr** and collapsed to a 6-byte `CALL`, with all three templates costing 260 bytes total.
That is a **14x reduction on the shareable portion**, and it is effectively complete: there is no body left to
squeeze. The overall ratio is "only" 3.34x because everything else is fixed overhead.

### The walking loop isolates the cost of an address in the body

3000 instructions = 750 iterations x 4. The 2250 register-only instructions template normally (22 B each = 49,500),
so the 750 stores account for 111,292 − 49,500 = **61,792 B, or 82.4 B each** — versus 22 B if their bodies could be
shared. An unshareable store costs **3.75x** a shareable one, purely because `SetMem<true,4>` carries the target
address in the packet.

The 750 pending hashes are the second half of that cost: one hash retained per body that will never recur.

## Derived: sim3 packets

Fragment sizes, from `lib/sim3/api/traced/`:

- **variant tag** — 1 byte. zpp_bits defaults a variant's discriminant to `std::byte{Index}` when no `serialize_id`
  is declared, and none of the sim3 fragment types declare one.
- **`frame::header::Trace`** — tag(1) + `u16 length`(2) + `varint back_offset`(1, since frames here are < 128 B) = **4 B**
- **`packet::header::Write`** — tag(1) + `varint device`(1) + `varint path`(1) + `VariableBytes` address (1 length
  byte + N address bytes). Per the note in `trace_packets.hpp`, Pep/10 registers use a 1-byte address and main
  memory 2, giving **5 B** for a register write and **6 B** for a memory write.
- **`payload::Variable`** — tag(1) + length(1) + N payload bytes = **3 B** for a 1-byte write, **4 B** for a 2-byte write.

### Write counts per instruction

| instruction | writes | payload bytes |
|---|---:|---:|
| `ADDA` | 9 — PC(2), IS(2), PC(2), OS(2), A(2), N(1), Z(1), V(1), C(1) | 14 |
| `STWA` | 5 — PC(2), IS(2), PC(2), OS(2), mem(2) | 10 |
| `BR` | 5 — PC(2), IS(2), PC(2), OS(2), PC(2) | 10 |

The fixed loop averages 6.33 writes and 11.33 payload bytes per instruction. **That matches the measured `data` term of 11.33 B/instr exactly**.

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
| fixed loop | 60.0 | **33.5** (1.79x better) | 112.0 (1.87x **worse**) |
| walking loop | 65.3 | **49.2** (1.33x better) | 122.0 (1.87x **worse**) |

Like-for-like on the fixed loop, sim3 records no timestamps at all; excluding the 6 B `ISYN` puts the new format at
27.5 B/instr, or **2.2x better**.

### Reusing bodies is reequired

Per individual write the new encoding is *more* verbose than sim3:

| | bytes |
|---|---:|
| sim3, 2-byte register write | 9 (5 header + 4 payload) |
| TVM, same write | 16 (`SETMEMX<4>` 10 + `ACCDP` 4 + 2 payload) |
This is a 1.78x increase per individual write. The new format is only smaller on average because of compression via calls.
So, we need to support compression on every common sequence if we want comparble memory usage.

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
| reverse address translation | working (`AddressBiMap`, `ModifiedAddressSink`) | `TRADDR` unimplemented |
| MMIO / impure reads | `ImpureRead` packets | not yet encoded |
| external deps in trace core | Qt, spdlog, zpp_bits | none |

## Reproducing

```
ctest -R "Trace footprint"
```

The harness reports through `SPDLOG_WARN`, so the numbers appear in the test log on a passing run.
