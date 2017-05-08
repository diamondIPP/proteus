Commands
========

pt-noisescan
------------

This command performs the noise scan for the telescope sensors and DUTs.
It is run in this way:

.. code:: sh

    pt-noisescan [OPTIONS] input_file output_prefix

``input_file`` is the ROOT file with the raw data

``output_prefix`` is the prefix for the generated files, which are:

-  ``output_prefix-hists.root``
-  ``output_prefix-mask.toml``

These files will be described later.

Options
~~~~~~~

The possible options for this command are:

``-d, --device``: the device configuration file (default:
``device.toml``). This defines the sensor types, the geometry file path
and possibly the noise masks.

``-g, --geometry``: loads a separate geometry file, otherwise uses the
one defined in the device configuration file.

``-c, --config``: the analysis configuration file (default:
analysis.toml).

``-u, --subsection``: a subsection of the analysis configuration file
(e.g. it can be used to run the noisemask only on the telescope planes
or only on the DUT)

``-s, --skip_events``: skip the first n events (default: 0)

``-n, --num_events``: number of events to process (default: -1, i.e. all
of them).

Output files
~~~~~~~~~~~~

pt-noisescan generates two output files, ``output_prefix-hists.root``
and ``output_prefix-mask.toml``. The first one contains the histograms
generated during the noise scan, the second one contains the mask (i.e.
a list of the coordinates of the noisy pixels) that will be used in the
following steps of the analysis.

The ROOT files contains two folders: NoiseScan and Occupancy. The first
folder contains, for each sensor, different histograms:

-  Occupancy: number of hits per pixel per event (i.e. the total number
   of hits per pixel divided by the number of events)
-  OccupancyDist: distribution of the occupancy
-  DensityEstimate: estimation of the expected occupancy [not divided by
   the number of events], calculated through kernel density estimate
   with variable bandwidth (selectable in the analysis configuration
   file). The density estimation of a certain pixel is calculated
   without using the occupancy of that pixel, just the neighbors.
-  LocalSignificance: it is an estimation of how much the occupancy of
   each pixel is far from the expected value, obtained with the kernel
   density estimate: (occupancy - density estimate) / density estimate
   error
-  LocalSignificancePixels: distribution of the local significance
-  MaskedPixels: a 2D histogram with the masked pixels. Each bin
   corresponds to a pixel and contains 0 if the pixel is not masked, 1
   if it is masked.

The second folder contains, for each sensor:

