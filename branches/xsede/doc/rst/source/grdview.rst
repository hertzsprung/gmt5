.. index:: ! grdview

*******
grdview
*******

.. only:: not man

    grdview - Create 3-D perspective image or surface mesh from a grid

Synopsis
--------

.. include:: common_SYN_OPTs.rst_

**grdview** *relief_file* **-J**\ *parameters*
[ |SYN_OPT-B| ]
[ **-C**\ [*cptfile*]]
[ **-G**\ *drapefile* \| **-G**\ *grd_r*,\ *grd_g*,\ *grd_b* ]
[ **-I**\ *intensfile*\ \|\ *intensity* ] [ **-Jz**\ \|\ **Z**\ *parameters* ] [ **-K** ]
[ **-N**\ *level*\ [**+g**\ *fill*] ] [ **-O** ] [ **-P** ]
[ **-Q**\ *args*\ [**+m**] ]
[ |SYN_OPT-Rz| ]
[ **-S**\ *smooth* ]
[ **-T**\ [**s**][\ **o**\ [*pen*]] ]
[ |SYN_OPT-U| ]
[ **-W**\ **type**\ *pen* ]
[ |SYN_OPT-X| ]
[ |SYN_OPT-Y| ]
[ |SYN_OPT-c| ]
[ |SYN_OPT-n| ]
[ |SYN_OPT-p| ]
[ |SYN_OPT-t| ]

|No-spaces|

Description
-----------

**grdview** reads a 2-D grid file and produces a 3-D perspective plot by
drawing a mesh, painting a colored/grayshaded surface made up of
polygons, or by scanline conversion of these polygons to a raster image.
Options include draping a data set on top of a surface, plotting of
contours on top of the surface, and apply artificial illumination based
on intensities provided in a separate grid file. 

Required Arguments
------------------

*relief_file*
    2-D gridded data set to be imaged (the relief of the surface). (See
    GRID FILE FORMAT below.) 

.. |Add_-J| unicode:: 0x20 .. just an invisible code
.. include:: explain_-J.rst_

.. include:: explain_-Jz.rst_

Optional Arguments
------------------

.. include:: explain_-B.rst_

**-C**\ [*cptfile*]
    name of the color palette file. Must be present if you want
    (1) mesh plot with contours (**-Qm**), or
    (2) shaded/colored perspective image (**-Qs** or
    **-Qi**). For **-Qs**: You can specify that you want to skip a
    z-slice by setting red = -; to use a pattern give red =
    **P\|p**\ *dpi/pattern*\ [:**F**\ *color*\ [**B**\ *color*]].
    Alternatively, supply the name of a GMT color master CPT [rainbow] and let
    **grdview** automatically determine a 16-level continuous CPT from
    the grid's z-range.
**-G**\ *drapefile* \| **-G**\ *grd_r*,\ *grd_g*,\ *grd_b*
    Drape the image in *drapefile* on top of the relief provided by
    *relief_file*. [Default is *relief_file*]. Note that **-Jz** and
    **-N** always refers to the *relief_file*. The *drapefile* only
    provides the information pertaining to colors, which is looked-up
    via the cpt file (see **-C**). Alternatively, give three grid files
    separated by commas. These files must contain the red, green, and
    blue colors directly (in 0-255 range) and no cpt file is needed. The
    *drapefile* may be of higher resolution than the *relief_file*.
**-I**\ *intensfile*\ \|\ *intensity*
    Gives the name of a grid file with intensities in the (-1,+1) range,
    or a constant intensity to apply everywhere.
    [Default is no illumination]. 

.. include:: explain_-K.rst_

**-N**\ *level*\ [**+g**\ *fill*]
    Draws a plane at this z-level. If the optional *color* is provided
    via the **+g** modifier,
    the frontal facade between the plane and the data perimeter is
    colored. See **-Wf** for setting the pen used for the outline. 

.. include:: explain_-O.rst_

.. include:: explain_-P.rst_

**-Q**\ *args*\ [**+m**]
    Select one of following settings. For any of these choices, you may force
    a monochrome image by appending the modifier **+m**. Colors are then
    converted to shades of gray using the (monochrome television) YIQ transformation

    #. Specify **m** for mesh plot [Default], and optionally append *color* for a different mesh paint [white].
    #. Specify **mx** or **my** for waterfall plots (row or column profiles). Specify color as for plain **m**
    #. Specify **s** for surface plot, and optionally append **m** to have mesh lines drawn on top of surface.
    #. Specify **i** for image plot, and optionally append the effective dpi resolution for the rasterization [100].
    #. Specify **c**. Same as **-Qi** but will make nodes with z = NaN transparent, using the colormasking
       feature in PostScript Level 3 (the PS device must support PS Level 3). . 

.. |Add_-R| unicode:: 0x20 .. just an invisible code
.. include:: explain_-R.rst_

