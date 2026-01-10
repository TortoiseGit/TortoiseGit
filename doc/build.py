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
) -> subprocess.CompletedProcess:
    if debug:
        print(f"Executing: {args} in cwd {cwd}\n")
    return subprocess.run(args, cwd=str(cwd) if cwd else None, check=check, capture_output=capture, text=True, env=None)


def delete(p: Path) -> None:
    if p.is_dir():
        shutil.rmtree(p, ignore_errors=True)
    else:
        try:
            p.unlink()
        except FileNotFoundError:
            pass


def copy_file(src: Path, dst: Path, *, overwrite: bool = True) -> None:
    if dst.exists() and not overwrite:
        return
    shutil.copy2(src, dst)


def replace_tokens(data: str, mapping: Dict[str, str], begintoken: str, endtoken: str) -> str:
    # NAnt replacetokens: tokens appear as begintoken + KEY + endtoken
    for k, v in mapping.items():
        data = data.replace(f"{begintoken}{k}{endtoken}", v)
    return data


def file_uptodate(target: Path, sources: Iterable[Path]) -> bool:
    if not target.exists():
        return False
    t_mtime = target.stat().st_mtime
    for s in sources:
        if s.exists() and s.stat().st_mtime > t_mtime:
            return False
    return True


# -----------------------------
# Config
# -----------------------------

@dataclass
class Config:
    # Can be overriden in doc.build.user
    applications: str = "TortoiseGit,TortoiseMerge"
    docformats: str = "html"  # "pdf,html"
    help_mapping: str = "1"   # "1" or "0"
    external_gitdocs: str = "0"  # "1" to use external git docs
    debug: bool = False

    path_bin: str = str(Path("../Tools").resolve())
    path_fop: str = str(Path("../Tools/fop").resolve())
    name_fop: str = "fop.bat"
    path_xsl: str = str(Path("../Tools/xsl").resolve()).replace("\\", "/")
    name_python: str = "python3"

    path_user_xsl: str = "./xsl"
    path_user_css: str = "./source"

    xsl_pdf_params: str = "--nonet --xinclude --stringparam gitdoc.external ${external.gitdocs}"
    xsl_pdf_file: str = "pdfdoc.xsl"
    xsl_htmlchunk_params: str = (
        "--nonet --xinclude --stringparam chunker.output.encoding UTF-8 "
        "--stringparam html.stylesheet styles_html.css --stringparam use.id.as.filename 1 "
        "--stringparam gitdoc.external ${external.gitdocs} --stringparam generate.help.mapping ${help.mapping}"
    )
    xsl_htmlchunk_file: str = "htmlchunk.xsl"

    # Derived at runtime:
    docverstring: str = ""  # Major.Minor.Micro

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
        "debug": "debug",
        "applications": "applications",
        "docformats": "docformats",
        "help.mapping": "help_mapping",
        "external.gitdocs": "external_gitdocs",
        "path.bin": "path_bin",
        "path.fop": "path_fop",
        "name.fop": "name_fop",
        "path.xsl": "path_xsl",
        "name.python": "name_python",
        "path.user.xsl": "path_user_xsl",
        "path.user.css": "path_user_css",
        "xsl.pdf.params": "xsl_pdf_params",
        "xsl.pdf.file": "xsl_pdf_file",
        "xsl.htmlchunk.params": "xsl_htmlchunk_params",
        "xsl.htmlchunk.file": "xsl_htmlchunk_file",
    }
    for k, v in overrides.items():
        attr = mapping.get(k)
        if attr is None:
            continue
        if hasattr(cfg, attr):
            setattr(cfg, attr, v)
    return cfg


def prepare_version_info(cfg: Config) -> None:
    # If env vars are not set, set defaults.
    if "MajorVersion" not in os.environ:
        os.environ["MajorVersion"] = "2"
        os.environ["MinorVersion"] = "18"
        os.environ["MicroVersion"] = "0"
    cfg.docverstring = f"{os.environ.get('MajorVersion','')}.{os.environ.get('MinorVersion','')}.{os.environ.get('MicroVersion','')}"


# -----------------------------
# Build logic (targets)
# -----------------------------

