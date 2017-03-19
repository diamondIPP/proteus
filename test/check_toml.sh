#!/bin/sh -ex
#
# check toml syntax

for path in $(find . -iname '*.toml'); do
    python -c "import toml; toml.load('${path}')"
done
