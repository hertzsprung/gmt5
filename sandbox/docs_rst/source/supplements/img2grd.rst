*******
img2grd
*******

img2grd - Extract subset of img file in Mercator or Geographic format

`Synopsis <#toc1>`_
-------------------

**img2grd** *imgfile* **-G**\ *grdfile*
**-R**\ *west*/*east*/*south*/*north*\ [**r**\ ] **-T**\ *type* [ **-C**
] [ **-D**\ [*minlat/maxlat*\ ] ] [ **-E** ] [ **-I**\ *minutes* ] [
**-M** ] [ **-N**\ *navg* ] [ **-S**\ [*scale*\ ] ] [
**-V**\ [*level*\ ] ] [ **-W**\ *maxlon* ] [
**-n**\ [**b**\ \|\ **c**\ \|\ **l**\ \|\ **n**][**+a**\ ][\ **+b**\ *BC*][\ **+t**\ *threshold*]
]

`Description <#toc2>`_
----------------------

**img2grd** reads an img format file, extracts a subset, and writes it
to a grid file. The **-M** option dictates whether or not the Spherical
Mercator projection of the img file is preserved or if a Geographic grid
should be written by undoing the Mercator projection. If geographic grid
is selected you can also request a resampling onto the exact **-R**
given.

`Common Arguments And Specifications <#toc3>`_
----------------------------------------------

All options marked with an asterisk (\*) are common GMT command-line
options. Their full syntax as well as how to specify pens, pattern
fills, colors, and fonts can be found in the **gmt** man page. Note: No
space is allowed between the option flag and the associated arguments.

`Required Arguments <#toc4>`_
-----------------------------

*imgfile*
    A Mercator img format file such as the marine gravity or seafloor
    topography fields estimated from satellite altimeter data by
    Sandwell and Smith. If the user has set an environment variable
    **$GMT\_DATADIR**, then **img2grd** will try to find *imgfile* in
    **$GMT\_DATADIR**; else it will try to open *imgfile* directly.
**-G**\ *grdfile*
    *grdfile* is the name of the output grid file.
**-R**\ *west*/*east*/*south*/*north*\ [/*zmin*/*zmax*][**r**\ ]
    *west*, *east*, *south*, and *north* specify the region of interest,
    and you may specify them in decimal degrees or in
    [+-]dd:mm[:ss.xxx][W\|E\|S\|N] format. Append **r** if lower left
    and upper right map coordinates are given instead of w/e/s/n. The
    two shorthands **-Rg** and **-Rd** stand for global domain (0/360
    and -180/+180 in longitude respectively, with -90/+90 in latitude).
    Alternatively, specify the name of an existing grid file and the
    **-R** settings (and grid spacing, if applicable) are copied from
    the grid.

`Optional Arguments <#toc5>`_
-----------------------------

**-C**
    Set the x and y Mercator coordinates relative to projection center
    [Default is relative to lower left corner of grid]. Requires **-M**.
**-D**\ [*minlat/maxlat*\ ]
    Use the extended latitude range -80.738/+80.738. Alternatively,
    append *minlat/maxlat* as the latitude extent of the input img file.
    [Default is -72.006/72.006]. Not usually required since we can
    determine the extent from inspection of the file size.
**-E**
    Can be used when **-M** is not set to force the final grid to have
    the exact same region as requested with **-R**. By default, the
    final region is a direct projection of the original Mercator region
    and will typically extend slightly beyond the requested latitude
    range, and furthermore the grid increment in latitude does not match
    the longitude increment. However, the extra resampling introduces
    small interpolation errors and should only be used if the output
    grid must match the requested region and have x\_inc = y\_inc. In
    this case the region set by **-R** must be given in multiples of the
    increment (.e.g, **-R**\ 0/45/45/72).
**-I**
    Indicate *minutes* as the width of an input img pixel in minutes of
    longitude. [Default is 2.0]. Not usually required since we can
    determine the pixel size from inspection of the size.
