Internal structure
==================

The reconstruction is organized around a few major components:

*   `Device description`_
*   `Event data model`_
*   `Event loop`_

Device description
------------------

A device contains multiple sensors. Each sensor describe the size, geometry,
segmentation, and pixel masks of one physical pixel sensor device. A separate
geometry class stores the positions of the sensors and their local coordinate
systems in the global laboratory system. There are no distinctions made between
sensors belonging to the telescope and devices-under-test.

.. doxygenclass:: Mechanics::Device
    :outline:
    :members:
.. doxygenclass:: Mechanics::Sensor
    :outline:
    :members:
.. doxygenclass:: Mechanics::Geometry
    :outline:
    :members:
.. doxygenstruct:: Mechanics::Plane
    :outline:
    :members:

Event data model
----------------

A simple event data model is used that aims to be independent of any specific
file format. Data is stored as a set of events. In the case of a triggered data
aquisition method, one event should contain the data associated to one trigger.
With a continous, untriggered data aquisition on events should correspond to the
smallest reasonable readout block.

Each event contains a separate sensor event for each sensor configured in the
device. Each sensor event stores hits, clusters, and extrapolated track states
for the given sensor. Clusters are list of hits with additional combined
properties.

.. doxygenclass:: Storage::Event
    :outline:
    :members:
.. doxygenclass:: Storage::SensorEvent
    :outline:
    :members:
.. doxygenclass:: Storage::Hit
    :outline:
    :members:
.. doxygenclass:: Storage::Cluster
    :outline:
    :members:
.. doxygenclass:: Storage::Track
    :outline:
    :members:
.. doxygenclass:: Storage::TrackState
    :outline:
    :members:

Event loop
----------

The event loop is responsible for reading data, processing it, and writing it
back to disk.

Data is read from a single reader that is responsible from converting a specific
file format into the internal data format. Implementations for different file
formats must implement the reader interface to be used in the event loop.

All data processing is performed using a configurable set of algorithms.
Algorithms are independent from each other and can only communicate through the
data stored in the data model. Two different type of algorithms can be
implemented: processors and analyzers. A processor can modify an event, e.g. by
generating clusters from hits and adding them, but can not modify its internal
state. They must act as pure functions. Analyzers can not modify the event. It
can only read the given event, but is allowed to modify its internal state. Most
analyzers store some histograms that are written to disk after all events have
been processed. Algorithms are executed sequentially: processors are executed
first in the order in which they were added. Afterwards the analyzers are
executed.

The processed event data can be written to disk using an arbitrary number of
writers.

.. doxygenclass:: Loop::Reader
    :members:
.. doxygenclass:: Loop::Writer
    :members:
.. doxygenclass:: Loop::Processor
    :members:
.. doxygenclass:: Loop::Analyzer
    :members:
