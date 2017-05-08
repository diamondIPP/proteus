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

Using TOML you can save parameters in the format ``key = value`` where
value can be a number, a string or an array (of numbers, strings or
arrays).

The parameters can be grouped into tables, which are defined by
``[table_name]``.

.. code-block:: toml

    geometry = "geometry/aligned.toml"
    pixel_mask = ["masks/tel_mask_empty.toml",
                  "masks/duts_mask_empty.toml"]

    [device]
    name = "FEI4Tel"
    clock = 40.0
    window = 16
    space_unit = "um"
    time_unit = "ns"

Here we see two parameters that don't belong to any table (``geometry``
and ``noise_mask``) and then the table ``[device]`` with its parameters.

You can also define sub-tables.

.. code-block:: toml

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

Here ``fei4-si`` and ``ccpdv4`` are both subtables of
``[sensor_types]``.

You can define arrays of tables too.

.. code-block:: toml

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

Here we see an array of subtables. The tables in the array are added in
the order in which they are written in the file.

Device configuration (``device.toml``)
--------------------------------------

The telescope device configuration must contain the global information,
a set of sensor types, and the list of sensors in the data file. This
configuration is assumed to be static over the course of one measurement
campaign. As a convenience, optional paths to the separate geometry and
pixel masks configuration files can be provided. They must occur at the
head of the file before any other configuration section.

The paths can be either absolute or relative. In the latter case they
are interpreted relative to the directory of the device configuration
file.

Global settings
~~~~~~~~~~~~~~~

The global settings (i.e. geometry and noise mask) must be at the
beginning of the file, before any ``[section]`` so the first rows will
look like this:

.. code-block:: toml

    # global settings **must** appear before any [section] command

    geometry = "geometry/aligned.toml"

    # only to demonstrate the setting.
    # leaving out the `noise_mask` setting altogether has the same effect
    # as adding empty masks.
    noise_mask = ["masks/tel_mask_empty.toml",
                  "masks/duts_mask_empty.toml"]

while ``geometry`` is mandatory, one can omit ``pixel_masks``: in this
case it will be considered empty. The ``pixel_masks`` must be an array
of paths.

Device settings
~~~~~~~~~~~~~~~

After the global settings, there is the definition of some parameters of
the telescope:

.. code-block:: toml

    # common global device settings
    [device]
    name = "FEI4Tel"
    clock = 40.0
    window = 16
    space_unit = "um"
    time_unit = "ns"

Here you define the name of your telescope, the clock speed is used to
convert from timestamp int real time, windows is the integration time
measured in timestamps. The space and time units are used just to create
histogram labels. The units are not defined anywhere, they just have to
be consistent.

Sensor types
~~~~~~~~~~~~

Then there are the definitions of all the possible sensor types used in
the setup. Each sensor type must be a sub-table of ``[sensors_types]``,
i.e. its name must be something like
``[sensor_types.name-of-your-sensor-type]``.

.. code-block:: toml

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
TOT (time over threshold) information or just binary hits (hit/not-hit).
It can have 3 different values:

-  ``pixel_tot`` if physical and digital pixels are mapped one-to-one
   and you consider TOT information

-  ``pixel_binary``\ same mapping, but with binary information

-  ``ccpdv4_binary``\ mapping for the CCPDv4 chip, binary information

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

.. code-block:: toml

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

This file contains the description of the telescope setup, i.e. the
positions and rotations of each sensor and the slopes of the beam. The
length units must be consistent with the other configuration files, the
angle units are radians.

[beam]
~~~~~~

It just describes the x and y slope of the beam.

.. code-block:: toml

    [beam]
    slope_x = 2.2589004909162290e-05
    slope_y = -2.3725615037855144e-07

[[sensors]]
~~~~~~~~~~~

