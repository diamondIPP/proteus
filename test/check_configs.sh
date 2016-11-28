#!/bin/sh -e
#
# check validity of config files

source build/activate.sh

# this should succeed
pt-cfg run000875/device.toml
pt-cfg run000875/geometry/unaligned.toml
pt-cfg run001066/device.toml
pt-cfg run001066/geometry/unaligned.toml
pt-cfg run001066/masks/mask_duts_empty.toml
pt-cfg run001066/masks/mask_tel_empty.toml

# this should fail
pt-cfg run000875/ || true # directory
pt-cfg run000875/does-not-exists || true