class DocBuilder:
    def __init__(self, root: Path, cfg: Config, cleanup: bool = False):
        self.root = root
        self.cfg = cfg
        self.cleanup = cleanup

    def _resolve_fontpath(self) -> Path:
        if not is_windows():
            fp = Path("/usr/share/fonts/truetype/")
        else:
            windir = os.environ.get("windir") or os.environ.get("WINDIR")
            if not windir:
                raise RuntimeError("Cannot infer Windows font directory because %WINDIR% is not set.")
            fp = Path(windir.replace("\\", "/")) / "Fonts"
        if not fp.exists():
            raise RuntimeError(f"fontpath '{fp}' does not exist")
        return fp

    def clean(self) -> None:
        delete(self.root / "output")

        delete(self.root / "source" / "en" / "version.xml")
        delete(self.root / "source" / "en" / "TortoiseGit" / "git_doc" / "git-doc.xml")

        user_xsl_dir = self.root / self.cfg.path_user_xsl
        for file in user_xsl_dir.glob("db_*.xsl"):
            delete(file)
        delete(user_xsl_dir / "en" / "userconfig.xml")

    def prepare_custom(self) -> None:
        # write helper stylesheets that import docbook ones.
        xsl_dir = self.root / self.cfg.path_user_xsl
        (xsl_dir / "db_pdfdoc.xsl").write_text(
            "\n".join([
                '<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">',
                f'  <xsl:import href="{self.cfg.path_xsl}/fo/docbook.xsl"/>',
                "</xsl:stylesheet>",
                "",
            ])
        )
        (xsl_dir / "db_htmlchunk.xsl").write_text(
            "\n".join([
                '<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">',
                f'  <xsl:import href="{self.cfg.path_xsl}/html/chunk.xsl"/>',
                "</xsl:stylesheet>",
                "",
            ])
        )

    def prepare(self) -> None:
        if self.cleanup:
            self.clean()
        (self.root / "output").mkdir(exist_ok=True)

        self.prepare_custom()

    def update_version_info(self) -> None:
        mapping = {
            "MajorVersion": os.environ.get("MajorVersion", ""),
            "MinorVersion": os.environ.get("MinorVersion", ""),
            "MicroVersion": os.environ.get("MicroVersion", ""),
        }
        data = (self.root / "source" / "en" / "Version.in").read_text(encoding="utf-8")
        data = replace_tokens(data, mapping, begintoken="$", endtoken="$")
        (self.root / "source" / "en" / "version.xml").write_text(data, encoding="utf-8")

    def _expand_params(self, s: str) -> str:
        # minimal ${...} expansion for the known placeholders used in include
        # (NAnt expressions are much richer; we only need a few).
        ret = (s.replace("${external.gitdocs}", self.cfg.external_gitdocs)
                .replace("${help.mapping}", self.cfg.help_mapping))
        if "${" in ret:
            raise RuntimeError(f'unresolved ${...} parameter in "{s}".')
        return ret

    def uptodate(self, target: Path, app: str) -> bool:
        sources = [
            *list((self.root / "source" / "en" / app).rglob("*.xml")),
            self.root / "source" / "en" / "glossary.xml",
            self.root / "source" / "en" / "wishlist.xml",
            self.root / "source" / "en" / "version.in",
        ]
        return file_uptodate(target, sources)

    def xsltproc(self, *, xslt_source: Path, xslt_file: str, xslt_params: str, xslt_target: Path) -> None:
        # Select language-specific stylesheet if present.
        xsl_dir = self.root / self.cfg.path_user_xsl
        lang_candidate = xsl_dir / "en" / xslt_file
        if lang_candidate.exists():
            stylesheet = lang_candidate
        else:
            stylesheet = xsl_dir / xslt_file

        # Run xsltproc
        params = self._expand_params(xslt_params)
        cmd = [
            Path(self.cfg.path_bin) / "xsltproc",
            *params.split(),
            "--output",
            str(xslt_target),
            str(stylesheet),
            str(xslt_source),
        ]

        # capture stdout/stderr for checks
        proc = run(cmd, cwd=self.root, capture=True, check=False)
        out = (proc.stdout or "") + "\n" + (proc.stderr or "")
        print(out)

        # Check for errors in XML files
        fatal_markers = [
            ": parser error :",
            "Error: no ID for constraint linkend:",
            "ERROR: xref linking to ",
            ", but no template matches.",
        ]
        for m in fatal_markers:
            if m in out:
                raise RuntimeError(f'xsltproc output contains fatal marker "{m}".')

        if proc.returncode != 0:
            raise RuntimeError(f"xsltproc failed with exit code {proc.returncode}.")

    def copyimages(self, *, app: str, doc_target_work: Path, xslt_source: Path) -> None:
        images_dir = doc_target_work / "images"
        delete(images_dir)
        images_dir.mkdir()

        # Run images/extract-images.xsl to get the file list.
        params = self._expand_params(self.cfg.xsl_pdf_params)
        cmd = [
            Path(self.cfg.path_bin) / "xsltproc",
            *params.split(),
            "images/extract-images.xsl",
            str(xslt_source),
        ]
        proc = run(cmd, cwd=self.root, check=True, capture=True)
        # xslt output is a list of filenames (one per line)
        lines = [ln.strip() for ln in proc.stdout.splitlines() if ln.strip()]
        for filename in lines:
            copy_file(self.root / "images" / "en" / filename, images_dir / filename)

        # Copy fixed UI icons
        fixed = ["caution.png", "warning.png", "important.png", "tip.png", "note.png", "link.png"]
        for f in fixed:
            copy_file(self.root / "images" / "en" / f, images_dir / f)

        # Copy callouts svg (when app==TortoiseGit and external gitdocs disabled)
        if app == "TortoiseGit" and self.cfg.external_gitdocs == "0":
            callouts_dir = images_dir / "callouts"
            callouts_dir.mkdir(parents=True, exist_ok=True)
            for svg in (self.root / "images" / "en" / "callouts").glob("*.svg"):
                copy_file(svg, callouts_dir / svg.name)

    def html(self, *, app: str, doc_target_name: str, doc_target_work: Path, xslt_source: Path) -> None:
        if not self.uptodate(doc_target_work / "index.html", app):
            # copy styles_html.css into output workdir
            copy_file(self.root / self.cfg.path_user_css / "styles_html.css", doc_target_work / "styles_html.css", overwrite=True)

            self.xsltproc(
                xslt_source=xslt_source,
                xslt_file=self.cfg.xsl_htmlchunk_file,
                xslt_params=self.cfg.xsl_htmlchunk_params,
                xslt_target=doc_target_work / doc_target_name,  # xsltproc will create chunked html files
            )

        if self.cfg.help_mapping == "1":
            self.helpmapping(app=app, doc_target_work=doc_target_work)

            help_wixfilelist = Path("../src/TortoiseGitSetup/HTMLHelpfiles.wxi")
            cmd = [self.cfg.name_python, "scripts/generate_wix_filelist.py"]
            proc = run(cmd, cwd=self.root, capture=True, check=True)
            (self.root / help_wixfilelist).write_text(proc.stdout or "", encoding="utf-8")

            if not is_windows():
                run(["sed", "-i", r"s/$/\r/", str(self.root / help_wixfilelist)], cwd=self.root, check=True)

            print(f"Updated help WiX filelist: '{self.root / help_wixfilelist}'")

    def pdf(self, *, app: str, doc_target_name: str, doc_target_work: Path, xslt_source: Path) -> None:
        target = self.root / "output" / f"{app}-{self.cfg.docverstring}-en.pdf"
        if self.uptodate(target, app):
            return

        xslt_final = doc_target_work / doc_target_name
        tmp_out = Path(str(xslt_final) + ".tmp")

        self.xsltproc(
            xslt_source=xslt_source,
            xslt_file=self.cfg.xsl_pdf_file,
            xslt_params=self.cfg.xsl_pdf_params,
            xslt_target=tmp_out,
        )

        # Move tmp -> .fo while removing span="inherit" and replacing image sizing attrs
        fo_file = Path(str(xslt_final) + ".fo")
        fo_data = tmp_out.read_text(encoding="utf-8")
        fo_data = fo_data.replace('span="inherit" ', "")
        fo_data = fo_data.replace(
            'width="auto" height="auto" content-width="auto" content-height="auto"',
            'inline-progression-dimension.maximum="100%" content-width="scale-down-to-fit" content-height="scale-down-to-fit"',
        )
        fo_file.write_text(fo_data, encoding="utf-8")

        # Optional userconfig.xml generation with %FONTSDIR% token
        userconfig_template = self.root / self.cfg.path_user_xsl / "en" / "userconfig.template.xml"
        userconfig = self.root / self.cfg.path_user_xsl / "en" / "userconfig.xml"
        if userconfig_template.exists():
            cfg_data = userconfig_template.read_text(encoding="utf-8")
            cfg_data = replace_tokens(cfg_data, {"FONTSDIR": str(self._resolve_fontpath()).replace("\\", "/")}, begintoken="%", endtoken="%")
            userconfig.write_text(cfg_data, encoding="utf-8")

        # Run FOP
        cmdline = ["-q", "-fo", str(fo_file), "-pdf", str(target)]
        if userconfig.exists():
            cmdline = ["-c", str(userconfig), *cmdline]

        run(
            [Path(self.cfg.path_fop) / self.cfg.name_fop, *cmdline],
            cwd=self.root,
            check=True,
            capture=False,
        )

    def helpmapping(self, *, app: str, doc_target_work: Path) -> None:
        if app == "TortoiseGit":
            help_resource = self.root / "../src/TortoiseProc/resource.h"
            help_mappingfile = self.root / "../src/Resources/TGitHelpMapping.ini"
        elif app == "TortoiseMerge":
            help_resource = self.root / "../src/TortoiseMerge/resource.h"
            help_mappingfile = self.root / "../src/Resources/TGitMergeHelpMapping.ini"
        else:
            raise RuntimeError(f"Unknown app for help.prepare: {app}")

        alias_h = doc_target_work / "alias.h"

        cmd = [
            self.cfg.name_python,
            "scripts/create_html_mapping.py",
            str(help_resource),
            str(alias_h),
        ]
        proc = run(cmd, cwd=self.root, capture=True, check=True)
        help_mappingfile.write_text(proc.stdout or "", encoding="utf-8")

        if not is_windows():
            run(["sed", "-i", r"s/$/\r/", str(help_mappingfile)], cwd=self.root, check=True)

        print(f"Updated help mapping: '{help_mappingfile}'")

    def all(self) -> None:
        self.prepare()

        apps = [a.strip() for a in self.cfg.applications.split(",") if a.strip()]
        formats = [f.strip() for f in self.cfg.docformats.split(",") if f.strip()]

        for app in apps:
            doc_target_name = f"{app}_en"
            doc_target_work = self.root / "output" / doc_target_name
            doc_target_work.mkdir(exist_ok=True)

            xslt_source = self.root / "source" / "en" / app / f"{app}.xml"

            print("-" * 60)
            print(f"Creating '{doc_target_name}' documentation")

            # Special handling for git_doc.xml
            if app == "TortoiseGit":
                git_doc_xml = self.root / "source" / "en" / "TortoiseGit" / "git_doc" / "git-doc.xml"
                delete(git_doc_xml)
                if self.cfg.external_gitdocs == "0":
                    copy_file(self.root / "source" / "en" / "TortoiseGit" / "git_doc" / "git-doc.xml.in", git_doc_xml, overwrite=True)

            # update version info in version.xml
            self.update_version_info()

            # Build each format
            for docformat in formats:
                self.copyimages(app=app, doc_target_work=doc_target_work, xslt_source=xslt_source)
                if docformat == "html":
                    self.html(app=app, doc_target_name=doc_target_name, doc_target_work=doc_target_work, xslt_source=xslt_source)
                elif docformat == "pdf":
                    self.pdf(app=app, doc_target_name=doc_target_name, doc_target_work=doc_target_work, xslt_source=xslt_source)
                else:
                    raise RuntimeError(f"Unknown docformat '{docformat}' (expected html or pdf)")


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
              python3 doc_build.py --applications TortoiseGit --docformats html
              python3 doc_build.py --target clean
            """)
    )
    parser.add_argument("--target", default="all", help="Target to run (all, clean)")
    parser.add_argument("--cleanup", choices=["yes", "no"], default="no", help="If 'yes', clean deletes output/")
    parser.add_argument("--applications", help="Comma-separated apps (e.g. TortoiseGit,TortoiseMerge)")
    parser.add_argument("--docformats", help="Comma-separated formats (html,pdf)")
    parser.add_argument("--external-gitdocs", choices=["0", "1"], help="0=use local git docs, 1=use external git docs")
    parser.add_argument("--help-mapping", choices=["0", "1"], help="0=disable mapping/wix generation, 1=enable")
    parser.add_argument("--debug", action="store_true", help="Show debug output")
    args = parser.parse_args(argv)

    root = Path(__file__).resolve().parent

    cfg = Config()

    # Load doc.build.user overrides (if present)
    user_props = load_user_properties(root / "doc.build.user")
    cfg = apply_overrides(cfg, user_props)

    # Apply CLI overrides
    if args.applications:
        cfg.applications = args.applications
    if args.docformats:
        cfg.docformats = args.docformats
    if args.external_gitdocs is not None:
        cfg.external_gitdocs = args.external_gitdocs
    if args.help_mapping is not None:
        cfg.help_mapping = args.help_mapping
    if args.debug:
        cfg.debug = args.debug

    # Prepare version info early so docverstring is always available
    prepare_version_info(cfg)

    b = DocBuilder(root=root, cfg=cfg, cleanup=(args.cleanup == "yes"))

    t = args.target
    if t == "all":
        b.all()
    elif t == "clean":
        b.clean()
    else:
        raise SystemExit(f"Unknown target '{t}'")

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
