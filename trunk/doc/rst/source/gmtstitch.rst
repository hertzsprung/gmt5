*********
gmtstitch
*********

gmtstitch - Join individual lines whose end points match within
tolerance

`Synopsis <#toc1>`_
-------------------

**gmtstitch** [ *table* ] [ **-C**\ [*closed*\ ] ] [
**-D**\ [*template*\ ] ] [ **-L**\ [*linkfile*\ ] ] [
**-Q**\ [*template*\ ] ] [ **-T**\ *cutoff*\ [*unit*\ ][/\ *nn\_dist*] ]
[ **-V**\ [*level*\ ] ] [
**-b**\ [*ncol*\ ][**t**\ ][\ **+L**\ \|\ **+B**] ] [
**-bo**\ [*ncols*\ ][*type*\ ] ] [ **-f**\ [**i**\ \|\ **o**]\ *colinfo*
] [
**-g**\ [**a**\ ]\ **x**\ \|\ **y**\ \|\ **d**\ \|\ **X**\ \|\ **Y**\ \|\ **D**\ \|[*col*\ ]\ **z**\ [+\|-]\ *gap*\ [**u**\ ]
] [ **-h**\ [**i**\ \|\ **o**][*n*\ ] ] [
**-i**\ *cols*\ [**l**\ ][\ **s**\ *scale*][\ **o**\ *offset*][,\ *...*]
] [ **-o**\ *cols*\ [,*...*] ] [ **-:**\ [**i**\ \|\ **o**] ]

`Description <#toc2>`_
----------------------

**gmtstitch** reads standard input or one or more data files, which may
be multisegment files, and examines the coordinates of the end points of
all line segments. If a pair of end points are identical or closer to
each other than the specified separation tolerance then the two line
segments are joined into a single segment. The process repeats until all
the remaining endpoints no longer pass the tolerance test; the resulting
segments are then written out to standard output or specified output
file. If it is not clear what the separation tolerance should be then
use **-L** to get a list of all separation distances and analyze them to
determine a suitable cutoff. 

.. include:: explain_commonitems.rst_

`Required Arguments <#toc4>`_
-----------------------------

None.

`Optional Arguments <#toc5>`_
-----------------------------

.. |Add_intables| unicode:: 0x0C .. just an invisible code
.. include:: explain_intables.rst_


**-C**\ [*closed*\ ]
    Write all the closed polygons to *closed* [gmtstitch\_closed.txt]
    and all other segments as they are to stdout. No stitching takes
    place. Use **-T**\ *cutoff* to set a minimum separation [0], and if
    *cutoff* is > 0 then we also explicitly close the polygons on
    output.
**-D**\ [*template*\ ]
    For multiple segment data, dump each segment to a separate output
    file [Default writes a single multiple segment file]. Append a
    format template for the individual file names; this template
    **must** contain a C format specifier that can format an integer
    argument (the segment number); this is usually %d but could be %08d
    which gives leading zeros, etc. Optionally, it may also contain the
    format %c *before* the integer; this will then be replaced by C
    (closed) or O (open) to indicate segment type. [Default is
    gmtstitch\_segment\_%d.txt]. Note that segment headers will be
    written in either case. For composite segments, a generic segment
    header will be written and the segment headers of individual pieces
    will be written out as comments to make it possible to identify
    where the stitched pieces came from.
**-L**\ [*linkfile*\ ]
    Writes the link information to the specified file
    [gmtstitch\_link.txt]. For each segment we write the original
    segment id, and for the beginning and end point of the segment we
    report the id of the closest segment, whether it is the beginning
    (B) or end (E) point that is closest, and the distance between those
    points in units determined by **-T**.
**-Q**\ [*template*\ ]
    Used with **-D** to a list file with the names of the individual
    output files. Optionally, append a filename template for the
    individual file names; this template **may** contain a C format
    specifier that can format an character (C or O for closed or open,
    respectively). [Default is gmtstitch\_list.txt].
**-T**\ *cutoff*\ [*unit*\ ][/\ *nn\_dist*]
    Specifies the separation tolerance in the data coordinate units [0];
    append distance unit (see UNITS). If two lines has end-points that
    are closer than this cutoff they will be joined. Optionally, append
    /*nn\_dist* which adds the requirement that a link will only be made
    if the second closest connection exceeds the *nn\_dist*. The latter
    distance must be given in the same units as *cutoff*. 

.. |Add_-V| unicode:: 0x0C .. just an invisible code
.. include:: explain_-V.rst_

.. |Add_-bi| replace:: [Default is 2 input columns]. 
.. include:: explain_-bi.rst_

.. |Add_-bo| replace:: [Default is same as input].
.. include:: explain_-bo.rst_

.. |Add_-f| unicode:: 0x0C .. just an invisible code
.. include:: explain_-f.rst_

.. |Add_-g| unicode:: 0x0C .. just an invisible code
.. include:: explain_-g.rst_

.. |Add_-h| unicode:: 0x0C .. just an invisible code
.. include:: explain_-h.rst_

.. include:: explain_-icols.rst_

.. include:: explain_-ocols.rst_

.. include:: explain_colon.rst_

.. include:: explain_help.rst_

.. include:: explain_distunits.rst_

.. include:: explain_precision.rst_

`Examples <#toc8>`_
-------------------

To combine the digitized segment lines segment\_\*.txt (whose
coordinates are in cm) into as few complete lines as possible, assuming
the end points slop could be up to 0.1 mm, run

gmtstitch segment\_\*.txt -Tf0.1 > new\_segments.txt

To combine the digitized segments in the multisegment file my\_lines.txt
(whose coordinates are in lon,lat) into as few complete lines as
possible, assuming the end points slop could be up to 150 m, and write
the complete segments to separate files called Map\_segment\_0001.dat,
Map\_segment\_0002.dat, etc., run

gmtstitch my\_lines.txt -T150e -DMap\_segment\_%04d.dat

`Bugs <#toc9>`_
---------------

The line connection does not work if a line only has a single point.
However, gmtstitch will correctly add the point to the nearest segment.
Running gmtstitch again on the new set of lines will eventually connect
all close lines.

`See Also <#toc10>`_
--------------------

`gmt <gmt.html>`_ , `mapproject <mapproject.html>`_
