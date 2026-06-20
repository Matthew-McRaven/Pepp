#!/usr/bin/env python3
"""Generate C++ test vectors for umul128h (high 64 bits of a 64x64->128 multiply)."""

# Python ints are infinite precision, so the result will automatically be correct for the 128 bit result.
# Extract the upper 64 bits with shifts / masks.
def umul128h(a, b):
    return ((a * b) >> 64) & 0xFFFFFFFFFFFFFFFF

# (lhs, rhs) inputs; results are computed.
INPUTS = [
    (0x0000000000000000, 0xFFFFFFFFFFFFFFFF),
    (0x0000000000000001, 0xFFFFFFFFFFFFFFFF),
    (0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF),
    (0x00000000FFFFFFFF, 0x00000000FFFFFFFF),
    (0x0000000100000000, 0x0000000100000000),
    (0xFFFFFFFF00000000, 0xFFFFFFFF00000000),
    (0x0000000080000000, 0x0000000080000000),
    (0x8000000000000000, 0x0000000000000002),
    (0x8000000000000000, 0x8000000000000000),
    (0xFEDCBA9876543210, 0x0123456789ABCDEF),
    (0xDEADBEEFDEADBEEF, 0xCAFEBABECAFEBABE),
    (0x1234567890ABCDEF, 0xFEDCBA0987654321),
    (0xFFFFFFFFFFFFFFFF, 0x0000000000000002),
    (0xAAAAAAAAAAAAAAAA, 0x5555555555555555),
    (0x0000000000000003, 0x5555555555555556),
    # carry propagation at the 2^32 seam
    (0xFFFFFFFF00000001, 0xFFFFFFFF00000001),
    (0xFFFFFFFFFFFFFFFF, 0xFFFFFFFF00000000),
    # asymmetric magnitudes / multiply-by-small
    (0x0000000000000003, 0xFFFFFFFFFFFFFFFF),
    (0xFFFFFFFFFFFFFFFF, 0x0000000000000003),
    # identity and zero anchors
    (0xFFFFFFFFFFFFFFFF, 0x0000000000000001),
    (0xFFFFFFFFFFFFFFFF, 0x0000000000000000),
    # single-bit results across the boundary
    (0x0000000100000000, 0x0000000200000000),
    (0x0000000200000000, 0x0000000200000000),
    # near-max both operands, off-by-one from all-ones
    (0xFFFFFFFFFFFFFFFE, 0xFFFFFFFFFFFFFFFE),
]


def main():
    n = len(INPUTS)
    for lhs, rhs in INPUTS:
        res = umul128h(lhs, rhs)
        print(f"    T{{0x{lhs:016X}ULL, 0x{rhs:016X}ULL, 0x{res:016X}ULL}},")
    # Copy the output


if __name__ == "__main__":
    main()