***********
grdreformat
***********

grdreformat - Convert between different grid formats

`Synopsis <#toc1>`_
-------------------

.. include:: common_SYN_OPTs.rst_

**grdreformat** *ingrdfile*\ [*=id*\ [*/scale/offset*\ [*/NaNvalue*]]]
*outgrdfile*\ [*=id*\ [*/scale/offset*\ [*/NaNvalue*]][\ *:driver*\ [*/datatype*]]]
[ **-N** ]
[ |SYN_OPT-R| ]
[ |SYN_OPT-V| ]
[ |SYN_OPT-f| ]

|No-spaces|

Description
-----------

**grdreformat** reads a grid file in one format and writes it out using
another format. As an option the user may select a subset of the data to
be written and to specify scaling, translation, and NaN-value. 

Required Arguments
------------------

*ingrdfile*
    The grid file to be read. Append format =\ *id* code if not a
    standard COARDS-compliant netCDF grid file. If =\ *id* is set (see
    below), you may optionally append *scale* and *offset*. These
    options will scale the data and then offset them with the specified
    amounts after reading.
    If *scale* and *offset* are supplied you may also append a value
    that represents 'Not-a-Number' (for floating-point grids this is
    unnecessary since the IEEE NaN is used; however integers need a
    value which means no data available). The *scale* and *offset*
    modifiers may be left empty to select default values (scale = 1,
    offset = 0).

*outgrdfile*
    The grid file to be written. Append format =\ *id* code if not a
    standard COARDS-compliant netCDF grid file. If =\ *id* is set (see
    below), you may optionally append *scale* and *offset*. These
    options are particularly practical when storing the data as
    integers, first removing an offset and then scaling down the values.
    Since the scale and offset are applied in reverse order when
    reading, this does not affect the data values (except for
    round-offs).

    If *scale* and *offset* are supplied you may also append a value
    that represents 'Not-a-Number' (for floating-point grids this is
    unnecessary since the IEEE NaN is used; however integers need a
    value which means no data available). The *scale* and *offset*
    modifiers may be left empty to select default values (scale = 1,
    offset = 0), or you may specify *a* for auto-adjusting the scale
    and/or offset of packed integer grids (=\ *id/a* is a shorthand for
    =\ *id/a/a*). When *id*\ =\ *gd*, the file will be saved using the
    GDAL library. Append the format *:driver* and optionally the output
    *datatype*. The driver names are those used by GDAL itself (e.g.,
    netCDF, GTiFF, etc.), and the data type is one of
    *u8*\ \|\ *u16*\ \|\ *i16*\ \|\ *u32*\ \|\ *i32*\ \|\ *float32*,
    where ’i’ and ’u’ denote signed and unsigned integers respectively.
    The default type is *float32*. Note also that both driver names and
    data types are case insensitive.

    Consider setting :ref:`IO_NC4_DEFLATION_LEVEL <IO_NC4_DEFLATION_LEVEL>`
    to reduce file size and to further increase read/write performace.
    Especially when working with subsets of global grids, masks, and grids with
    repeating grid values, the improvement is usually significant.

Optional Arguments
------------------

**-N**
    Suppress the writing of the **GMT** header structure. This is useful
    when you want to write a native grid to be used by **grdraster**. It
    only applies to native grids and is ignored for netCDF output. 

.. |Add_-R| unicode:: 0x20 .. just an invisible code
.. include:: explain_-R.rst_

.. |Add_-V| unicode:: 0x20 .. just an invisible code
.. include:: explain_-V.rst_

.. |Add_-f| unicode:: 0x20 .. just an invisible code
.. include:: explain_-f.rst_

.. include:: explain_help.rst_

Format Identifier
-----------------

By default, grids will be written as floating point data stored in
binary files using the netCDF format and meta-data structure. This
format is conform the COARDS conventions. **GMT** versions prior to 4.1
produced netCDF files that did not conform to these conventions.
Although these files are still supported, their use is deprecated. To
write other than floating point COARDS-compliant netCDF files, append
the =\ *id* suffix to the filename *outgrdfile*.

When reading files, **grdreformat** and other **GMT** programs will try
to automatically recognize the type of the input grid file. If this
fails you may append the =\ *id* suffix to the filename *ingrdfile*.

