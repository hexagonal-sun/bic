#!/bin/bash

RUN_FOLDER=$(dirname $0)
SUPP_FILE="${RUN_FOLDER}"/default.supp

valgrind --leak-check=full --show-leak-kinds=definite --suppressions="${SUPP_FILE}" $@
