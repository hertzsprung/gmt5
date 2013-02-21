********
splitxyz
********

splitxyz - Split xyz[dh] data tables into individual segments

`Synopsis <#toc1>`_
-------------------

**splitxyz** [ *table* ] **-C**\ *course\_change* [
**-A**\ *azimuth*/*tolerance* ] [ **-D**\ *minimum\_distance* ] [
**-F**\ *xy\_filter*/*z\_filter* ] [ **-N**\ *template* ] [
**-Q**\ *flags* ] [ **-S** ] [ **-V**\ [*level*\ ] ] [ **-Z** ] [
**-b**\ [*ncol*\ ][**t**\ ][\ **+L**\ \|\ **+B**] ] [
**-f**\ [**i**\ \|\ **o**]\ *colinfo* ] [
**-g**\ [**a**\ ]\ **x**\ \|\ **y**\ \|\ **d**\ \|\ **X**\ \|\ **Y**\ \|\ **D**\ \|[*col*\ ]\ **z**\ [+\|-]\ *gap*\ [**u**\ ]
] [ **-h**\ [**i**\ \|\ **o**][*n*\ ] ] [
**-i**\ *cols*\ [**l**\ ][\ **s**\ *scale*][\ **o**\ *offset*][,\ *...*]
] [ **-:**\ [**i**\ \|\ **o**] ]

`Description <#toc2>`_
----------------------

**splitxyz** reads a series of (x,y[,z]) records [or optionally
(x,y,z,d,h); see **-S** option] from standard input [or *xyz[dh]file*]
and splits this into separate lists of (x,y[,z]) series, such that each
series has a nearly constant azimuth through the x,y plane. There are
options to choose only those series which have a certain orientation, to
set a minimum length for series, and to high- or low-pass filter the z
values and/or the x,y values. **splitxyz** is a useful filter between
data extraction and **pswiggle** plotting, and can also be used to
divide a large x,y,z dataset into segments. The output is always in the
ASCII format; input may be ASCII or binary (see
**-bi**\ [*ncols*\ ][*type*\ ]). 

.. include:: explain_commonitems.rst_

`Required Arguments <#toc4>`_
-----------------------------

**-C**\ *course\_change*
    Terminate a segment when a course change exceeding *course\_change*
    degrees of heading is detected.

`Optional Arguments <#toc5>`_
-----------------------------

*table*
    One or more ASCII [or binary, see **-bi**\ [*ncols*\ ][*type*\ ]]
    files with 3 (or 2, see **-Z**) [or 5] columns holding (x,y,z[,d,h])
    data values. To use (x,y,z,d,h) input, sorted so that d is
    non-decreasing, specify the **-S** option; default expects (x,y,z)
    only. If no files are specified, **splitxyz** will read from
    standard input.
**-A**\ *azimuth*/*tolerance*
    Write out only those segments which are within +/- *tolerance*
    degrees of *azimuth* in heading, measured clockwise from North, [0 -
    360]. [Default writes all acceptable segments, regardless of
    orientation].
**-D**\ *minimum\_distance*
    Do not write a segment out unless it is at least *minimum\_distance*
    units long [0]
**-F**\ *xy\_filter*/*z\_filter*
    Filter the z values and/or the x,y values, assuming these are
    functions of d coordinate. *xy\_filter* and *z\_filter* are filter
    widths in distance units. If a filter width is zero, the filtering
    is not performed. The absolute value of the width is the full width
    of a cosine-arch low-pass filter. If the width is positive, the data
    are low-pass filtered; if negative, the data are high-pass filtered
    by subtracting the low-pass value from the observed value. If
    *z\_filter* is non-zero, the entire series of input z values is
    filtered before any segmentation is performed, so that the only edge
    effects in the filtering will happen at the beginning and end of the
    complete data stream. If *xy\_filter* is non-zero, the data is first
    divided into segments and then the x,y values of each segment are
    filtered separately. This may introduce edge effects at the ends of
    each segment, but prevents a low-pass x,y filter from rounding off
    the corners of track segments. [Default = no filtering].
