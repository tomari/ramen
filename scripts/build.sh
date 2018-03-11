#!/bin/bash -e
SRC_PATH="`dirname $0`/.."
cd "$SRC_PATH"
palmdev-prep sdk-5r3
make -j4

