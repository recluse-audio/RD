#!/usr/bin/env python3
"""Parse plugin name info from CMakeLists.txt for use by signing/release scripts."""

from __future__ import annotations

import re
from pathlib import Path


def get_plugin_info(root: Path) -> dict[str, str]:
    """
    Parse CMakeLists.txt to extract plugin naming info.

    Returns dict with:
        target:       CMake target name (e.g. "Flanger2") — used for artefacts directory
        product_name: PRODUCT_NAME (e.g. "Flanger 2") — used for binary filenames
    """
    cmake_file = root / "CMakeLists.txt"
    text = cmake_file.read_text()

    # Collect set(VAR "value") definitions so we can resolve ${VAR} references.
    vars_map: dict[str, str] = {}
    for vm in re.finditer(r'set\(\s*(\w+)\s+"([^"]*)"\s*\)', text):
        vars_map[vm.group(1)] = vm.group(2)

    def resolve(s: str) -> str:
        prev = None
        while prev != s:
            prev = s
            s = re.sub(r'\$\{(\w+)\}', lambda m: vars_map.get(m.group(1), m.group(0)), s)
        return s

    # Extract CMake target from: project(<Name> VERSION ...)
    m = re.search(r'project\(\s*(\S+)', text)
    target = resolve(m.group(1)) if m else root.name

    # Extract PRODUCT_NAME from: PRODUCT_NAME "..."
    m = re.search(r'PRODUCT_NAME\s+"([^"]+)"', text)
    product_name = resolve(m.group(1)) if m else target

    return {"target": target, "product_name": product_name}
