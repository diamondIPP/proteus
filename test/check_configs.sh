#!/bin/sh -ex
#
# check validity of config files

echo "using $(which pt-cfg)"

# pt-cfg should succeed
pt-cfg run000875/device.toml
pt-cfg run000875/geometry/tel_aligned.toml
pt-cfg run000875/geometry/unaligned.toml
pt-cfg run001066/device.toml
pt-cfg run001066/geometry/aligned.toml
pt-cfg run001066/geometry/tel_aligned.toml
pt-cfg run001066/geometry/unaligned.toml
pt-cfg run001066/masks/duts_mask_empty.toml
pt-cfg run001066/masks/tel_mask_empty.toml

# pt-cfg should fail
# NOTE the shell seems to treat `! <cmd>` as a single command. To ensure the
# that the shell exits when pt-cfg unexpectedly succeeds, an additional
# failing command `false` is needed.
! pt-cfg run000875/ || false # directory
! pt-cfg run000875/does-not-exists || false

! pt-cfg geometry/missing_id.toml || false
! pt-cfg geometry/non_orthogonal.toml || false
pt-cfg geometry/with_angles.toml
pt-cfg geometry/with_directions.toml
