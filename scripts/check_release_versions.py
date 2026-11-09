#!/usr/bin/env python3
"""Require a CLI version bump whenever a PR increases the GUI version."""
import argparse
import json
import pathlib
import re
import subprocess


def version(value):
    if not isinstance(value, str) or not re.fullmatch(r"\d+\.\d+\.\d+", value):
        raise ValueError(f"expected a major.minor.patch version, got {value!r}")
    return tuple(int(part) for part in value.split("."))


def cli_version(contents):
    parts = []
    for part in ("MAJOR", "MINOR", "PATCH"):
        match = re.search(r'^#define\s+OFPROJECTGENERATOR_' + part + r'_VERSION\s+"(\d+)"\s*$', contents, re.MULTILINE)
        if not match:
            raise ValueError(f"missing command-line {part} version")
        parts.append(int(match[1]))
    return tuple(parts)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("base", help="PR base commit or branch, for example upstream/master")
    args = parser.parse_args()
    root = pathlib.Path(subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip())

    def previous(file):
        return subprocess.check_output(["git", "show", f"{args.base}:{file}"], cwd=root, text=True)

    package = json.loads((root / "frontend/package.json").read_text())
    lock = json.loads((root / "frontend/npm-shrinkwrap.json").read_text())
    gui = version(package["version"])
    cli = cli_version((root / "commandLine/src/defines.h").read_text())
    old_gui = version(json.loads(previous("frontend/package.json"))["version"])
    old_cli = cli_version(previous("commandLine/src/defines.h"))

    if lock["version"] != package["version"] or lock["packages"][""]["version"] != package["version"]:
        raise ValueError("both npm-shrinkwrap.json root versions must match frontend/package.json")
    if gui < old_gui or cli < old_cli:
        raise ValueError("GUI and command-line versions must not decrease relative to the base")
    if gui > old_gui and cli <= old_cli:
        raise ValueError("a GUI version bump also requires a command-line version bump in commandLine/src/defines.h")

    print(f"Release versions OK: GUI {'.'.join(map(str, gui))}, CLI {'.'.join(map(str, cli))}")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, KeyError, subprocess.CalledProcessError) as error:
        raise SystemExit(f"Release version check failed: {error}")
