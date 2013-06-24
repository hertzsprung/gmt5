**********
grdcontour
**********

grdcontour - Make contour map using a grid

`Synopsis <#toc1>`_
-------------------

.. include:: common_SYN_OPTs.rst_

**grdcontour** *grid* **-C**\ [+]\ *cont\_int*\ \|\ *cpt*
**-J**\ *parameters* [ **-A**\ [**-**\ \|\ [+]\ *annot\_int*][*labelinfo*] ]
[ |SYN_OPT-B| ]
[ **-F**\ [**l**\ \|\ **r**] ]
[ **-G**\ [**d**\ \|\ **f**\ \|\ **n**\ \|\ **l**\ \|\ **L**\ \|\ **x**\ \|\ **X**]\ *params* ] 
[ **-Jz**\ \|\ **Z**\ *parameters* ] [ **-K** ] [ **-L**\ *low/high* ]
[ **-O** ] [ **-P** ] [ **-Q**\ *cut* ]
[ |SYN_OPT-Rz| ]
[ **-S**\ *smoothfactor* ]
[ **-T**\ [**+\|-**][*gap/length*][\ **:**\ [*labels*]] ]
[ |SYN_OPT-U| ]
[ |SYN_OPT-V| ]
[ **-W**\ [**+**][*type*]\ *pen* ]
[ |SYN_OPT-X| ]
[ |SYN_OPT-Y| ]
[ **-Z**\ [*factor*\ [/*shift*]][**p**] ]
[ |SYN_OPT-bo| ]
[ |SYN_OPT-c| ]
[ **-ho**\ [*n*] ]
[ |SYN_OPT-p| ]
[ |SYN_OPT-t| ]

|No-spaces|

Description
-----------

**grdcontour** reads a 2-D grid file and produces a contour map by
tracing each contour through the grid. *PostScript* code is generated
and sent to standard output. Various options that affect the plotting
are available. Alternatively, the x/y/z positions of the contour lines
may be saved to one or more output files (or stdout) and no plot is produced. 

Required Arguments
------------------

*grid*
    2-D gridded data set to be contoured. (See GRID FILE FORMATS below).

**-C**\ [+]\ *cont\_int*
    The contours to be drawn may be specified in one of three possible ways:

    (1) If *cont_int* has the suffix ".cpt" and can be opened as a
        file, it is assumed to be a color palette table. The color
        boundaries are then used as contour levels. If the cpt-file has
        annotation flags in the last column then those contours will be
        annotated. By default all contours are labeled; use **-A-** to
        disable all annotations.

    (2) If *cont_int* is a file but not a cpt-file, it is expected to
        contain contour levels in column 1 and a
        C(ontour) OR A(nnotate) in
        col 2. The levels marked C (or c) are contoured, the levels marked A
        (or a) are contoured and annotated. Optionally, a third column may
        be present and contain the fixed annotation angle for this contour
        level.

    (3) If no file is found, then *cont_int* is interpreted as a
        constant contour interval. However, if prepended with the + sign the
        *cont_int* is taken as meaning draw that single contour. The **-A**
        option offers the same possibility so they may be used together to
        plot only one annotated and one non-annotated contour.
        If **-A** is set and **-C** is not, then the contour interval is set
        equal to the specified annotation interval.

    If a file is given and **-T** is set, then only contours marked with
    upper case C or A will have tickmarks. In all cases the contour
    values have the same units as the grid. 

.. include:: explain_-J.rst_

Optional Arguments
------------------

**-A**\ [**-**\ \|\ [+]\ *annot\_int*][*labelinfo*]
    *annot_int* is annotation interval in data units; it is ignored if
    contour levels are given in a file. [Default is no annotations]. Append
    **-** to disable all annotations implied by **-C**. Alternatively prepend
    + to the annotation interval to plot that as a single contour. The optional
    *labelinfo* controls the specifics of the label formatting and consists
    of a concatenated string made up of any of the following control arguments:

.. include:: explain_labelinfo.rst_

.. include:: explain_-B.rst_

**-F**\ [**l**\ \|\ **r**]
    Force dumped contours to be oriented so that higher z-values are to the
    left (**-Fl** [Default]) or right (**-Fr**) as we move along the contour
    [Default is arbitrary orientation]. Requires **-D**.

**-G**\ [**d**\ \|\ **f**\ \|\ **n**\ \|\ **l**\ \|\ **L**\ \|\ **x**\ \|\ **X**]\ *params* 

.. include:: explain_contlabel.rst_

.. include:: explain_-Jz.rst_

.. include:: explain_-K.rst_

**-L**\ *low/high*
    Limit range: Do not draw contours for data values below *low* or
    above *high*. 

.. include:: explain_-O.rst_

.. include:: explain_-P.rst_

**-Q**\ *cut*
    Do not draw contours with less than *cut* number of points [Draw all contours]. 

.. |Add_-R| unicode:: 0x20 .. just an invisible code
.. include:: explain_-R.rst_

.. |Add_-Rz| replace:: [Default is region defined in the grid file].
.. include:: explain_-Rz.rst_

**-S**\ *smoothfactor*
    Used to resample the contour lines at roughly every
    (gridbox\_size/*smoothfactor*) interval.
**-T**\ [**+\|-**\ ][*gap/length*\ ][\ **:**\ [*labels*\ ]]
    Will draw tickmarks pointing in the downward direction every *gap*
    along the innermost closed contours. Append *gap* and tickmark
    length (append units as **c**, **i**, or **p**) or use defaults
    [15\ **p**/3\ **p**]. User may choose to tick only local highs or local
    lows by specifying **-T+** or **-T-**, respectively. Append
    **:**\ *labels* to annotate the centers of closed innermost contours
    (i.e, the local lows and highs). If no *labels* is appended we use -
    and + as the labels. Appending two characters, **:LH**, will plot
    the two characters (here, L and H) as labels. For more elaborate
    labels, separate the two label strings by a comma (e.g.,
    **:**\ *lo*,\ *hi*). If a file is given by **-C** and **-T** is set,
    then only contours marked with upper case C or A will have tickmarks
    [and annotation]. 

.. include:: explain_-U.rst_

.. |Add_-V| unicode:: 0x20 .. just an invisible code
.. include:: explain_-V.rst_

**-W**\ [**+**\ ][*type*\ ]\ *pen* :ref:`(more ...) <set-pens>`
    *type*, if present, can be **a** for annotated contours or **c** for
    regular contours [Default]. *pen* sets the attributes for the
    particular line. Default pen for annotated contours: 0.75p,black.
    Regular contours use pen 0.25p,black. If the **+** flag is prepended
    then the color of the contour lines are taken from the cpt file (see
    **-C**). If the **-** flag is prepended then the color from the cpt
    file is applied both to the contours and the contour annotations. 

.. include:: explain_-XY.rst_

**-Z**\ [*factor*\ [/*shift*]][**p**]
    Use to subtract *shift* from the data and multiply the results by
    *factor* before contouring starts [1/0]. (Numbers in **-A**, **-C**,
    **-L** refer to values after this scaling has occurred.) Append
    **p** to indicate that this grid file contains z-values that are
    periodic in 360 degrees (e.g., phase data, angular distributions)
    and that special precautions must be taken when determining
    0-contours. 

.. |Add_-bo| unicode:: 0x20 .. just an invisible code
.. include:: explain_-bo.rst_

.. include:: explain_-c.rst_

.. |Add_-f| unicode:: 0x20 .. just an invisible code
.. include:: explain_-f.rst_

.. |Add_-h| unicode:: 0x20 .. just an invisible code
.. include:: explain_-h.rst_

.. |Add_perspective| unicode:: 0x20 .. just an invisible code
.. include:: explain_perspective.rst_

.. include:: explain_-t.rst_

.. include:: explain_help.rst_

.. include:: explain_precision.rst_

.. include:: explain_grd_input.rst_

Examples
--------

To contour the file hawaii\_grav.nc every 25 mGal on a Mercator map at
0.5 inch/degree, annotate every 50 mGal (using fontsize = 10p), using 1
degree tickmarks, and draw 30 minute gridlines:

   ::

    gmt grdcontour hawaii_grav.nc -Jm0.5i -C25 -A50+f10p -B1g30m > hawaii_grav.ps

To contour the file image.nc using the levels in the file cont.d on a
linear projection at 0.1 cm/x-unit and 50 cm/y-unit, using 20 (x) and
0.1 (y) tickmarks, smooth the contours a bit, use "RMS Misfit" as
plot-title, use a thick red pen for annotated contours, and a thin,
dashed, blue pen for the rest, and send the output to the default printer:

   ::

    gmt grdcontour image.nc -Jx0.1c/50.0c -Ccont.d -S4 -B20/0.1:."RMS \
                   Misfit":-Wathick,red -Wcthinnest,blue,- | lp

The labeling of local highs and lows may plot outside the innermost
contour since only the mean value of the contour coordinates is used to
position the label.

To save the smoothed 100-m contour lines in topo.nc and separate them
into two multisegment files: contours_C.txt for closed and
contours_O.txt for open contours, try

   ::

    gmt grdcontour topo.nc -C100 -S4 -Dcontours_%c.txt

See Also
--------

`gmt <gmt.html>`_, `gmt.conf <gmt.conf.html>`_,
`gmtcolors <gmtcolors.html>`_,
`psbasemap <psbasemap.html>`_,
`grdimage <grdimage.html>`_, `grdview <grdview.html>`_,
`pscontour <pscontour.html>`_
