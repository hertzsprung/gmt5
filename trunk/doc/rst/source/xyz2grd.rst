*******
xyz2grd
*******

xyz2grd - Convert data table to a grid file

`Synopsis <#toc1>`_
-------------------

.. include:: common_SYN_OPTs.rst_

**xyz2grd** [ *table* ] **-G**\ *grdfile*
**-I**\ *xinc*\ [*unit*\ ][\ **=**\ \|\ **+**][/\ *yinc*\ [*unit*\ ][\ **=**\ \|\ **+**]]
**-R**\ *west*/*east*/*south*/*north*\ [**r**\ ] [
**-A**\ [**f**\ \|\ **l**\ \|\ **m**\ \|\ **n**\ \|\ **r**\ \|\ **s**\ \|\ **u**\ \|\ **z**]
] [ **-D**\ *xname*/*yname*/*zname*/*scale*/*offset*/*title*/*remark* ]
[ **-N**\ *nodata* ] [ **-S**\ [*zfile*\ ] ] [ **-V**\ [*level*\ ] ] [
**-Z**\ [*flags*\ ] ] [ **-bi**\ [*ncols*\ ][*type*\ ] ] [
**-f**\ *colinfo* ] [ **-h**\ [**i**\ \|\ **o**][*n*\ ] ] [
**-i**\ *cols*\ [**l**\ ][\ **s**\ *scale*][\ **o**\ *offset*][,\ *...*]
] [ **-r** ] [ **-:**\ [**i**\ \|\ **o**] ]

`Description <#toc2>`_
----------------------

**xyz2grd** reads one or more z or xyz tables and creates a binary grid
file. **xyz2grd** will report if some of the nodes are not filled in
with data. Such unconstrained nodes are set to a value specified by the
user [Default is NaN]. Nodes with more than one value will be set to the
mean value. As an option (using **-Z**), a 1-column z-table may be read
assuming all nodes are present (z-tables can be in organized in a number
of formats, see **-Z** below.) 

.. include:: explain_commonitems.rst_

`Required Arguments <#toc4>`_
-----------------------------

**-G**\ *grdfile*
    *grdfile* is the name of the binary output grid file. (See GRID FILE
    FORMAT below.) 

.. include:: explain_-I.rst_

.. |Add_-R| unicode:: 0x20 .. just an invisible code
.. include:: explain_-R.rst_

`Optional Arguments <#toc5>`_
-----------------------------

*table*
    One or more ASCII [or binary, see **-bi**\ [*ncols*\ ][*type*\ ]]
    files holding z or (x,y,z) values. The xyz triplets do not have to
    be sorted. One-column z tables must be sorted and the **-Z** must be
    set.
**-A**\ [**f**\ \|\ **l**\ \|\ **m**\ \|\ **n**\ \|\ **r**\ \|\ **s**\ \|\ **u**\ \|\ **z**]
    By default we will calculate mean values if multiple entries fall on
    the same node. Use **-A** to change this behavior, except it is
    ignored if **-Z** is given. Append **f** or **s** to simply keep the
    first or last data point that was assigned to each node. Append
    **l** or **u** to find the lowest (minimum) or upper (maximum) value
    at each node, respectively. Append **m** or **r** to compute mean or
    RMS value at each node, respectively. Append **n** to simply count
    the number of data points that were assigned to each node. Append
    **z** to sum multiple values that belong to the same node.
**-D**\ *xname*/*yname*/*zname*/*scale*/*offset*/*title*/*remark*
    Give values for *xname*, *yname*, *zname*, *scale*, *offset*,
    *title*, and *remark*. To leave some of these values untouched,
    specify = as the value. Alternatively, to allow "/" to be part of
    one of the values, use any non-alphanumeric character (and not the
    equal sign) as separator by both starting and ending with it. For
    example:
    **-D**:*xname*:*yname*:*zname*:*scale*:*offset*:*title*:*remark*:
**-N**\ *nodata*
    No data. Set nodes with no input xyz triplet to this value [Default
    is NaN]. For z-tables, this option is used to replace z-values that
    equal *nodata* with NaN.
**-S**\ [*zfile*\ ]
    Swap the byte-order of the input only. No grid file is produced. You
    must also supply the **-Z** option. The output is written to *zfile*
    (or stdout if not supplied). 

.. |Add_-V| unicode:: 0x20 .. just an invisible code
.. include:: explain_-V.rst_

**-Z**\ [*flags*\ ]
    Read a 1-column ASCII [or binary] table. This assumes that all the
    nodes are present and sorted according to specified ordering
    convention contained in *flags*. If incoming data represents rows,
    make *flags* start with **T**\ (op) if first row is y
    = ymax or **B**\ (ottom) if first row is y = ymin.
    Then, append **L** or **R** to indicate that first element is at
    left or right end of row. Likewise for column formats: start with
    **L** or **R** to position first column, and then append **T** or
    **B** to position first element in a row. Note: These two row/column
    indicators are only required for grids; for other tables they do not
    apply. For gridline registered grids: If data are periodic in x but
    the incoming data do not contain the (redundant) column at x = xmax,
    append **x**. For data periodic in y without redundant row at y =
    ymax, append **y**. Append **s**\ *n* to skip the first *n* number
    of bytes (probably a header). If the byte-order or the words needs
    to be swapped, append **w**. Select one of several data types (all
    binary except **a**):

    **A** ASCII representation of one or more floating point values per record

    **a** ASCII representation of a single item per record

    **c** int8\_t, signed 1-byte character

    **u** uint8\_t, unsigned 1-byte character

    **h** int16\_t, signed 2-byte integer

    **H** uint16\_t, unsigned 2-byte integer

    **i** int32\_t, signed 4-byte integer

    **I** uint32\_t, unsigned 4-byte integer

    **l** int64\_t, long (8-byte) integer

    **L** uint64\_t, unsigned long (8-byte) integer

    **f** 4-byte floating point single precision

    **d** 8-byte floating point double precision

    Default format is scanline orientation of ASCII numbers: **-ZTLa**.
    Note that **-Z** only applies to 1-column input. The difference
    between **A** and **a** is that the latter can decode both
    *date*\ **T**\ *clock* and *ddd:mm:ss[.xx]* formats while the former
    is strictly for regular floating point values. 

.. |Add_-bi| replace:: [Default is 3 input columns]. This option only applies
    to xyz input files; see **-Z** for z tables. 
.. include:: explain_-bi.rst_

.. |Add_-f| unicode:: 0x20 .. just an invisible code
.. include:: explain_-f.rst_

.. |Add_-h| replace:: Not used with binary data.
.. include:: explain_-h.rst_

.. include:: explain_-icols.rst_

.. |Add_nodereg| unicode:: 0x20 .. just an invisible code
.. include:: explain_nodereg.rst_

.. include:: explain_colon.rst_

.. include:: explain_help.rst_

.. include:: explain_float.rst_

.. include:: explain_grd_output.rst_

.. include:: explain_grd_coord.rst_

Swapping Limitations
--------------------

All data types can be read, even 64-bit integers, but internally grids
are stored using floats. Hence, integer values exceeding the float
type's 23-bit mantissa may not be represented exactly. When **-S** is
used no grids are implied and we read data into an intermediate double
container. This means all but 64-bit integers can be represented using
the double type's 53-bit mantissa.

Examples
--------

To create a grid file from the ASCII data in hawaii\_grv.xyz, use

    xyz2grd hawaii\_grv.xyz -Ddegree/degree/mGal/1/0/"Hawaiian
    Gravity"/"GRS-80 Ellipsoid used" -Ghawaii\_grv\_new.nc -R198/208/18/25 -I5m -V

To create a grid file from the raw binary (3-column, single-precision
scanline-oriented data raw.b, use

    xyz2grd raw.b -Dm/m/m/1/0/=/= -Graw.nc -R0/100/0/100 -I1 -V -Z -bi3f

To make a grid file from the raw binary USGS DEM (short integer
scanline-oriented data topo30. on the NGDC global relief Data CD-ROM,
with values of -9999 indicate missing data, one must on some machine
reverse the byte-order. On such machines (like Sun, use

    xyz2grd topo30. -Dm/m/m/1/0/=/= -Gustopo.nc -R234/294/24/50 -I30s -N-9999 -B -ZTLhw

Say you have received a binary file with 4-byte floating points that
were written on a machine of different byte-order than yours. You can
swap the byte-order with

    xyz2grd floats.bin -Snew\_floats.bin -V -Zf

See Also
--------

`gmt <gmt.html>`_, `grd2xyz <grd2xyz.html>`_, `grdedit <grdedit.html>`_,
`grdreformat <grdreformat.html>`_