**-N**\ *template*
    Write each segment to a separate output file [Default writes a
    multiple segment file to stdout]. Append a format template for the
    individual file names; this template **must** contain a C format
    specifier that can format an integer argument (the running segment
    number across all tables); this is usually %d but could be %08d
    which gives leading zeros, etc. [Default is
    splitxyz\_segment\_%d.{txt\|bin}, depending on
    **-bo**\ [*ncols*\ ][*type*\ ]]. Alternatively, give a template with
    two C format specifiers and we will supply the table number and the
    segment number within the table to build the file name.
**-Q**\ *flags*
    Specify your desired output using any combination of *xyzdh*, in any
    order. Do not space between the letters. Use lower case. The output
    will be ASCII (or binary, see **-bo**\ [*ncols*\ ][*type*\ ])
    columns of values corresponding to *xyzdh* [Default is
    **-Q**\ *xyzdh* (**-Q**\ *xydh* if **-Z** is set)].
**-S**
    Both d and h are supplied. In this case, input contains x,y,z,d,h.
    [Default expects (x,y,z) input, and d,h are computed from delta x,
    delta y. Use **-fg** to indicate map data; then x,y are assumed to
    be in degrees of longitude, latitude, distances are considered to be
    in kilometers, and angles are actually azimuths. Otherwise,
    distances are Cartesian in same units as x,y and angles are
    counter-clockwise from horizontal].
    
.. |Add_-V| unicode:: 0x0C .. just an invisible code
.. include:: explain_-V.rst_

**-Z**
    Data have x,y only (no z-column). 

.. |Add_-bi| replace:: [Default is 2, 3, or 5 input columns as set by **-S**, **-Z**]. 
.. include:: explain_-bi.rst_

.. |Add_-bo| replace:: [Default is 1-5 output columns as set by **-Q**]. 
.. include:: explain_-bo.rst_

.. |Add_-f| unicode:: 0x0C .. just an invisible code
.. include:: explain_-f.rst_

**-g**\ [**a**\ ]\ **x**\ \|\ **y**\ \|\ **d**\ \|\ **X**\ \|\ **Y**\ \|\ **D**\ \|[*col*\ ]\ **z**\ [+\|-]\ *gap*\ [**u**\ ]
    Do not let a segment have a gap exceeding *gap*; instead, split it into two segments. [Default ignores gaps]. 

.. |Add_-h| unicode:: 0x0C .. just an invisible code
.. include:: explain_-h.rst_

.. include:: explain_-icols.rst_

.. include:: explain_colon.rst_

.. include:: explain_help.rst_

.. include:: explain_precision.rst_

`Distance Calculations <#toc7>`_
--------------------------------

The type of input data is dictated by the **-f** option. If **-fg** is
given then x,y are in degrees of longitude, latitude, distances are in
kilometers, and angles are azimuths. Otherwise, distances are Cartesian
in same units as x,y and angles are counter-clockwise from horizontal.

`Examples <#toc8>`_
-------------------

Suppose you want to make a wiggle plot of magnetic anomalies on segments
oriented approximately east-west from a cruise called cag71 in the
region **-R**\ 300/315/12/20. You want to use a 100km low-pass filter to
smooth the tracks and a 500km high-pass filter to detrend the magnetic
anomalies. Try this:

gmtlist cag71 -R300/315/12/20 -Fxyzdh \| splitxyz -A90/15 -F100/-500
-D100 -S -V -fg \| pswiggle -R300/315/12/20 -Jm0.6 -Ba5f1:.cag71: -T1
-W0.75p -Ggray -Z200 > cag71\_wiggles.ps

MGD-77 users: For this application we recommend that you extract d, h
from **mgd77list** rather than have **splitxyz** compute them
separately.

Suppose you have been given a binary, double-precision file containing
lat, lon, gravity values from a survey, and you want to split it into
profiles named *survey*\ \_\ *###.txt* (when gap exceeds 100 km). Try
this:

splitxyz survey.bin -Nsurvey\_%03d.txt -V -gd100k -D100 -: -fg -bi3d

`See Also <#toc9>`_
-------------------

`gmt <gmt.html>`_ , `mgd77list <mgd77list.html>`_ , `pswiggle <pswiggle.html>`_