This array of tables contains the id, position and rotation of every
sensor. The position is wrt a global reference frame: z is along the
beam, y points towards the sky and x points right, looking into the beam
(I don't suggest to look into the beam, though).

The origin can be placed anywhere and for convenience it is usually
placed in the origin of the first sensor of the telescope.

The rotations are wrt the **local** coordinates of the sensor, and are
applied in the order z, y and x. This is the 3-2-1 Euler angle
convention implemented in ``ROOT::Math::RotationZYX`` .

.. code-block:: toml

    [[sensors]]
    id = 0
    offset_x = 0.0000000000000000
    offset_y = 0.0000000000000000
    offset_z = 0.0000000000000000
    rotation_x = 0.0000000000000000
    rotation_y = 0.0000000000000000
    rotation_z = 3.1415926535896999

    [[sensors]]
    id = 1
    offset_x = 576.96988923689821
    offset_y = -118.80586318697972
    offset_z = 158000.00000000000
    rotation_x = 0.0000000000000000
    rotation_y = 0.0000000000000000
    rotation_z = 1.5710308251919820

    [[sensors]]
    id = 2
    offset_x = 390.14416778667271
    offset_y = -364.16817448303863
    offset_z = 208000.00000000000
    rotation_x = 0.0000000000000000
    rotation_y = 0.0000000000000000
    rotation_z = 3.1372384758096055

    [[sensors]]
    id = 3
    offset_x = -367.12785459733891
    offset_y = -142.55276460807866
    offset_z = 678000.00000000000
    rotation_x = 3.1415926535896999
    rotation_y = 0.0000000000000000
    rotation_z = 0.0016241692564387122

    [[sensors]]
    id = 4
    offset_x = -301.30437112478540
    offset_y = 464.12176059680814
    offset_z = 728000.00000000000
    rotation_x = 3.1415926535897931
    rotation_y = 9.3258734068513149e-14
    rotation_z = -1.5685967823000699

    [[sensors]]
    id = 5
    offset_x = 246.40469635452345
    offset_y = 524.26153344411364
    offset_z = 862000.00000000000
    rotation_x = 3.1415926535896999
    rotation_y = 0.0000000000000000
    rotation_z = 0.0033826323299548378

    [[sensors]]
    id = 6
    offset_x = 181.15032324936138
    offset_y = 9290.4575599981417
    offset_z = 501000.00000000000
    rotation_x = 0.0000000000000000
    rotation_y = 0.0000000000000000
    rotation_z = 1.5696899267190794

    [[sensors]]
    id = 7
    offset_x = 639.46594585813284
    offset_y = 9305.3693033210529
    offset_z = 518000.00000000000
    rotation_x = 0.0000000000000000
    rotation_y = 0.0000000000000000
    rotation_z = 1.5940703309149813

Pixel masks
-----------

Pixels can be masked with a separate configuration file. Masked pixels
are not considered for the analysis, e.g. in the clusterization. A mask
file contains a list of sensors, defined by its sensor id, and a list of
pixels, defined by their column and row address.

.. code-block:: toml

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
some other stuff. It has no global settings. Each proteus tool uses a
separate block (e.g. ``[track]`` is the configuration of ``pt-track``,
and so on).

Each section can be written in a separate file and when calling the
corresponding command, you have to give the ``-c file_path`` option.

Eache section can be splitted in subsections (e.g.
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

.. code-block:: toml

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

.. code-block:: toml

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

.. code-block:: toml

    # coarse alignment of only the telescope planes using cluster correlations
    [align.tel_coarse]
    method = "correlations" # use method based on cluster correlations
    sensor_ids = [0, 1, 2, 3, 4, 5] #these are the sensors to be considered
    align_ids = [1, 2, 3, 4, 5] #this are the sensors to be aligned. The first one is considered already aligned and the remaining will be aligned wrt it.

    # fine alignment of only the telescope planes using track residuals
    [align.tel_fine]
    method = "residuals" # use method based on track residuals
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

.. code-block:: toml

    #noise scan parameters for the telescope. 
    [noisescan.tel]
    sigma_above_avg_max = 5.0 
    rate_max = 0.1
    density_bandwidth = 2.0

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