+----------+---------------------------------------------------------------+
| ID       | Explanation                                                   |
+----------+---------------------------------------------------------------+
| **nb**   | GMT netCDF format (8-bit integer, COARDS, CF-1.5)             |
+----------+---------------------------------------------------------------+
| **ns**   | GMT netCDF format (16-bit integer, COARDS, CF-1.5)            |
+----------+---------------------------------------------------------------+
| **ni**   | GMT netCDF format (32-bit integer, COARDS, CF-1.5)            |
+----------+---------------------------------------------------------------+
| **nf**   | GMT netCDF format (32-bit float, COARDS, CF-1.5)              |
+----------+---------------------------------------------------------------+
| **nd**   | GMT netCDF format (64-bit float, COARDS, CF-1.5)              |
+----------+---------------------------------------------------------------+
| **cb**   | GMT netCDF format (8-bit integer, deprecated)                 |
+----------+---------------------------------------------------------------+
| **cs**   | GMT netCDF format (16-bit integer, deprecated)                |
+----------+---------------------------------------------------------------+
| **ci**   | GMT netCDF format (32-bit integer, deprecated)                |
+----------+---------------------------------------------------------------+
| **cf**   | GMT netCDF format (32-bit float, deprecated)                  |
+----------+---------------------------------------------------------------+
| **cd**   | GMT netCDF format (64-bit float, deprecated)                  |
+----------+---------------------------------------------------------------+
| **bm**   | GMT native, C-binary format (bit-mask)                        |
+----------+---------------------------------------------------------------+
| **bb**   | GMT native, C-binary format (8-bit integer)                   |
+----------+---------------------------------------------------------------+
| **bs**   | GMT native, C-binary format (16-bit integer)                  |
+----------+---------------------------------------------------------------+
| **bi**   | GMT native, C-binary format (32-bit integer)                  |
+----------+---------------------------------------------------------------+
| **bf**   | GMT native, C-binary format (32-bit float)                    |
+----------+---------------------------------------------------------------+
| **bd**   | GMT native, C-binary format (64-bit float)                    |
+----------+---------------------------------------------------------------+
| **rb**   | SUN rasterfile format (8-bit standard)                        |
+----------+---------------------------------------------------------------+
| **rf**   | GEODAS grid format GRD98 (NGDC)                               |
+----------+---------------------------------------------------------------+
| **sf**   | Golden Software Surfer format 6 (32-bit float)                |
+----------+---------------------------------------------------------------+
| **sd**   | Golden Software Surfer format 7 (64-bit float, read-only)     |
+----------+---------------------------------------------------------------+
| **af**   | Atlantic Geoscience Center format AGC (32-bit float)          |
+----------+---------------------------------------------------------------+
| **ei**   | ESRI Arc/Info ASCII Grid Interchange format (ASCII integer)   |
+----------+---------------------------------------------------------------+
| **ef**   | ESRI Arc/Info ASCII Grid Interchange format (ASCII float)     |
+----------+---------------------------------------------------------------+
| **gd**   | Import/export through GDAL                                    |
+----------+---------------------------------------------------------------+

Gmt Standard Netcdf Files
-------------------------

The standard format used for grdfiles is based on netCDF and conforms to
the COARDS conventions. Files written in this format can be read by
numerous third-party programs and are platform-independent. Some
disk-space can be saved by storing the data as bytes or shorts in stead
of integers. Use the *scale* and *offset* parameters to make this work
without loss of data range or significance. For more details, see
Appendix B.

**Multi-variable grid files**

By default, **GMT** programs will read the first 2-dimensional grid
contained in a COARDS-compliant netCDF file. Alternatively, use
*ingrdfile*\ **?**\ *varname* (ahead of any optional suffix **=**\ *id*)
to specify the requested variable *varname*. Since **?** has special
meaning as a wildcard, escape this meaning by placing the full filename
and suffix between quotes.

**Multi-dimensional grids**

To extract one *layer* or *level* from a 3-dimensional grid stored in a
COARDS-compliant netCDF file, append both the name of the variable and
the index associated with the layer (starting at zero) in the form:
*ingrdfile*\ **?**\ *varname*\ **[**\ *layer*\ **]**. Alternatively,
specify the value associated with that layer using parentheses in stead
of brackets:
*ingridfile*\ **?\ `*varname*\ **(**\ *level*\ **)** <varname.level.html>`_
.**

In a similar way layers can be extracted from 4- or even 5-dimensional
grids. For example, if a grid has the dimensions (parameter, time,
depth, latitude, longitude), a map can be selected by using:
*ingridfile*\ **?**\ *varname*\ **(**\ *parameter*,\ *time*,\ *depth*\ **)**.

Since question marks, brackets and parentheses have special meanings on
the command line, escape these meanings by placing the full filename and
suffix between quotes.

Native Binary Files
-------------------

For binary native **GMT** files the size of the **GMT** grdheader block
is *hsize* = 892 bytes, and the total size of the file is *hsize* + *nx*
\* *ny* \* *item_size*, where *item_size* is the size in bytes of each
element (1, 2, 4). Bit grids are stored using 4-byte integers, each
holding 32 bits, so for these files the size equation is modified by
using ceil (*nx* / 32) \* 4 instead of *nx*. Note that these files are
platform-dependent. Files written on Little Endian machines (e.g., PCs)
can not be read on Big Endian machines (e.g., most workstations). Also
note that it is not possible for **GMT** to determine uniquely if a
4-byte grid is float or int; in such cases it is best to use the *=ID*
mechanism to specify the file format. In all cases a native grid is
considered to be signed (i.e., there are no provision for unsigned short
ints or unsigned bytes). For header and grid details, see Appendix B. 

.. include:: explain_float.rst_

Examples
--------

To extract the second layer from a 3-dimensional grid named temp from a
COARDS-compliant netCDF file climate.nc:

   ::

    gmt grdreformat climate.nc?temp[1] temp.nc -V

To create a 4-byte native floating point grid from the COARDS-compliant
netCDF file data.nc:

   ::

    gmt grdreformat data.nc ras_data.b4=bf -V

To make a 2-byte short integer file, scale it by 10, subtract 32000,
setting NaNs to -9999, do

   ::

    gmt grdreformat values.nc shorts.i2=bs/10/-32000/-9999 -V

To create a Sun standard 8-bit rasterfile for a subset of the data file
image.nc, assuming the range in image.nc is 0-1 and we need 0-255, run

   ::

    gmt grdreformat image.nc -R-60/-40/-40/-30 image.ras8=rb/255/0 -V

To convert etopo2.nc to etopo2.i2 that can be used by **grdraster**, try

   ::

    gmt grdreformat etopo2.nc etopo2.i2=bs -N -V

To creat a dumb file saved as a 32 bits float GeoTiff using GDAL, run

   ::

    gmt grdmath -Rd -I10 X Y MUL = lixo.tiff=gd:GTiff

See Also
--------

`gmt.conf <gmt.conf.html>`_, `gmt <gmt.html>`_, `grdmath <grdmath.html>`_
