#!/usr/bin/env python3
"""Build the VST3 target."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

PLUGIN_NAME = "RD"


def run(cmd: list[str], cwd: Path) -> None:
    print("+", " ".join(cmd))
    subprocess.run(cmd, cwd=str(cwd), check=True)


def regenerate_cmake_lists() -> None:
    regen_script = Path(__file__).parent / "regenSource.py"
    if regen_script.exists():
        print("Regenerating CMake file lists...")
        subprocess.run([sys.executable, str(regen_script)], check=True)
    else:
        print("Warning: regenSource.py not found, skipping regeneration")


def main() -> int:
    regenerate_cmake_lists()

    build_dir = Path("BUILD").resolve()
    build_dir.mkdir(exist_ok=True)

    run(["cmake", ".."], cwd=build_dir)

    build_cmd = ["cmake", "--build", ".", "--target", f"{PLUGIN_NAME}_VST3"]
    if sys.platform.startswith("win"):
        build_cmd += ["--config", "Debug"]

    run(build_cmd, cwd=build_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
