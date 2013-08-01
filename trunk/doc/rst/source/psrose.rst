******
psrose
******

.. only:: not man

    psrose - Plot a polar histogram (rose, sector, windrose diagrams)

Synopsis
--------

.. include:: common_SYN_OPTs.rst_

**psrose** [ *table* ] [ **-A**\ [**r**]*sector\_width* ]
[ |SYN_OPT-B| ]
[ **-C**\ [*mode_file*] ]
[ **-D** ] [ **-I** ] [ **-G**\ *fill* ] [ **-I** ] [ **-K** ]
[ **-L**\ [*wlabel*/*elabel*/*slabel*/*nlabel*] ] [ **-M**\ *parameters* ]
[ **-O** ] [ **-P** ]
[ **-R**\ *r0*/*r1*/*az_0*/*az_1* ]
[ **-S**\ [**n**]*radial\_scale* ] [ **-T** ]
[ |SYN_OPT-U| ]
[ |SYN_OPT-V| ]
[ **-W**\ [**v**]\ *pen* ]
[ |SYN_OPT-X| ]
[ |SYN_OPT-Y| ]
[ **-Z**\ **u**\ \|\ *scale* ]
[ |SYN_OPT-bi| ]
[ |SYN_OPT-c| ]
[ |SYN_OPT-h| ]
[ |SYN_OPT-i| ]
[ |SYN_OPT-p| ]
[ |SYN_OPT-t| ]
[ |SYN_OPT-:| ]

|No-spaces|

Description
-----------

**psrose** reads (length,azimuth) pairs from *file* [or standard input]
and generates *PostScript* code that will plot a windrose diagram.
Optionally (with **-A**), polar histograms may be drawn (sector diagram
or rose diagram). Options include full circle and half circle plots. The
*PostScript* code is written to standard output. The outline of the
windrose is drawn with the same color as **MAP\_DEFAULT\_PEN**. 

Required Arguments
------------------

None.

Optional Arguments
------------------

.. |Add_intables| replace:: If a file with only
    azimuths are given, use **-i** to indicate the single column with
    azimuths; then all lengths are set to unity (see **-Zu** to set
    actual lengths to unity as well).
.. include:: explain_intables.rst_

**-A**\ [**r**]*sector\_width*
    Gives the sector width in degrees for sector and rose diagram.
    [Default 0 means windrose diagram]. Use **-Ar** to draw rose
    diagram instead of sector diagram. 

.. include:: explain_-B.rst_
|
|   Remember that "x" here is
|   radial distance and "y" is azimuth. The ylabel may be used to plot a figure caption.

**-C**\ [*mode_file*]
    Plot vectors showing the principal directions given in the *modes*
    file. If no file is given, compute and plot mean direction. See
    **-M** to control vector attributes.
**-D**
    Shift sectors so that they are centered on the bin interval (e.g.,
    first sector is centered on 0 degrees).
**-F**
    Do not draw the scale length bar [Default plots scale in lower right
    corner]
**-G**\ *fill*
    Selects shade, color or pattern for filling the sectors [Default is
    no fill].
**-I**
    Inquire. Computes statistics needed to specify useful **-R**. No
    plot is generated. 

.. include:: explain_-K.rst_

**-L**\ [*wlabel*/*elabel*/*slabel*/*nlabel*]
    Specify labels for the 0, 90, 180, and 270 degree marks. For
    full-circle plot the default is WEST/EAST/SOUTH/NORTH and for
    half-circle the default is 90W/90E/-/0. A - in any entry disables
    that label. Use **-L** with no argument to disable all four labels

**-M**\ *parameters*
    Used with **-C** to modify vector parameters. For vector heads,
    append vector head *size* [Default is 0, i.e., a line]. See VECTOR
    ATTRIBUTES for specifying additional attributes. 

.. include:: explain_-O.rst_

.. include:: explain_-P.rst_

**-R**\ *r0*/*r1*/*az\_0*/*az\_1*
    Specifies the ’region’ of interest in (r,azimuth) space. r0 is 0, r1
    is max length in units. For azimuth, specify either -90/90 or 0/180
    for half circle plot or 0/360 for full circle.

**-S**\ [**n**]*radial_scale*
    Specifies radius of circle. Use **-Sn** to normalize input radii to
    go from 0 to 1.

**-T**
    Specifies that the input data is orientation data (has a 180 degree
    ambiguity) instead of true 0-360 degree directions [Default]. 

.. include:: explain_-U.rst_

.. |Add_-V| unicode:: 0x20 .. just an invisible code
.. include:: explain_-V.rst_

**-W**\ *pen*
    Set pen attributes for sector outline or rose plot. [Default is no
    outline]. Use **-Wv**\ *pen* to change pen used to draw vector
    (requires **-C**) [Default is same as sector outline]. 

.. include:: explain_-XY.rst_

**-Z**\ *scale*
    Multiply the data radii by *scale*. E.g., use **-Z**\ 0.001 to
    convert your data from m to km. To exclude the radii from
    consideration, set them all to unity with **-Zu** [Default is no
    scaling].
**-:**
    Input file has (azimuth,radius) pairs rather than the expected
    (radius,azimuth). 

.. |Add_-bi| replace:: [Default is 2 input columns]. 
.. include:: explain_-bi.rst_

.. include:: explain_-c.rst_

.. |Add_-h| unicode:: 0x20 .. just an invisible code
.. include:: explain_-h.rst_

.. include:: explain_-icols.rst_

.. |Add_perspective| unicode:: 0x20 .. just an invisible code
.. include:: explain_perspective.rst_

.. include:: explain_-t.rst_

.. include:: explain_help.rst_

.. include:: explain_vectors.rst_

Examples
--------

To plot a half circle rose diagram of the data in the file
fault_segments.az_r (containing pairs of (azimuth, length in meters),
using a 10 degree bin sector width, on a circle of radius = 3 inch, grid
going out to radius = 150 km in steps of 25 km with a 30 degree sector
interval, radial direction annotated every 50 km, using a light blue
shading outlined by a solid red pen (width = 0.75 points), draw the mean
azimuth, and shown in Portrait orientation, use:

   ::

    gmt psrose fault_segments.az_r -R0/150/-90/90 -B50g25:"Fault
               length":/g30:."Rose diagram":-S3i -Ar10 -Glightblue
               -W0.75p,red -Z0.001 -C -P -T -: > half_rose.ps

To plot a full circle wind rose diagram of the data in the file
lines.r_az, on a circle of radius = 5 cm, grid going out to radius =
500 units in steps of 100 with a 45 degree sector interval, using a
solid pen (width = 0.5 point, and shown in landscape [Default]
orientation with UNIX timestamp and command line plotted, use:

   ::

    gmt psrose lines.az_r -R0/500/0/360 -S5c -Bg100/g45:."Windrose diagram": -W0.5p -Uc | lpr

Bugs
----

No default radial scale and grid settings for polar histograms. User
must run **psrose** **-I** to find max length in binned data set.

See Also
--------

`gmt <gmt.html>`_, `gmt.conf <gmt.conf.html>`_,
`gmtcolors <gmtcolors.html>`_, `pshistogram <pshistogram.html>`_
