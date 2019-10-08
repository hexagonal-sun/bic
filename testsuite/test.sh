#!/bin/bash

set -xue

SRCFILE="${srcdir}/${1}.c";

BIC="../src/bic"

if [ ! -f "${SRCFILE}" ]; then
    echo "Error: Source file not found";
    exit 99;
fi

if [ ! -x "$BIC" ]; then
    echo "Error: could not find the BIC interpreter"
    exit 99;
fi

COMPILED_OUTPUT=$(mktemp)
BIC_OUTPUT=$(mktemp)

"${1}" > "${COMPILED_OUTPUT}";
"${BIC}" "${SRCFILE}" > "${BIC_OUTPUT}";

if ! diff "${COMPILED_OUTPUT}" "${BIC_OUTPUT}"; then
    echo "Error: BIC and compiled output differ.";
    exit 1;
fi

rm "${COMPILED_OUTPUT}" "${BIC_OUTPUT}"

exit 0
