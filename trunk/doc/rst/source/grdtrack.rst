********
grdtrack
********

grdtrack - Sample grids at specified (x,y) locations

`Synopsis <#toc1>`_
-------------------

.. include:: common_SYN_OPTs.rst_

**grdtrack** [ *xyfile* ] **-G**\ *grd1* **-G**\ *grd2* ... [
**-A**\ **f**\ \|\ **p**\ \|\ **m**\ \|\ **r**\ \|\ **R**\ [**+l**\ ] ] [
**-C**\ *length*\ [**u**\ ]/\ *ds*\ [*spacing*\ ][**+a**\ ] ] [**-D**\ *dfile* ]
[ **-E**\ *line*\ [,\ *line*,...][**+a**\ *az*][**+i**\ *inc*[**u**]][**+l** \*length*[**u**]][**+n*\ *np*][**+r**\ *radius*[**u**]]
[ **-N** ] [ **-R**\ *west*/*east*/*south*/*north*\ [**r**\ ] ] [
**-S**\ *method*/*modifiers* ][ **-V**\ [*level*\ ] ] [ **-Z** ] [
**-b**\ [*ncol*\ ][**t**\ ][\ **+L**\ \|\ **+B**] ] [
**-f**\ [**i**\ \|\ **o**]\ *colinfo* ] [
**-g**\ [**a**\ ]\ **x**\ \|\ **y**\ \|\ **d**\ \|\ **X**\ \|\ **Y**\ \|\ **D**\ \|[*col*\ ]\ **z**\ [+\|-]\ *gap*\ [**u**\ ]
] [ **-h**\ [**i**\ \|\ **o**][*n*\ ] ] [
**-i**\ *cols*\ [**l**\ ][\ **s**\ *scale*][\ **o**\ *offset*][,\ *...*]
] [
**-n**\ [**b**\ \|\ **c**\ \|\ **l**\ \|\ **n**][**+a**\ ][\ **+b**\ *BC*][\ **+t**\ *threshold*]
] [ **-o**\ *cols*\ [,*...*] ] [ **-s**\ [*cols*\ ][\ **a**\ \|\ **r**]
] [ **-:**\ [**i**\ \|\ **o**] ]

`Description <#toc2>`_
----------------------

**grdtrack** reads one or more grid files (or a Sandwell/Smith IMG
files) and a table (from file or standard input; but see **-E** for
exception) with (x,y) [or (lon,lat)] positions in the first two columns
(more columns may be present). It interpolates the grid(s) at the
positions in the table and writes out the table with the interpolated
values added as (one or more) new columns. Alternatively (**-C**), the
input is considered to be line-segments and we create orthogonal
cross-profiles at each data point or with an equidistant separation and
sample the grid(s) along these profiles. A bicubic [Default], bilinear,
B-spline or nearest-neighbor (see **-n**) interpolation is used,
requiring boundary conditions at the limits of the region (see **-n**;
Default uses "natural" conditions (second partial derivative normal to
edge is zero) unless the grid is automatically recognized as periodic.)

.. include:: explain_commonitems.rst_

`Required Arguments <#toc4>`_
-----------------------------

**-G**\ *gridfile*
    *grdfile* is a 2-D binary grid file with the function f(x,y). If the
    specified grid is in Sandwell/Smith Mercator format you must append
    a comma-separated list of arguments that includes a scale to
    multiply the data (usually 1 or 0.1), the mode which stand for the
    following: (0) Img files with no constraint code, returns data at
    all points, (1) Img file with constraints coded, return data at all
    points, (2) Img file with constraints coded, return data only at
    constrained points and NaN elsewhere, `and (3) <and.html>`_ Img file
    with constraints coded, return 1 at constraints and 0 elsewhere, and
    optionally the max latitude in the IMG file [80.738]. You may repeat
    **-G** as many times as you have grids you wish to sample. The grids
    are sampled and results are output in the order given. (See GRID
    FILE FORMAT below.)

`Optional Arguments <#toc5>`_
-----------------------------

*xyfile*
    This is an ASCII (or binary, see **-bi**\ [*ncols*\ ][*type*\ ])
    file where the first 2 columns hold the (x,y) positions where the
    user wants to sample the 2-D data set.
**-A**\ **f**\ \|\ **p**\ **m**\ \|\ **r**\ \|\ **R**
    For track resampling (if **-C** is set) we can select how this is to
    be performed. Append **f** to keep original points, but add
    intermediate points if needed [Default], **m** as **f**, but first
    follow meridian (along y) then parallel (along x), **p** as **f**,
    but first follow parallel (along y) then meridian (along x), **r**
    to resample at equidistant locations; input points are not
    necessarily included in the output, and **R** as **r**, but adjust
    given spacing to fit the track length exactly. Finally, append
    **+l** if distances should be measured along rhumb lines
    (loxodromes). Ignored unless **-C** is used.
**-C**\ *length*/*ds*\ [/*spacing*]
    Use input line segments to create an equidistant and (optionally)
    equally-spaced set of crossing profiles along which we sample the
    grid(s) [Default simply samples the grid(s) at the input locations].
    Specify two length scales that control how the sampling is done:
    *length* sets the full length of each cross-profile, while *ds* is
    the distance increment along each cross-profile. Optionally, append
    **/**\ *spacing* for an equidistant spacing between cross-profiles
    [Default erects cross-profiles at the input coordinates]. By
    default, all cross-profiles have the same direction. Append **+a**
    to alternate the direction of cross-profiles. Append suitable units
    to *length*; it sets the unit used for *ds* [and *spacing*] (See
    UNITS below). The output columns will be *lon*, *lat*, *dist*,
    *azimuth*, *z1*, *z2*, ... (sampled value for each grid)
**-D**\ *dfile*
    In concert with **-C** we can save the (possibly resampled) original
    lines to the file *dfile* [Default only saves the cross-profiles].
    The columns will be *lon*, *lat*, *dist*, *azimuth*, *z1*, *z2*, ...
    (sampled value for each grid)
[ **-E**\ *line*\ [,\ *line*,...][**+a**\ *az*][**+i**\ *inc*[**u**]][**+l** \*length*[**u**]][**+n*\ *np*][**+r**\ *radius*[**u**]]
    Instead of reading input track coordinates, specify profiles via
    coordinates and modifiers. The format of each *line* is
    *start*/*stop*, where *start* or *stop* are either *lon*/*lat* (*x*/*y* for
    Cartesian data) or a 2-character XY key that uses the "pstext"-style
    justification format format to specify a point on the map as
    [LCR][BMT]. In addition, you can use Z-, Z+ to mean the global
    minimum and maximum locations in the grid (only available if only
    one grid is given). Instead of two coordinates you can specify an
    origin and one of **+a**, **+o**, or **+r**. You may append 
    **+i**\ *inc*[**u**] to set the sampling interval (append appropriate
    unit); if not given then we default to half the minimum grid interval,
    and if geographic we select great circle distances in km as the default
    unit and method. The **+a** sets the azimuth of a profile of given
    length starting at the given origin, while **+o** centers the profile
    on the origin; both require **+l**. For circular sampling specify
    **+r** to define a circle of given radius centered on the origin;
    this option requires either **+n** or **+i**.  The **+n**\ *np* sets
    the desired number of points, while **+l**\ *length* gives the
    total length of the profile.  Note: No track file will be read.
**-N**
    Do *not* skip points that fall outside the domain of the grid(s)
    [Default only output points within grid domain]. 

.. |Add_-R| unicode:: 0x20 .. just an invisible code
.. include:: explain_-R.rst_

**-S**\ *method*/*modifiers*
    In conjunction with **-C**, compute a single stacked profile from
    all profiles across each segment. Append how stacking should be
    computed: **a** = mean (average), **m** = median, **p** = mode
    (maximum likelihood), **l** = lower, **L** = lower but only consider
    positive values, **u** = upper, **U** = upper but only consider
    negative values [**a**\ ]. The *modifiers* control the output;
    choose one or more among these choices: **+a** : Append stacked
    values to all cross-profiles. **+d** : Append stack deviations to
    all cross-profiles. **+d** : Append data residuals (data - stack) to
    all cross-profiles. **+s**\ [*file*\ ] : Save stacked profile to
    *file* [grdtrack\_stacked\_profile.txt]. **+c**\ *fact* : Compute
    envelope on stacked profile as +/- *fact*\ \*\ *deviation* [2].
    Notes: (1) Deviations depend on *method* and are st.dev (**a**), L1
    scale (**e** and **p**), or half-range (upper-lower)/2. (2) The
    stacked profile file contains 1 plus groups of 4-6 columns, one
    group for each sampled grid. The first column holds cross distance,
    while the first 4 in a group hold stacked value, deviation, min
    value, and max value. If *method* is one of
    **a**\ \|\ **m**\ \|\ **p** then we also write the lower and upper
    confidence bounds (see **+c**). When one or more of **+a**, **+d**,
    and **+r** are used then we append the results to the end of each
    row for all cross-profiles. The order is always stacked value
    (**+a**), followed by deviations (**+d**) and residuals (**+r**).
    When more than one grid is sampled this sequence of 1-3 columns are
    repeated for each grid. 

.. |Add_-V| unicode:: 0x20 .. just an invisible code
.. include:: explain_-V.rst_

**-Z**
    Only write out the sampled z-values [Default writes all columns].
**-:**
    Toggles between (longitude,latitude) and (latitude,longitude)
    input/output. [Default is (longitude,latitude)]. 

.. |Add_-bi| replace:: [Default is 2 input columns]. 
.. include:: explain_-bi.rst_

.. |Add_-bo| replace:: [Default is one more than input]. 
.. include:: explain_-bo.rst_

.. |Add_-f| unicode:: 0x20 .. just an invisible code
.. include:: explain_-f.rst_

.. |Add_-g| unicode:: 0x20 .. just an invisible code
.. include:: explain_-g.rst_

.. |Add_-h| unicode:: 0x20 .. just an invisible code
.. include:: explain_-h.rst_

.. include:: explain_-icols.rst_

.. include:: explain_-n.rst_

.. include:: explain_-ocols.rst_

.. include:: explain_-s.rst_

.. include:: explain_help.rst_

.. include:: explain_distunits.rst_

.. include:: explain_precision.rst_

.. include:: explain_grd_input.rst_

`Hints <#toc9>`_
----------------

If an interpolation point is not on a node of the input grid, then a NaN
at any node in the neighborhood surrounding the point will yield an
interpolated NaN. Bicubic interpolation [default] yields continuous
first derivatives but requires a neighborhood of 4 nodes by 4 nodes.
Bilinear interpolation [**-n**\ ] uses only a 2 by 2 neighborhood, but
yields only zeroth-order continuity. Use bicubic when smoothness is
important. Use bilinear to minimize the propagation of NaNs, or lower
*threshold*.

`Examples <#toc10>`_
--------------------

To sample the file hawaii\_topo.nc along the SEASAT track track\_4.xyg
(An ASCII table containing longitude, latitude, and SEASAT-derived
gravity, preceded by one header record):

    grdtrack track\_4.xyg -Ghawaii\_topo.nc -h > track\_4.xygt

To sample the Sandwell/Smith IMG format file topo.8.2.img (2 minute
predicted bathymetry on a Mercator grid) and the Muller et al age grid
age.3.2.nc along the lon,lat coordinates given in the file
cruise\_track.xy, try

    grdtrack cruise\_track.xy -Gtopo.8.2.img,1,1 -Gage.3.2.nc > depths-age.d

To sample the Sandwell/Smith IMG format file grav.18.1.img (1 minute
free-air anomalies on a Mercator grid) along 100-km-long cross-profiles
that are orthogonal to the line segment given in the file track.xy,
erecting cross-profiles every 25 km and sampling the grid every 3 km, try

    grdtrack track.xy -Ggrav.18.1.img,0.1,1 -C100k/3/25 -Ar > xprofiles.d

`See Also <#toc11>`_
--------------------

`gmt <gmt.html>`_, `surface <surface.html>`_, `sample1d <sample1d.html>`_
