#!/usr/bin/env python3
"""Compare TestCFHD -d output from two architectures.

The question this answers: does the NEON path in Codec/simd_compat.h produce the
same bitstream as the SSE2/MMX path it replaces? A local answer is not available,
because a Mac runs the x86 build under Rosetta -- which does its own SSE-to-NEON
translation, so neither side would be native. Two runners settle it.

Sizes are compared exactly. PSNR is compared with a tolerance, because it is
printed to one decimal and a boundary case can round either way.
"""
import re, sys

LINE = re.compile(r"\s*(\d+): source (\d+) compressed to (\d+).*PSNR ([\d.]+)dB")
PSNR_TOLERANCE_DB = 0.2


def parse(path):
    rows = []
    for line in open(path, errors="ignore"):
        m = LINE.match(line)
        if m:
            rows.append((int(m.group(1)), int(m.group(3)), float(m.group(4))))
    return rows


def main(a_path, b_path, a_name, b_name):
    a, b = parse(a_path), parse(b_path)
    if not a or not b:
        print(f"no vectors parsed ({a_name}: {len(a)}, {b_name}: {len(b)})")
        return 1
    if len(a) != len(b):
        print(f"frame count differs: {a_name} {len(a)}, {b_name} {len(b)}")
        return 1

    print(f"{'#':>4} {a_name:>12} {b_name:>12} {'delta':>7} {'ppm':>8}  {'psnr A':>7} {'psnr B':>7}")
    identical = 0
    worst_bytes = 0
    psnr_failures = []
    for (fa, sa, pa), (fb, sb, pb) in zip(a, b):
        d = sa - sb
        ppm = abs(d) / sb * 1e6 if sb else 0
        if d == 0:
            identical += 1
        worst_bytes = max(worst_bytes, abs(d))
        flag = ""
        if abs(pa - pb) > PSNR_TOLERANCE_DB:
            psnr_failures.append((fa, pa, pb))
            flag = "  <-- PSNR"
        print(f"{fa:>4} {sa:>12} {sb:>12} {d:>+7} {ppm:>8.1f}  {pa:>7.1f} {pb:>7.1f}{flag}")

    print()
    print(f"frames compared    : {len(a)}")
    print(f"byte-identical     : {identical}/{len(a)}")
    print(f"largest size delta : {worst_bytes} bytes")

    # Bitstreams differing is a recorded fact, not a failure: the shim is a port,
    # not a bit-exact reimplementation. Quality diverging IS a failure, because
    # that means a shim is wrong rather than merely rounding elsewhere.
    if psnr_failures:
        print()
        print(f"FAIL: {len(psnr_failures)} frame(s) differ by more than {PSNR_TOLERANCE_DB} dB")
        for f, pa, pb in psnr_failures:
            print(f"  frame {f}: {a_name} {pa} dB vs {b_name} {pb} dB")
        return 1

    if identical == len(a):
        print("\nBit-identical across architectures.")
    else:
        print(f"\nNot bit-identical. Quality equivalent within {PSNR_TOLERANCE_DB} dB.")
        print("Anything content-addressed must not mix architectures.")
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 5:
        print("usage: compare_vectors.py A.txt B.txt A_NAME B_NAME")
        sys.exit(2)
    sys.exit(main(*sys.argv[1:]))
