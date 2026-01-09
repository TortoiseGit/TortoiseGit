#!/usr/bin/env python
#
# Copyright (C) 2026 the TortoiseGit team
# This file is distributed under the same license as TortoiseGit
#
# Author: Sven Strickroth <email@cs-ware.de>, 2026
#

from __future__ import annotations

import argparse
import os
import platform
import shutil
import subprocess
import sys
import tempfile
import textwrap
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple

# -----------------------------
# Utilities
# -----------------------------

def is_windows() -> bool:
    return os.name == "nt"


def run(
    args: List[str],
    cwd: Optional[Path] = None,
    *,
    check: bool = True,
    capture: bool = False,
    debug: bool = False,
    input: String = None,
) -> subprocess.CompletedProcess:
    if debug:
        print(f"Executing: {args} in cwd {cwd}\n")
    return subprocess.run(args, cwd=str(cwd) if cwd else None, check=check, capture_output=capture, text=True, input=input, env=None)


def delete(p: Path) -> None:
    if p.is_dir():
        shutil.rmtree(p, ignore_errors=True)
    else:
        try:
            p.unlink()
        except FileNotFoundError:
            pass


def replace_tokens(data: str, mapping: Dict[str, str], begintoken: str, endtoken: str) -> str:
    # NAnt replacetokens: tokens appear as begintoken + KEY + endtoken
    for k, v in mapping.items():
        data = data.replace(f"{begintoken}{k}{endtoken}", v)
    return data


def list_xml_files_for_spellcheck(root: Path, app: str) -> List[Path]:
    files: List[Path] = []
    app_root = root / "source" / "en" / app
    for xmlfile in app_root.rglob("*.xml"):
        # exclude git_doc/*.xml when app is TortoiseGit
        if app == "TortoiseGit":
            try:
                rel = xmlfile.relative_to(root)
            except ValueError:
                rel = xmlfile
            if str(rel).replace("\\", "/").startswith("source/en/TortoiseGit/git_doc/"):
                continue
        files.append(xmlfile)

    for extra in ["glossary.xml", "wishlist.xml"]:
        files.append(root / "source" / "en" / extra)

    # stable ordering
    return sorted(set(files))


# -----------------------------
# Config
# -----------------------------

@dataclass
class Config:
    # Can be overriden in doc.build.user
    debug: bool = False
    applications: str = "TortoiseGit,TortoiseMerge"

    path_bin: str = "/usr/bin"
    path_spellcheck: str = "aspell"

def load_user_properties(doc_build_user: Path) -> Dict[str, str]:
    """
    Reads doc.build.user like:
      <project ...>
        <property name="applications" value="TortoiseGit,TortoiseMerge" />
        ...
      </project>
    Returns name->value.
    """
    if not doc_build_user.exists():
        return {}

    # It's regular XML, so ET is fine.
    tree = ET.parse(str(doc_build_user))
    root = tree.getroot()
    props: Dict[str, str] = {}
    for prop in root.findall(".//property"):
        name = prop.attrib.get("name")
        value = prop.attrib.get("value")
        if name and value is not None:
            props[name] = value
    return props


def apply_overrides(cfg: Config, overrides: Dict[str, str]) -> Config:
    # Only apply keys we understand; ignore others.
    mapping = {
        "applications": "applications",
        "path.bin": "path_bin",
        "path.spellcheck": "path_spellcheck",
    }
    for k, v in overrides.items():
        attr = mapping.get(k)
        if attr is None:
            continue
        if hasattr(cfg, attr):
            setattr(cfg, attr, v)
    return cfg

def spellcheck(root: Path, *, cfg: Config, app: str) -> None:
    print(f"Spellchecking: '{app} en' This may take a few minutes")

    spellcheck_log = root / f"{app}_en.log"

    with tempfile.TemporaryDirectory() as tmp, spellcheck_log.open("w", encoding="utf-8") as dst:
        # Copy TortoiseGit.tmpl.pws -> Temp.pws with $LANG$ set
        tmpl = root / "TortoiseGit.tmpl.pws"
        temp_pws = Path(tmp) / "Temp.pws"
        data = tmpl.read_text(encoding="utf-8")
        data = replace_tokens(data, {"LANG": "en"}, begintoken="$", endtoken="$")
        temp_pws.write_text(data, encoding="utf-8")

        files = list_xml_files_for_spellcheck(root.parent, app)

        for file_target in files:
            print(f"Checking: {file_target.name}")

            xsltproc = run(
                [Path(cfg.path_bin) / "xsltpoc", "--nonet" , "removetags.xsl", file_target],
                cwd=root,
                check=True,
                capture=True,
                debug=cfg.debug,
            )

            aspell = run(
                [cfg.path_spellcheck, "--mode=sgml", "--encoding=utf-8", f"--add-extra-dicts=./en.pws", f"--add-extra-dicts={temp_pws}", "--lang=en", "list", "check"],
                cwd=root,
                check=True,
                capture=True,
                input=xsltproc.stdout,
                debug=cfg.debug,
            )

            # Append file_log to overall app log
            if aspell.stdout:
                relpath = os.path.relpath(file_target, root.parent / "source")
                dst.write(f"---{relpath}\n")
                dst.write(aspell.stdout)

# -----------------------------
# CLI
# -----------------------------

def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(
        prog="doc_build.py",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=textwrap.dedent("""\
            Documentation Builder.

            Examples:
              python3 doc_build.py
              python3 doc_build.py --applications TortoiseGit
            """)
    )
    parser.add_argument("--applications", help="Comma-separated apps (e.g. TortoiseGit,TortoiseMerge)")
    parser.add_argument("--debug", help="Show debug output")
    args = parser.parse_args(argv)

    root = Path(__file__).resolve().parent

    cfg = Config()

    # Load doc.build.user overrides (if present)
    user_props = load_user_properties(root.parent / "doc.build.user")
    cfg = apply_overrides(cfg, user_props)

    # Apply CLI overrides
    if args.applications:
        cfg.applications = args.applications

    spellerrors = False

    # Runs for all apps in config
    for app in [a.strip() for a in cfg.applications.split(",") if a.strip()]:
        spellerrors |= spellcheck(root, app=app, cfg=cfg)

    return 0 if !spellerrors else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
