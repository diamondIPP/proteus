Configuration files
===================

All configuration files are using the `TOML configuration file
format <https://github.com/toml-lang/toml>`__. It is a simple ini-like
file format, easy to read and write. To run an analysis multiple
configurations files are needed that describe the setup and the
analysis. They are discussed in the following sections. Examples can
also be found in the ``test`` directory.

Basic introduction to TOML
--------------------------

TOML stores parameters in the format ``key = value`` where value can be a
number, a string or an array (of numbers, strings or arrays). Parameters can be
grouped into tables, which are defined by ``[table_name]``. Tables are similar
to objects in the `JSON <http://json.org>`_ notation.

.. code::
    
    geometry = "geometry/aligned.toml"
    pixel_mask = ["masks/tel_mask_empty.toml",
                  "masks/duts_mask_empty.toml"]

    [example]
    keyname = 0.123

Here we see two global parameters that don't belong to any table (``geometry``
and ``noise_mask``) and then the table ``[example]`` with its parameters.

You can also define sub-tables.

.. code::

    [sensor_types.fei4-si]
    measurement = "pixel_tot"
    cols = 80
    rows = 336
    pitch_col = 250.
    pitch_row = 50.
    thickness = 250.
    x_x0 = 0.005443

    [sensor_types.ccpdv4]
    measurement = "ccpdv4_binary"
    cols = 160
    rows = 168
    pitch_col = 125.
    pitch_row = 100.
    thickness = 250.
    x_x0 = 0.005443

Here ``fei4-si`` and ``ccpdv4`` are both subtables of ``[sensor_types]``.

You can define arrays of tables too. Again, these would correspond to
lists of objects in `JSON <http://json.org>`_ notation.

.. code::

    [[noisescan.tel.sensors]]
    id = 0
    col_min = 33
    col_max = 48
    row_min = 146
    row_max = 188

    [[noisescan.tel.sensors]]
    id = 1
    col_min = 36
    col_max = 45
    row_min = 144
    row_max = 221

    [[noisescan.tel.sensors]]
    id = 2
    col_min = 34
    col_max = 50
    row_min = 138
    row_max = 181

This corresponds to an array of three tables available under the
``noisescan.tel.sensor`` key. The tables in the array are added in
the order in which they are written in the file.

Device configuration (``device.toml``)
--------------------------------------

This configuration file describes the setup, the sensors
types used and lists all the sensors used for the measurements (both the
telescope ones and the DUTs). Furthermore, it can contain the path to an
geometry configuration files (which contains the positions of the
sensors) and to the noise masks, i.e. a list of the noisy pixels for
each sensor.

The paths in this file can be either absolute or relative path. Relative paths
are interpreted with respect to the directory containing the file.

The device file is assumed to remain static over a measurement campaign.

Global settings
~~~~~~~~~~~~~~~

The global settings (i.e. geometry and noise mask) must be at the
beginning of the file, before any ``[section]`` so the first rows will
look like this:

.. code::

    # global settings **must** appear before any [section] command

    geometry = "geometry/aligned.toml"
    pixel_masks = ["masks/tel_mask_empty.toml",
                   "masks/duts_mask_empty.toml"]

Both ``geometry`` and ``pixel_masks`` are optional. If ``geometry`` is not
defined in the device file it must be supplied on the command line.
``pixel_masks`` must be a list of paths, but can be empty.

Sensor types
~~~~~~~~~~~~

Then there are the definitions of all the possible sensor types used in
the setup. Each sensor type must be a sub-table of ``[sensors_types]``,
i.e. its name must be something like
``[sensor_types.name-of-your-sensor-type]``.

.. code::

    [sensor_types.fei4-si]
    measurement = "pixel_tot"
    cols = 80
    rows = 336
    pitch_col = 250.
    pitch_row = 50.
    thickness = 250.
    x_x0 = 0.005443

    [sensor_types.ccpdv4]
    measurement = "ccpdv4_binary"
    cols = 160
    rows = 168
    pitch_col = 125.
    pitch_row = 100.
    thickness = 250.
    x_x0 = 0.005443

Here we have defined two sensor types, one for the telescope planes,
``[sensor_types.fei4-si]`` and one for the DUTs,
``[sensor_types.ccpdv4]``: in our case we have FE-I4 based sensors for
the telescope planes and the DUTs will be CCPDv4 sensors.

For each sensor type you define the number of columns and rows, rows and
columns pitch, thickness of the sensor (both in the chosen units and in
radiation lengths).

