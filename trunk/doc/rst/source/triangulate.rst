***********
triangulate
***********

triangulate - Do optimal (Delaunay) triangulation and gridding of
Cartesian table data [method]

Synopsis
--------

.. include:: common_SYN_OPTs.rst_

**triangulate** [ *table* ] [ **-Dx**\ \|\ **y** ] [ **-E**\ *empty* ] [
**-G**\ *grdfile* ]
[ |SYN_OPT-I| ]
[ **-J**\ *parameters* ] [ **-M** ] [ **-Q** ]
[ |SYN_OPT-R| ] [ **-S** ]
[ |SYN_OPT-V| ] [ **-Z** ]
[ **-b**\ [*ncol*][**t**][\ **+L**\ \|\ **+B**] ]
[ **-f**\ [**i**\ \|\ **o**]\ *colinfo* ]
[ |SYN_OPT-h| ]
[ |SYN_OPT-i| ]
[ **-r** ] [ **-:**\ [**i**\ \|\ **o**] ]

|No-spaces|

Description
-----------

**triangulate** reads one or more ASCII [or binary] files (or standard
input) containing x,y[,z] and performs Delaunay triangulation, i.e., it
find how the points should be connected to give the most equilateral
triangulation possible. If a map projection (give **-R** and **-J**) is
chosen then it is applied before the triangulation is calculated. By
default, the output is triplets of point id numbers that make up each
triangle and is written to standard output. The id numbers refer to the
points position (line number, starting at 0 for the first line) in the
input file. As an option, you may choose to create a multiple segment
file that can be piped through `psxy <psxy.html>`_ to draw the triangulation
network. If **-G** **-I** are set a grid will be calculated based on the
surface defined by the planar triangles. The actual algorithm used in
the triangulations is either that of Watson [1982] [Default] or Shewchuk
[1996] (if installed; type **triangulate -** to see which method is
selected). This choice is made during the **GMT** installation. 

Required Arguments
------------------

None.

Optional Arguments
------------------

.. |Add_intables| unicode:: 0x20 .. just an invisible code
.. include:: explain_intables.rst_

**-Dx**\ \|\ **y**
    Take either the *x*- or *y*-derivatives of surface represented by
    the planar facets (only used when **-G** is set).
**-E**\ *empty*
    Set the value assigned to empty nodes when **-G** is set [NaN].
**-G**\ *grdfile*
    Use triangulation to grid the data onto an even grid (specified with
    **-R** **-I**). Append the name of the output grid file. The
    interpolation is performed in the original coordinates, so if your
    triangles are close to the poles you are better off projecting all
    data to a local coordinate system before using **triangulate** (this
    is true of all gridding routines).
**-I**
    *x\_inc* [and optionally *y\_inc*] sets the grid size for optional
    grid output (see **-G**). Append **m** to indicate arc minutes or
    **s** to indicate arc seconds. 

.. include:: explain_-J.rst_

**-M**
    Output triangulation network as multiple line segments separated by
    a segment header record.
**-Q**
    Output the edges of the Voronoi cells instead [Default is Delaunay
    triangle edges]. Requires **-R** and is only available if linked
    with the Shewchuk [1996] library. Note that **-Z** is ignored on
    output. 

.. |Add_-R| unicode:: 0x20 .. just an invisible code
.. include:: explain_-R.rst_

**-S**
    Output triangles as polygon segments separated by a segment header
    record. Requires Delaunay triangulation. 

.. |Add_-V| unicode:: 0x20 .. just an invisible code
.. include:: explain_-V.rst_

**-Z**
    Controls whether we read (x,y) or (x,y,z) data and if z should be
    output when **-M** or **-S** are used [Read (x,y) only]. 

.. |Add_-bi| replace:: [Default is 2 input columns]. 
.. include:: explain_-bi.rst_

.. |Add_-bo| replace:: [Default is same as input].  Node ids are stored as double triplets. 
.. include:: explain_-bo.rst_

.. |Add_-f| unicode:: 0x20 .. just an invisible code
.. include:: explain_-f.rst_

.. |Add_-h| unicode:: 0x20 .. just an invisible code
.. include:: explain_-h.rst_

.. include:: explain_-icols.rst_

.. |Add_nodereg| replace:: (Only valid with **-G**). 
.. include:: explain_nodereg.rst_

.. include:: explain_colon.rst_

.. include:: explain_help.rst_

.. include:: explain_precision.rst_

.. include:: explain_float.rst_

Examples
--------

To triangulate the points in the file samples.xyz, store the triangle
information in a binary file, and make a grid for the given area and spacing, use

    triangulate samples.xyz -bo -R0/30/0/30 -I2 -Gsurf.nc > samples.ijk

To draw the optimal Delaunay triangulation network based on the same
file using a 15-cm-wide Mercator map, use

    triangulate samples.xyz -M -R-100/-90/30/34 -JM15c \| psxy
    -R-100/-90/30/34 -JM15c -W0.5p -B1 > network.ps

To instead plot the Voronoi cell outlines, try

    triangulate samples.xyz -M -Q -R-100/-90/30/34 -JM15c \|
    psxy -R-100/-90/30/34 -JM15c -W0.5p -B1 > cells.ps

See Also
--------

`gmt5 <gmt5.html>`_, `pscontour <pscontour.html>`_

References
----------

Watson, D. F., 1982, Acord: Automatic contouring of raw data, *Comp. &
Geosci.*, **8**, 97-101.

Shewchuk, J. R., 1996, Triangle: Engineering a 2D Quality Mesh Generator
and Delaunay Triangulator, First Workshop on Applied Computational
Geometry (Philadelphia, PA), 124-133, ACM, May 1996.

`<www.cs.cmu.edu/~quake/triangle.html>`_
