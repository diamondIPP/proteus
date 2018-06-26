#!/bin/sh
#
# check toml syntax on all config files

set -ex

for path in $(find . -iname '*.toml'); do
    python -c "import toml; toml.load('${path}')"
done
