.. index:: ! sphtriangulate

**************
sphtriangulate
**************

.. only:: not man

    sphtriangulate - Delaunay or Voronoi construction of spherical lon,lat data

Synopsis
--------

.. include:: common_SYN_OPTs.rst_

**sphtriangulate** [ *table* ] [ **-A** ] [ **-C** ] [ **-D** ]
[ **-L**\ *unit* ] [ **-N**\ *nfile* ] [ **-Q**\ **d**\ \|\ **v** ]
[ **-T** ] [ |SYN_OPT-V| ]
[ |SYN_OPT-b| ]
[ |SYN_OPT-h| ]
[ |SYN_OPT-i| ]
[ |SYN_OPT-:| ]

|No-spaces|

Description
-----------

**sphtriangulate** reads one or more ASCII [or binary] files (or
standard input) containing lon, lat and performs a spherical Delaunay
triangulation, i.e., it find how the points should be connected to give
the most equilateral triangulation possible on the sphere. Optionally,
you may choose **-Qv** which will do further processing to obtain the
Voronoi polygons. Normally, either set of polygons will be written as
fillable segment output; use **-T** to write unique arcs instead. As an
option, compute the area of each triangle or polygon. The algorithm used
is STRIPACK.

Required Arguments
------------------

None.

Optional Arguments
------------------

.. |Add_intables| unicode:: 0x20 .. just an invisible code
.. include:: explain_intables.rst_

**-A**
    Compute the area of the spherical triangles (**-Qd**) or polygons
    (**-Qv**) and write the areas (in chosen units; see **-L**) in the
    output segment headers [no areas calculated].

**-C**
    For large data set you can save some memory (at the expense of more
    processing) by only storing one form of location coordinates
    (geographic or Cartesian 3-D vectors) at any given time, translating
    from one form to the other when necessary [Default keeps both arrays in memory].

**-D**
    Used with **-m** to skip the last (repeated) input vertex at the end
    of a closed segment if it equals the first point in the segment.
    Requires **-m** [Default uses all points].

**-L**\ *unit*
    Specify the unit used for distance and area calculations. Choose
    among **e** (m), **f** (foot), **k** (km), **m** (mile), **n**
    (nautical mile), **u** (survey foot), or **d** (spherical degree). A
    spherical approximation is used unless :ref:`PROJ_ELLIPSOID <PROJ_ELLIPSOID>` is set to
    an actual ellipsoid, in which case we convert latitudes to authalic
    latitudes before calculating areas. When degree is selected the
    areas are given in steradians.

**-N**\ *nfile*
    Write the information pertaining to each polygon (for Delaunay: the
    three node number and the triangle area (if **-A** was set); for
    Voronoi the unique node lon, lat and polygon area (if **-A** was
    set)) to a separate file. This information is also encoded in the
    segment headers of ASCII output files]. Required if binary output is needed.

**d**\ \|\ **v**
    Select between **d**\ elaunay or **v**\ oronoi mode [Delaunay].

**-T**
    Write the unique arcs of the construction [Default writes fillable
    triangles or polygons]. When used with **-A** we store arc lenght in
    the segment header in chosen unit (see **-L**).

.. |Add_-V| unicode:: 0x20 .. just an invisible code
.. include:: explain_-V.rst_

.. |Add_-bi| replace:: [Default is 2 input columns].
.. include:: explain_-bi.rst_

.. |Add_-bo| replace:: [Default is same as input].
.. include:: explain_-bo.rst_

.. |Add_-h| unicode:: 0x20 .. just an invisible code
.. include:: explain_-h.rst_

.. include:: explain_colon.rst_

.. |Add_nodereg| unicode:: 0x20 .. just an invisible code
.. include:: explain_nodereg.rst_

.. include:: explain_help.rst_

.. include:: explain_ascii_precision.rst_

Grid Values Precision
---------------------

Regardless of the precision of the input data, GMT programs that create
grid files will internally hold the grids in 4-byte floating point
arrays. This is done to conserve memory and furthermore most if not all
real data can be stored using 4-byte floating point values. Data with
higher precision (i.e., double precision values) will lose that
precision once GMT operates on the grid or writes out new grids. To
limit loss of precision when processing data you should always consider
normalizing the data prior to processing.

Examples
--------

To triangulate the points in the file testdata.txt, and make a Voronoi
diagram via :doc:`psxy`, use

   ::

    gmt sphtriangulate testdata.txt -Qv | psxy -Rg -JG30/30/6i -L -P -W1p -B0g30 | gv -

To compute the optimal Delaunay triangulation network based on the
multiple segment file globalnodes.d and save the area of each triangle
in the header record, try

   ::

    gmt sphtriangulate globalnodes.d -Qd -A > global_tri.d

See Also
--------

:doc:`gmt`, :doc:`triangulate`,
:doc:`sphinterpolate`,
:doc:`sphdistance`

References
----------

Renka, R, J., 1997, Algorithm 772: STRIPACK: Delaunay Triangulation and
Voronoi Diagram on the Surface of a Sphere, *AMC Trans. Math. Software*,
**23**\ (3), 416-434.