.. |Add_-Rz| replace:: This option may be used to
    indicate the range used for the 3-D axes [Default is region given by
    the *relief_file*]. You may ask for a larger *w/e/s/n* region to
    have more room between the image and the axes. A smaller region than
    specified in the *relief_file* will result in a subset of the grid.
.. include:: explain_-Rz.rst_

**-S**\ *smooth*
    Smooth the contours before plotting (see **grdcontour**) [Default is
    no smoothing].
**-T**\ [**s**\ ][\ **o**\ [*pen*]]
    Plot image without any interpolation. This involves converting each
    node-centered bin into a polygon which is then painted separately.
    Append **s** to skip nodes with z = NaN. This option is useful for
    categorical data where interpolating between values is meaningless.
    Optionally, append **o** to draw the tile outlines, and specify a
    custom pen if the default pen is not to your liking. As this option
    produces a flat surface it cannot be combined with **-JZ** or
    **-Jz**. 

.. include:: explain_-U.rst_

.. |Add_-V| unicode:: 0x20 .. just an invisible code
.. include:: explain_-V.rst_

**-W**\ **type**\ *pen*

    **-Wc**
        Draw contour lines on top of surface or mesh (not image). Append pen
        attributes used for the contours. [Default: width = 0.75p, color =
        black, style = solid].
    **-Wm**
        Sets the pen attributes used for the mesh. [Default: width = 0.25p,
        color = black, style = solid]. You must also select **-Qm** or
        **-Qsm** for meshlines to be drawn.
    **-Wf**
        Sets the pen attributes used for the facade. [Default: width =
        0.25p, color = black, style = solid]. You must also select **-N**
        for the facade outline to be drawn. 

.. include:: explain_-XY.rst_

.. include:: explain_-c.rst_

.. include:: explain_-n.rst_

.. |Add_perspective| unicode:: 0x20 .. just an invisible code
.. include:: explain_perspective.rst_

.. include:: explain_-t.rst_

.. include:: explain_help.rst_

.. include:: explain_grd_inout.rst_

.. include:: explain_grdresample.rst_

Examples
--------

To make a mesh plot from the file hawaii_grav.nc and drawing the
contours given in the color palette file hawaii.cpt on a Lambert map at
1.5 cm/degree along the standard parallels 18 and 24, with vertical
scale 20 mgal/cm, and looking at the surface from SW at 30 degree
elevation, run

   ::

    gmt grdview hawaii_grav.nc -Jl18/24/1.5c -Chawaii.cpt \
                -Jz0.05c -Qm -N-100 -p225/30 -Wc > hawaii_grav_image.ps

To create a illuminated color perspective plot of the gridded data set
image.nc, using the color palette file color.rgb, with linear scaling at
10 cm/x-unit and tickmarks every 5 units, with intensities provided by
the file intens.nc, and looking from the SE, use

   ::

    gmt grdview image.nc -Jx10.0c -Ccolor.rgb -Qs -p135/30 -Iintens.nc > image3D.ps

To make the same plot using the rastering option with dpi = 50, use

   ::

    gmt grdview image.nc -Jx10.0c -Ccolor.rgb -Qi50 -p135/30 -Iintens.nc > image3D.ps

To create a color PostScript perspective plot of the gridded data set
magnetics.nc, using the color palette file mag_intens.cpt, draped over
the relief given by the file topography.nc, with Mercator map width of 6
inch and tickmarks every 1 degree, with intensities provided by the file
topo_intens.nc, and looking from the SE, run

   ::

    gmt grdview topography.nc -JM6i -Gmagnetics.nc -Cmag_intens.cpt \
                -Qs -p140/30 -Itopo_intens.nc > draped3D.ps

Given topo.nc and the Landsat image veggies.ras, first run **grd2rgb**
to get the red, green, and blue grids, and then drape this image over
the topography and shade the result for good measure. The commands are

   ::

    gmt grd2rgb veggies.ras -Glayer_%c.nc
    gmt grdview topo.nc -JM6i -Qi -p140/30 -Itopo_intens.nc \
               -Glayer_r.nc,layer_g.nc,layer_b.nc > image.ps

Remarks
-------

For the **-Qs** option: PostScript provides no way of smoothly varying
colors within a polygon, so colors can only vary from polygon to
polygon. To obtain smooth images this way you may resample the grid
file(s) using **grdsample** or use a finer grid size when running
gridding programs like **surface** or **nearneighbor**. Unfortunately,
this produces huge PostScript files. The alternative is to use the
**-Qi** option, which computes bilinear or bicubic continuous color
variations within polygons by using scanline conversion to image the polygons.

See Also
--------

:doc:`gmt`, :doc:`grd2rgb`,
:doc:`gmtcolors`,
:doc:`grdcontour`,
:doc:`grdimage`,
:doc:`nearneighbor`,
:doc:`psbasemap`,
:doc:`pscontour`, :doc:`pstext`,
:doc:`surface`
