#!/bin/sh
#
# check validity of config files

set -ex # print commands and exit upon error

source build/activate.sh

# pt-cfg should succeed
pt-cfg run000875/device.toml
pt-cfg run000875/geometry/unaligned.toml
pt-cfg run001066/device.toml
pt-cfg run001066/geometry/unaligned.toml
pt-cfg run001066/masks/mask_duts_empty.toml
pt-cfg run001066/masks/mask_tel_empty.toml

# pt-cfg should fail
# NOTE the shell seems to treat `! <cmd>` as a single command. To ensure the
# that the shell exits when pt-cfg unexpectedly succeeds, an additional
# failing command `false` is needed.
! pt-cfg run000875/ || false # directory
! pt-cfg run000875/does-not-exists || false
