#!/bin/sh
#
# check validity of config files

set -ex

echo "=== using $(which pt-cfg)"

# pt-cfg should succeed for regular configs
pt-cfg device.toml
pt-cfg geometry/unigetel_ebeam012_nparticles01.toml
pt-cfg geometry/unigetel_ebeam012_nparticles01-telescope.toml
pt-cfg geometry/unigetel_ebeam012_nparticles01-telescope_first_last.toml
pt-cfg geometry/unigetel_ebeam120_nparticles01.toml
pt-cfg geometry/unigetel_ebeam120_nparticles01-telescope.toml
pt-cfg geometry/unigetel_ebeam120_nparticles01-telescope_first_last.toml
pt-cfg geometry/unigetel_ebeam120_nparticles01_xygamma.toml
pt-cfg geometry/unigetel_ebeam120_nparticles01_xygamma-telescope.toml
pt-cfg geometry/unigetel_ebeam120_nparticles01_xygamma-telescope_first_last.toml
pt-cfg masks/telescope_empty.toml
pt-cfg masks/dut_empty.toml

# pt-cfg should fail
# NOTE the shell seems to treat `! <cmd>` as a single command. To ensure the
# that the shell exits when pt-cfg unexpectedly succeeds, an additional
# failing command `false` is needed.
! pt-cfg devices/test_failure-missing_settings.toml || false
! pt-cfg devices/test_failure-non_exclusive_regions.toml || false

! pt-cfg geometry/ || false # directory
! pt-cfg geometry/does-not-exists || false

! pt-cfg geometry/test_failure-missing_id.toml || false
! pt-cfg geometry/test_failure-non_orthogonal.toml || false
pt-cfg geometry/test_success-with_angles.toml
pt-cfg geometry/test_success-with_directions.toml