The ``measurement`` option tells proteus how the physical pixel are
mapped to the digital pixel of the front end and if it should consider
time-over-threshold (tot) information or binary hits.
It can have 3 different values:

-  ``pixel_tot`` if physical and digital pixels are mapped one-to-one
   and you consider TOT information
-  ``pixel_binary`` same mapping, but with binary information
-  ``ccpdv4_binary`` mapping for the CCPDv4 chip, binary information

Sensors
~~~~~~~

After having declared the sensors types, you have to list all the
sensors used in your setup as an array of tables called ``[[sensors]]``.
You have to declare, for each sensor, its type (**must** be one of the
ones listed before in the **same** configuration file) and its name
(optional. If you don't write it, it will be generated automatically).
In our case ``type`` can be ``fei4-si`` or ``ccpdv4``.

**The order is important: it must be the same of the data file and the
index in the list will correspond to the sensor id in other
configuration files.** The ids begin with 0.

.. code::

    [[sensors]]
    type = "fei4-si"
    name = "tel0"

    [[sensors]]
    type = "fei4-si"
    name = "tel1"

    [[sensors]]
    type = "fei4-si"
    name = "tel2"

    [[sensors]]
    type = "fei4-si"
    name = "tel3"

    [[sensors]]
    type = "fei4-si"
    name = "tel4"

    [[sensors]]
    type = "fei4-si"
    name = "tel5"

    [[sensors]]
    type = "ccpdv4"
    name = "caribou04"

    [[sensors]]
    type = "ccpdv4"
    name = "caribou06"

Geometry (``geometry.toml``)
----------------------------

This file contains the description of the setup geometry, i.e. the
positions and rotations of each sensor and the beam parameters. The
length units must be consistent with the other configuration files, the
angle units are radians.

Beam parameters
~~~~~~~~~~~~~~~

The beam is described by its mean slope along the z-axis, the divergence/
standard deviation for the slope, and its energy.

.. code::

    [beam]
    slope = [0.0012, -0.002]
    divergence = [0.002, 0.0025]
    energy = 180.0

Sensor geometry
~~~~~~~~~~~~~~~

This array of tables contains the id, position, and orientation of every
sensor. The planar surface of each geometry is defined by two unit vectors
that point along the two internal axes of the sensor, i.e. the direction along
increasing columns and rows, and the offset of its origin. All three vectors
are defined in the global reference frame. This is a right-handed coordinate
system with z along the beam and y pointing towards the sky and x pointing
horizontally.

The origin can be placed anywhere and for convenience it is usually
placed in the origin of the first sensor of the telescope.

The units must be consistent with values in other coordinate systems, but are
otherwise undefined. Only the direction of the unit vectors is taken into
account the length of the unit vectors is ignored.

.. code::

    [[sensors]]
    id = 0
    offset = [0.0, 0.0, 0.0]
    unit_u = [1.0, 0.0, 0.0]
    unit_v = [0.0, 1.0, 0.0]

    [[sensors]]
    id = 1
    offset = [-100.0, 250.0, -158000]
    unit_u = [0.0, -1.0, 0.0]
    unit_v = [-1.0, 0.0, 0.0]

Pixel masks
-----------

Pixels can be masked with a separate configuration file. Masked pixels
are not considered for the analysis, e.g. in the clusterization. A mask
file contains a list of sensors, defined by its sensor id, and a list of
pixels, defined by their column and row address.

.. code::

    [[sensors]]
    id = 2
    masked_pixels = [[0, 2], [23, 42]]

    [[sensors]]
    id = 4
    masked_pixels = [[5, 23], [2, 6]]

Analysis (``analysis.toml``)
----------------------------

In this configuration file you have to define which sensors are the
telescope, which ones are the DUTs, the parameters for the alignment and
some other stuff. It has no global settings. Notice that the name of the
configuration is the same as the name of the related command (e.g.
``[track]`` is the configuration of ``pt-track``, and so on).

Each section can be written in a separate file and when calling the
corresponding command, you have to give the ``-c file_path`` option.

Each section can be splitted in subsections (e.g.
``[track.subsection]``) and to select one of them you have to use the
``-u subsection_name`` option.

.. warning::
    The default section and additional subsections are independent,
    i.e. values set in the default section do not propagate to the
    subsections.

[track]
~~~~~~~

The ``[track]`` table tells proteus which sensors must be used to
reconstruct tracks, so here you have to write the ids of the telescope
planes plus a few parameters used in the reconstructions.

.. code::

    [track]
    # sensors that are used to build the tracks, i.e. the telescope ones
    sensor_ids = [0, 1, 2, 3, 4, 5]
    # distance cut to assign clusters to the track
    search_sigma_max = 4.0
    # minimum number of points of the track
    num_points_min = 5
    # [reduced chi2 of what?]
    reduced_chi2_max = -1. # the value -1 disables chi2 cut; same as removing the line altogether

[match]
~~~~~~~

Here you just have to write the sensor ids of the DUTs, i.e. the ones
which will have to match the tracks

.. code::

    [match]
    sensor_ids = [6, 7]

[align]
~~~~~~~

There are usually at least 4 ``[align]`` sub-tables: two for the
telescope planes and two for the DUTs. There are two of them for each
sensor because you usually run a coarse alignment before, and a fine one
later. In each sub-table you specify the methot to be used, the sensors
used for tracking and the ones that will be aligned, plus other
parameters depending on the chosen method.

.. code::

    # coarse alignment of only the telescope planes using cluster correlations
    [align.tel_coarse]
    method = "correlations" # use method based on cluster correlations (more coarse)
    sensor_ids = [0, 1, 2, 3, 4, 5] #these are the sensors to be considered
    align_ids = [1, 2, 3, 4, 5] #this are the sensors to be aligned. The first one is considered already aligned and the remaining will be aligned wrt it.

    # fine alignment of only the telescope planes using track residuals
    [align.tel_fine]
    method = "residuals" # use method based on track residuals, iterative
    sensor_ids = [0, 1, 2, 3, 4, 5] # sensor to use for tracking
    align_ids = [1, 2, 3, 4, 5] # sensor for which alignment is calculated
    num_steps = 20 # number of iterative steps
    search_sigma_max = 5.0 # distance cut for track finding
    reduced_chi2_max = 10.0 # chi2 cut for track finding
    damping = 0.8 # scale correction steps to avoid oscillations in iteration

    # coarse alignment of the duts keeping the telescope planes fixed
    [align.dut_coarse]
    method = "correlations"
    sensor_ids = [0, 1, 2, 3, 4, 5, 6, 7]
    align_ids = [6, 7]

    # fine alignment of the duts keeping the telescope planes fixed
    [align.dut_fine]
    method = "residuals"
    sensor_ids = [0, 1, 2, 3, 4, 5, 6, 7]
    align_ids = [6, 7]
    num_steps = 20
    search_sigma_max = 10.0
    reduced_chi2_max = 10.0
    damping = 0.9

[noisescan]
~~~~~~~~~~~

This section defines the parameters for the noise scan, i.e. the
parameters used by proteus to determine which pixels must be considered
noisy.

You can define subgroups (e.g. if you want to give different parameters
for the telescope and for the DUT) and then, for each sensor, you can
define the region on which the noise scan must be run.

You can run a noisescan just on a subgroup using the ``-u`` option.

.. code::

    #noise scan parameters for the telescope.
    [noisescan.tel]
    sigma_above_avg_max = 5.0 #cut that defines how many sigma above avg a pixel must have to be considered noisy
    rate_max = 0.1 #[DEFINITION]
    density_bandwidth = 2.0 # Parameter to calculate the expected number of hits in each pixel

    #for each sensor, the noise scan will be run in the region defined by col_min, col_max, row_min and row_max [ARE THEY MANDATORY?]
    [[noisescan.tel.sensors]]
    id = 0
    col_min = 33
    col_max = 48
    row_min = 146
    row_max = 188

    [[noisescan.tel.sensors]]
    id = 1
    col_min = 36
    col_max = 45
    row_min = 144
    row_max = 221

    [[noisescan.tel.sensors]]
    id = 2
    col_min = 34
    col_max = 50
    row_min = 138
    row_max = 181

    [[noisescan.tel.sensors]]
    id = 3
    col_min = 33
    col_max = 49
    row_min = 142
    row_max = 185

    [[noisescan.tel.sensors]]
    id = 4
    col_min = 34
    col_max = 42
    row_min = 130
    row_max = 212

    [[noisescan.tel.sensors]]
    id = 5
    col_min = 30
    col_max = 46
    row_min = 155
    row_max = 198

    #noise scan parameters for the DUT
    [noisescan.dut0]
    sigma_above_avg_max =  5.0
    rate_max = 0.1
    density_bandwidth = 3.0

    [[noisescan.dut0.sensors]]
    id = 6
    col_min = 0
    col_max = 6
    row_min = 157
    row_max = 169
