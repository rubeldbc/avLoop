# Security policy

## Supported versions

Security fixes are made on the latest release and on `main`.

## Reporting a vulnerability

Please **do not open a public issue** for a security problem. Report it privately through GitHub's
*Report a vulnerability* button on the repository's **Security** tab (private vulnerability reporting). Include what
you found, how to reproduce it (a sample file if a crafted video triggers it) and the AvLoop version.

You will get an answer within a week. Once a fix is released, the report is credited unless you ask otherwise.

## Scope

AvLoop opens video files from anywhere, so a crafted file that crashes it, hangs it or makes it write outside the
folder you chose is in scope. Problems inside FFmpeg or Qt themselves are reported to those projects; AvLoop then
updates its pinned versions.
