#!/bin/sh -ex
#
# combine two files into a single stream

pt-combine -s 1000 -n 2000 \
  output/merged0.root raw/run000875.root raw/run001066.root
pt-combine -s 3000 -n 2000 -u example1 \
  output/merged1.root raw/run001066.root raw/run000875.root
