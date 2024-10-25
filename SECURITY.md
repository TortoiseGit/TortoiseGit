# Security Policy

## Reporting a vulnerability

Vulnerabilities in TortoiseGit can be reported using:

- a [GitLab issue](https://gitlab.com/tortoisegit/tortoisegit/-/issues/new) that is marked as confidential
- an mail to `email (at) cs-ware (dot) de` (PGP key fingerprint: [29BC C23B 1C15 355C 1D26 DBAB 165A CCB5 FD51 5839](https://www.cs-ware.de/sven-FD515839.asc))

Vulnerabilities are expected to be discussed _only_ using these two methods, and not in public, until the official announcement on the release date.

Examples for details to include:

- Ideally a short description (or a script) to demonstrate an exploit.
- The affected scenarios.
- The name and affiliation of the security researchers who are involved in the discovery, if any.
- Whether the vulnerability has already been disclosed.
- How long an embargo would be required to be safe.

We prefer all communication to be in English or German.

## Supported Versions

TortoiseGit only supports the most recent stable release. There are no official "Long Term Support" versions for TortoiseGit.

Based on the vulnerability, we decide how to distribute the fix, e.g. as a separate patch or as a new stable release containing either only the patch or also other fixes.

## Preview versions

TortoiseGit also provides preview releases (these are not stable releases) of the current development as per TortoiseGit's `master` branch at the [previews](https://download.tortoisegit.org/tgit/previews/) page on an irregular basis.

We ensure that people who run a preview release are also automatically notified for fixed versions using our automatic updater.

Note: in other projects' nomenclature these may be referred to as "nightly builds"
