#!/usr/bin/env python3
"""Validate the PS5 low-address map without a PS5 SDK or console."""

from __future__ import annotations

import pathlib
import re


ROOT = pathlib.Path(__file__).resolve().parents[1]
HEADER = ROOT / "include" / "bt4_ps5_memory_layout.h"
REGION = re.compile(
    r'X\("(?P<name>[^"]+)",\s*0x(?P<base>[0-9A-Fa-f]+)u,\s*'
    r'0x(?P<size>[0-9A-Fa-f]+)u\)'
)


def main() -> int:
    regions = [
        (match["name"], int(match["base"], 16), int(match["size"], 16))
        for match in REGION.finditer(HEADER.read_text(encoding="utf-8"))
    ]
    if not regions:
        raise SystemExit("no regions found in bt4_ps5_memory_layout.h")

    previous_end = 0
    for name, base, size in regions:
        assert base % 0x1000 == 0, f"{name}: base is not page-aligned"
        assert size > 0 and size % 0x1000 == 0, f"{name}: invalid size"
        assert base >= previous_end, f"{name}: overlaps the previous region"
        assert base + size <= 0x1_0000_0000, f"{name}: exceeds the 32-bit window"
        previous_end = base + size

    names = {name for name, _, _ in regions}
    expected = {
        "game heap",
        "game stack",
        "hardware regs",
        "GS regs",
        "host low arena",
        "game image",
        "scratchpad",
    }
    assert names == expected, f"region set changed: {sorted(names ^ expected)}"

    total = sum(size for _, _, size in regions)
    print(f"PS5 memory layout: {len(regions)} non-overlapping regions, "
          f"{total / (1024 * 1024):.1f} MiB of virtual address space")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
