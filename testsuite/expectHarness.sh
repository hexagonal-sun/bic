#!/usr/bin/env bash
set -ux
SRCDIR="$1"
EXPSCRIPT="$2"

expect "$EXPSCRIPT" "$SRCDIR"
