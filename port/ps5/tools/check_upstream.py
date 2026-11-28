#!/usr/bin/env python3
"""Validate the Xbox UWP source revision recorded by the PS5 port."""

from __future__ import annotations

import json
import pathlib
import re


PS5_ROOT = pathlib.Path(__file__).resolve().parents[1]
REPOSITORY_ROOT = PS5_ROOT.parents[1]
MANIFEST = PS5_ROOT / "UPSTREAM_UWP.json"
BT4_VERSION = REPOSITORY_ROOT / "port" / "release" / "BT4_VERSION.txt"


def main() -> int:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    required = {
        "repository",
        "branch",
        "commit",
        "xbox_release",
        "project_version",
        "revision",
        "synced_on",
        "ps5_milestone",
    }
    missing = required.difference(manifest)
    if missing:
        raise SystemExit(f"UPSTREAM_UWP.json is missing: {sorted(missing)}")

    commit = manifest["commit"]
    if not isinstance(commit, str) or not re.fullmatch(r"[0-9a-f]{40}", commit):
        raise SystemExit("UPSTREAM_UWP.json has an invalid commit SHA")

    expected_release = (
        f"{manifest['project_version']} R{manifest['revision']}"
    )
    if manifest["xbox_release"] != expected_release:
        raise SystemExit(
            f"Xbox release mismatch: {manifest['xbox_release']!r} != "
            f"{expected_release!r}"
        )

    version_line = BT4_VERSION.read_text(encoding="utf-8").splitlines()[0]
    if expected_release not in version_line:
        raise SystemExit(
            f"port/release/BT4_VERSION.txt does not contain {expected_release!r}"
        )

    print(
        "PS5 upstream: Xbox UWP "
        f"{expected_release} at {commit[:8]} ({manifest['synced_on']})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