-  HitMap: number of hits per pixel in that run (like occupancy, you
   don't divide by the total number of events)
-  ClusteredHitMap: empty, because the noise scan doesn't calculate the
   clusters (it would be pixel that have been clustered)
-  ClusterMap: empty (it would be the plot with the position of all the
   clusters)
-  OccupancyDist: as above

pt-align
--------

This command is used to align all the planes of the telescope and the
DUT. This is usually run twice, the firt time for the coarse alignment,
the second time for the fine alignment. The coarse and fine alignment
are defined in a configuration file. This command is run in this way:

``pt-align [OPTIONS] input_file output_prefix``

``input_file`` is the ROOT file with the raw data

``output_prefix`` is the prefix for the generated files, which are:

-  output\_prefix-hists.root
-  output\_prefix-geo.toml

These files will be described later

Options
~~~~~~~

The possible options for this command are:

``-d, --device``: the device configuration file (default:
``device.toml``). This defines the sensor types, the geometry file path
and possibly the noise masks.

``-g, --geometry``: loads a separate geometry file, otherwise uses the
one defined in the device configuration file [IS IT MANDATORY TO PUT THE
GEOMETRY FILE PATH IN THE DEV CONF FILE?]

``-c, --config``: the analysis configuration file (default:
analysis.toml).

``-u, --subsection``: a subsection of the analysis configuration file
(e.g. it can be used to run the alignment only on the telescope planes
or only on the DUT)

``-s, --skip_events``: skip the first n events (default: 0)

``-n, --num_events``: number of events to process (default: -1, i.e. all
of them). Usually you want to align on about 20k events, which then
won't be used in the tracking.

Output files
~~~~~~~~~~~~

pt-align generates two output files, **output\_prefix-hists.root** and
**output\_prefix-geo.toml**. The first one contains the histograms
generated during the alignment, the second one contains the new geometry
of the telescope.

The content of the ROOT file depends on the type of algorithm used for
the alignment.

correlations (a.k.a. coarse alignment)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Correlations: X and Y correlations between clusters position on
different sensors (specified in the name). The 2D histograms shows the
correlation between the position of two cluster on different planes in
the same event. If there is more than one cluster per event, all the
possible combination are added to the histogram. Most of the points
should be around the "y=x" line.

Diff: X and Y differences between clusters position on different sensors
(specified in the name). It should be something like a gaussian with fat
tails centered in 0.

CorrectionOffset and CorrectionRotation: the correction to the position
and rotations for each step. It is interesting when you have more than
one step (so for the fine alignment).

residuals (a.k.a. fine alignment)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There is a folder for each step of the alignment. Each folder contains 4
subfolders.

-  Trackinfo: contains the information of all the reconstructed tracks
-  Distribution of the number of cluster used for the reconstruction.
-  Distribution of the reduced chi^2.
-  Offset and slope of each track wrt the global coordinates system.
-  Residuals:
-  1D histograms with the residuals (cluster position - track position)
   along u and v for each plane.
-  2D histograms with the residuals vs the position and the slope of the
   tracks for each plane.
-  Unbiased residuals:
-  as the residuals, but in this case the cluster used to calculate the
   residual is not used to calculate the track.
-  ResidualsAligner:
-  Distribution of the corrections obtained from each residual. The mean
   of these values is the final correction of this step. There is a 2D
   histogram with the distribution of the tracks slope too.

CorrectionOffset and CorrectionRotation: see above. This graphs should
converge to zero.

pt-track
--------

This command is used for calculate the tracks from the raw data, once
the noisy pixel are masked and the telescope and the DUT are aligned, so
it is usually run after pt-noisescan and pt-align. This means that it
will estrapolate the track parameters [IS IT CORRECT? IF SO, WHICH
PARAMETERS?] from the hits. This command is run in this way:

``pt-track [OPTIONS] input_file output_prefix``

``input_file`` is the ROOT file with the raw data

``output_prefix`` is the prefix for the generated files, which are:

-  output\_prefix-data.root
-  output\_prefix-hists.root

These files will be described later

Options
~~~~~~~

The possible options for this command are:

``-d, --device``: the device configuration file (default:
``device.toml``). This defines the sensor types, the geometry file path
and possibly the noise masks.

``-g, --geometry``: loads a separate geometry file, otherwise uses the
one defined in the device configuration file

``-c, --config``: the analysis configuration file (default:
analysis.toml).

``-u, --subsection``: a subsection of the analysis configuration file.

``-s, --skip_events``: skip the first n events (default: 0). Usually you
want to skip the events used in the alignment.

``-n, --num_events``: number of events to process (default: -1, i.e. all
of them)

Output files
~~~~~~~~~~~~

pt-track generates two output files, **output\_prefix-data.root** and
**output\_prefix-hists.root**.

The first one contains the data of the calculated tracks and it is not
supposed to be read by humans, it will be used by proteus for the
analysis. The second one, however, contains a lot of histograms, in
different folders:

-  EventInfo: basic information for the events, like distributions of
   track, cluster and hits for each event and sensor. [TRIGGER OFFSET
   AND PHASE?]
-  HitInfo: 2D histogram with the distribution of the hits, time (the
   time between the trigger and the rising edge of the signal) and value
   (aka TOT) of each hit. Furthermore there is the !d histogram with the
   distribution of time and value.
-  ClusterInfo: Information about the clusters, like the distribution of
   the size, the correlation between the size along the rows and columns
-  TrackInfo: distribution of number of clusters per track, of the
   reduced chi^2 of the tracks. Furthermore there is the distribution of
   the offset and of the slope of the tracks.
-  Occupancy: 2D histograms with the map of the hits, the hits used in a
   cluster and of the clusters. Furthermore there is the distribution of
   the occupancy.
-  Correlations: as for the pt-align output file (correlation method)
-  Residuals: as for the pt-align output file (residuals method)
-  UnbiasedResiduals: as for the pt-align output file (residuals method)

pt-match
--------

This command is used for . This command is run in this way:

``pt-match [OPTIONS] input_file output_prefix``

``input_file`` is the \*trees.root file generated by ``pt-track``

``output_prefix`` is the prefix for the generated files, which are:

-  output\_prefix-trees.root
-  output\_prefix-hists.root

These files will be described later

Options
~~~~~~~

The possible options for this command are:

``-d, --device``: the device configuration file (default:
``device.toml``). This defines the sensor types, the geometry file path
and possibly the noise masks.

``-g, --geometry``: loads a separate geometry file, otherwise uses the
one defined in the device configuration file [IS IT MANDATORY TO PUT THE
GEOMETRY FILE PATH IN THE DEV CONF FILE?]

``-c, --config``: the analysis configuration file (default:
analysis.toml).

``-u, --subsection``: a subsection of the analysis configuration file

``-s, --skip_events``: skip the first n events (default: 0)

``-n, --num_events``: number of events to process (default: -1, i.e. all
of them)

Output files
~~~~~~~~~~~~

pt-match generates two output files, **output\_prefix-trees.root** and
**output\_prefix-hists.root**.

The first one can be used for the analysis, and contains a folder for
each sample with two trees.

-  tracks: contain info for each track (``trk_*``) and the associated
   cluster (``clu_*``). If there is no associated cluster, its data are
   invalid (``clu_size = 0``, ``clu_value = -1`` and so on)
-  unmatched\_clusters: clusters that are not matched to any track

The second one contains some histograms folders.

-  TrackInfo: see pt-track documentation
-  UnbiasedResiduals: see pt-track documentation
-  Distances:
-  TrackTrack distances are the distributions of the distances between
   the track (along u, v and absolute) in the same event
-  TrackClusters distances are the distributions of the distances
   between all track and all clusters for each event
-  Match: as TrackCluster, but here are used only the matched
   track-cluster couples.

D2 is the weighted distance between track and cluster. The weight is a
combination of the track and cluster uncertainties.