**-M**
    Output a Spherical Mercator grid [Default is a geographic lon/lat
    grid]. The Spherical Mercator projection of the img file is
    preserved, so that the region **-R** set by the user is modified
    slightly; the modified region corresponds to the edges of pixels [or
    groups of *navg* pixels]. The grid file header is set so that the x
    and y axis lengths represent distance from the west and south edges
    of the image, measured in user default units, with **-Jm**\ 1 and
    the adjusted **-R**. By setting the default **PROJ\_ ELLIPSOID** =
    Sphere, the user can make overlays with the adjusted **-R** so that
    they match. See **EXAMPLES** below. The adjusted **-R** is also
    written in the grdheader remark, so it can be found later. See
    **-C** to set coordinates relative to projection center.
**-N**\ *navg*
    Average the values in the input img pixels into *navg* by *navg*
    squares, and create one output pixel for each such square. If used
    with **-T**\ *3* it will report an average constraint between 0 and
    1. If used with **-T**\ *2* the output will be average data value or
    NaN according to whether average constraint is > 0.5. *navg* must
    evenly divide into the dimensions of the imgfile in pixels. [Default
    *1* does no averaging].
**-S**\ [*scale*\ ]
    Multiply the img file values by *scale* before storing in grid file.
    [Default is 1.0]. For recent img files: img topo files are stored in
    (corrected) meters [**-S**\ 1]; free-air gravity files in mGal\*10
    [**-S**\ 0.1 to get mGal]; vertical deflection files in
    microradians\*10 [**-S**\ 0.1 to get microradians], vertical gravity
    gradient files in Eotvos\*50 [**-S**\ 0.02 to get Eotvos, or
    **-S**\ 0.002 to get mGal/km]). If no *scale* is given we try to
    determine the scale by examining the file name for clues.
**-T**\ *type*
    *type* handles the encoding of constraint information. *type* = 0
    indicates that no such information is encoded in the img file (used
    for pre-1995 versions of the gravity data) and gets all data. *type*
    > 0 indicates that constraint information is encoded (1995 and later
    (current) versions of the img files) so that one may produce a grid
    file as follows: **-T**\ *1* gets data values at all points,
    **-T**\ *2* gets data values at constrained points and NaN at
    interpolated points; **-T**\ *3* gets 1 at constrained points and 0
    at interpolated points [Default is 1].
**-V**\ [*level*\ ] (\*)
    Select verbosity level [c]. Particularly recommended here, as it is
    helpful to see how the coordinates are adjusted.
**-W**\ *maxlon*
    Indicate *maxlon* as the maximum longitude extent of the input img
    file. Versions since 1995 have had *maxlon* = 360.0, while some
    earlier files had *maxlon* = 390.0. [Default is 360.0].
**-^** (\*)
    Print a short message about the syntax of the command, then exits.
**-?** (\*)
    Print a full usage (help) message, including the explanation of
    options, then exits.
**--version** (\*)
    Print GMT version and exit.
**--show-sharedir** (\*)
    Print full path to GMT share directory and exit.

`Geographic Examples <#toc6>`_
------------------------------

The **-M** option should be excluded if you need the output grid to be
in geographic coordinates. To extract data in the region
**-R**-40/40/-70/-30 from *world\_grav.img.7.2* and reproject to yield
geographic coordinates, you can try

**img2grd** world\_grav.img.16.1 **-G**\ merc\_grav.nc
**-R**-40/40/-70/-30 **-V**

Because the latitude spacing in the img file is equidistant in Mercator
units, the resulting grid will not match the specified **-R** exactly,
and the latitude spacing will not equal the longitude spacing. If you
need an exact match with your **-R** and the same spacing in longitude
and latitude, use the **-E** option:

**img2grd** world\_grav.img.16.1 **-G**\ merc\_grav.nc
**-R**-40/40/-70/-30 **-E** **-V**

`Mercator Examples <#toc7>`_
----------------------------

Since the img files are in a Mercator projection, you should NOT extract
a geographic grid if your plan is to make a Mercator map. If you did
that you end of projecting and reprojection the grid, loosing
short-wavelength detail. Better to use **-M** and plot the grid using a
linear projection with the same scale as the desired Mercator projection
(see GMT Example 29).
 To extract data in the region **-R**-40/40/-70/-30 from
*world\_grav.img.7.2*, run

**img2grd** **-M** world\_grav.img.7.2 **-G**\ merc\_grav.nc
**-R**-40/40/-70/-30 **-V**

