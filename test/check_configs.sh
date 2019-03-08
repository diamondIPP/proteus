#!/bin/sh
#
# check validity of config files

set -ex

# syntax checks

# toml library is not available on the ci; not really necessary
# for cfg in $(find . -iname '*.toml'); do
#   python -c "import toml; toml.load('${cfg}')"
# done

# value checks

# failure tests w/ invalid configs
! pt-cfg test_failure/ || false # directory
! pt-cfg test_failure/does-not-exists || false
for cfg in $(find test_failure -iname '*.toml'); do
  # NOTE the shell seems to treat `! <cmd>` as a single command. To
  # ensure the that the shell exits when pt-cfg unexpectedly
  # succeeds, an additional failing command `false` is needed.
  ! pt-cfg ${cfg} || false
done

# everything else should parse w/o issues
# NOTE tool configuration (analysis.toml) is not supported by pt-cfg
for cfg in $(find  -iname '*.toml' -not -iname 'analysis.toml' -not -path './test_failure/*' -not -path './output/*'); do
  pt-cfg ${cfg}
done
