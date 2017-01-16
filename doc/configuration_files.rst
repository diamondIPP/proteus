Configuration Files
===================

All configuration files are using the `TOML configuration file format
<https://github.com/toml-lang/toml>`_. This is a simple ini-like file
format that is easy to read and write. It looks like this:

.. code-block:: toml
    
    [device]
    name = "FEI4Tel"
    clock = 40.0
    window = 16

To run an analysis multiple configurations files are needed that
describe the setup and the analysis. They are discussed in the
following sections. Examples can also be found in the ``test``
directory.

Device Configuration
--------------------

The telescope device configuration must contain the global
information, a set of sensor types, and the list of sensors in the
data file. This configuration is assumed to be static over the course
of one measurement campaign. As a convenience, optional paths to the
separate geometry and pixel masks configuration files can be
provided. They must occur at the head of the file before any other
configuration section.

The paths can be either absolute or relative. In the latter case they
are interpreted relative to the directory of the device configuration
file.

.. code-block:: toml

    geometry = "path/relative/to/device/config.toml"
    pixel_masks = [ "/or/an/absolute/path.toml",
                    "another.toml" ]

The pixel maks must be a list files, e.g. separate files for the
telescope and the device-under-test.

Sensor types are defined by name and configure the pixel sensor
properties required for the analysis.

.. code-block:: toml

    [sensor_types.fei4-si]
    cols = 80
    rows = 336
    pitch_col = 250.
    pitch_row = 50.
    thickness = 250.
    x_x0 = 0.005443

For each sensor only the sensor type is required. The sensor type
**must** correspond to a sensor type defined in the same device
configuration file. An optional name can be provided otherwise one is
automatically generated.

.. code-block:: toml

    [[sensors]]
    type = "fei4-si"
    name = "telescope0"

Geometry
--------

Each sensor is defined by its local coordinate system and its relative
orientation in the global coordinate system. Its is defined by the
rotation from local into global coordinates and an additional offset,
i.e. the position of the local sensor origin in the global system.

.. code-block:: toml

    [[sensors]]
    id = 3
    offset_x = 181.
    offset_y = 9290.
    offset_z = 501000.
    rotation_x = 0.
    rotation_y = 0.
    rotation_z = 1.5696899267190794

The offset is directly given by its coordinate, using the same length
units consistent with the rest of the configuration. Here, micrometer
is used. The rotation is using the 3-2-1 Euler angle convention
implemented in ``ROOT::Math::RotationZYX`` to define the rotation from
the local sensor coordinates to the globall cordinate system.

Pixel masks
-----------

Pixels can be masked with a separate configuration file. Masked pixels
are not considered for the analysis, e.g. in the clusterization. A
mask files contains a list of sensors, defined by its sensor id, and a
list of pixels, defined by their column and row address.

.. code-block:: toml

    [[sensors]]
    id = 2
    masked_pixels = [[0, 2], [23, 42]]

Analysis
--------

The analysis file configured parameters for the various analysis
steps. Each tool uses a separate parameter block, e.g. the *pt-track*
tool is configured in the *track* section.

.. code-block:: toml

    [track]
    sensor_ids = [0, 1, 2, 3, 4, 5]
    search_sigma_max = 4.0
    num_points_min = 5

It is often necessary to run the same tool with different settings,
e.g. run separate alignment steps for the telescope and the
duts. Additional subsections can be setup, e.g.

.. code-block:: toml

    [track.with_dut]
    sensor_ids = [0, 1, 2, 3, 4, 5, 6] # add the dut sensor
    search_sigma_max = 5.0
    num_points_min = 5

By adding the ``-u with_dut`` the tool uses the given subsection
instead of the default one.

.. warning::

    The default section and additional subsections are independent,
    i.e. values set in the default section do not propagate to the
    subsections.