Note that the **-V** option tells us that the range was adjusted to
**-R**-40/40/-70.0004681551/-29.9945810754. We can also use **grdinfo**
to find that the grid file header shows its region to be
**-R**\ 0/80/0/67.9666667 This is the range of x,y we will get from a
Spherical Mercator projection using
**-R**-40/40/-70.0004681551/-29.9945810754 and **-Jm**\ 1. Thus, to take
ship.lonlatgrav and use it to sample the merc\_grav.nc, we can do this:

**gmtset** **PROJ\_ ELLIPSOID** Sphere
 **mapproject** **-R**-40/40/-70.0004681551/-29.9945810754 **-Jm**\ 1i
ship.lonlatgrav \| **grdtrack** **-G**\ merc\_grav.nc \| **mapproject**
**-R**-40/40/-70.0004681551/-29.9945810754 **-Jm**\ 1i **-I** >
ship.lonlatgravsat

It is recommended to use the above method of projecting and unprojecting
the data in such an application, because then there is only one
interpolation step (in **grdtrack**). If one first tries to convert the
grid file to lon,lat and then sample it, there are two interpolation
steps (in conversion and in sampling).

To make a lon,lat grid from the above grid we can use

**grdproject** merc\_grav.nc **-R**-40/40/-70.0004681551/-29.9945810754
**-Jm**\ 1i **-I** **-D**\ 2m **-G**\ grav.nc

In some cases this will not be easy as the **-R** in the two coordinate
systems may not align well. When this happens, we can also use (in fact,
it may be always better to use)

**grd2xyz** merc\_grav.nc \| **mapproject**
**-R**-40/40/-70.0004681551/-29.994581075 **-Jm**\ 1i **-I** \|
**surface** **-R**-40/40/-70/70 **-I**\ 2m **-G**\ grav.nc

To make a Mercator map of the above region, suppose our .gmtdefaults
**PROJ\_LENGTH\_UNIT** is inch. Then since the above merc\_grav.nc file
is projected with **-Jm**\ 1i it is 80 inches wide. We can make a map 8
inches wide by using **-Jx**\ 0.1i on any map programs applied to this
grid (e.g., **grdcontour**, **grdimage**, **grdview**), and then for
overlays which work in lon,lat (e.g., **psxy**, **pscoast**) we can use
the above adjusted **-R** and **-Jm**\ 0.1 to get the two systems to
match up.

However, we can be smarter than this. Realizing that the input img file
had pixels 2.0 minutes wide (or checking the nx and ny with grdinfo
merc\_grav.nc) we realize that merc\_grav.nc used the full resolution of
the img file and it has 2400 by 2039 pixels, and at 8 inches wide this
is 300 pixels per inch. We decide we don’t need that many and we will be
satisfied with 100 pixels per inch, so we want to average the data into
3 by 3 squares. (If we want a contour plot we will probably choose to
average the data much more (e.g., 6 by 6) to get smooth contours.) Since
2039 isn’t divisible by 3 we will get a different adjusted OPT(R) this
time:

**img2grd** **-M** world\_grav.img.7.2 **-G**\ merc\_grav\_2.nc
**-R**-40/40/-70/-30 **-N**\ 3 **-V**

This time we find the adjusted region is
**-R**-40/40/-70.023256525/-29.9368261101 and the output is 800 by 601
pixels, a better size for us. Now we can create an artificial
illumination file for this using **grdgradient**:

**grdgradient** merc\_grav\_2.nc **-G**\ illum.nc **-A**\ 0/270
**-N**\ e0.6

and if we also have a cpt file called "grav.cpt" we can create a color
shaded relief map like this:

**grdimage** merc\_grav\_2.nc **-I**\ illum.nc **-C**\ grav.cpt
**-Jx**\ 0.1i **-K** > map.ps
 **psbasemap** **-R**-40/40/-70.023256525/-29.9368261101 **-Jm**\ 0.1i
**-B**\ a10 **-O** >> map.ps

Suppose you want to obtain only the constrained data values from an img
file, in lat/lon coordinates. Then run **img2grd** with the **-T**\ 2
option, use **grd2xyz** to dump the values, pipe through grep -v NaN to
eliminate NaNs, and pipe through **mapproject** with the inverse
projection as above.

`See Also <#toc8>`_
-------------------

`*GMT*\ (1) <GMT.html>`_
