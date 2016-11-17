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

The telescope device consist of a list of sensors, a set of sensor
types, and additional global information. This configuration is
assumed to be static over the course of one measurement campaign. As a
convenience, optional paths to the separate alignment and pixel mask
configuration files can be provided. They must occur at the head of
the file before any other configuration section.

The paths can be either absolute or relative. In the latter case they
are interpreted relative to the directory of the device configuration
file.

.. code-block:: toml

    alignment = "path/relative/to/device/config.toml"
    noise_mask = "/or/an/absolute/path.toml"

The noise mask can also be set from multiple files, e.g. separate
files for the telescope and the device-under-test, by providing a list
of files:

.. code-block:: toml

    noise_mask = ["file1.toml", "file2.toml"]

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

Alignment and Pixel Masks
-------------------------

.. todo:: Describe alignment and pixel mask configuration file

Analysis
--------

.. todo:: Describe analysis configuration file
