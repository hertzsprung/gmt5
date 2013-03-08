*********
grdraster
*********

grdraster - Extract subregion from a binary raster and save as a GMT grid

Synopsis
--------

.. include:: ../../common_SYN_OPTs.rst_

**grdraster** [ *filenumber* \| *"text pattern"* ]
**-R**\ *west*/*east*/*south*/*north*\ [**r**\ ] [ **-G**\ *grdfile* ] [
**-I**\ *xinc*\ [**m**\ \|\ **s**][/\ *yinc*\ [**m**\ \|\ **s**]] ] [
**-J**\ *parameters* ] [ **-V**\ [*level*\ ] ] [
**-bo**\ [*ncols*][*type*] ] [ **-o**\ *cols*\ [,...] ]

Description
-----------

**grdraster** reads a file called *grdraster.info* from the current
working directory, the directories pointed to by the environment
variables **$GMT\_USERDIR** and **$GMT\_DATADIR**, or in
**$GMT\_SHAREDIR**/dbase (in that order). The file *grdraster.info*
defines binary arrays of data stored in scan-line format in data files.
Each file is given a *filenumber* in the info file. **grdraster**
figures out how to load the raster data into a grid file spanning a
region defined by **-R**. By default the grid spacing equals the raster
spacing. The **-I** option may be used to sub-sample the raster data. No
filtering or interpolating is done, however; the *x\_inc* and *y\_inc*
of the grid must be multiples of the increments of the raster file and
**grdraster** simply takes every n’th point. The output of **grdraster**
is either grid or pixel registered depending on the registration of the
raster used. It is up to the **GMT** system person to maintain the
*grdraster.info* file in accordance with the available rasters at each
site. Raster data sets are not supplied with **GMT** but can be obtained
by anonymous ftp and on CD-ROM (see README page in dbase directory).
**grdraster** will list the available files if no arguments are given.
Finally, **grdraster** will write xyz-triplets to stdout if no output
gridfile name is given 

.. include:: ../../explain_commonitems.rst_

Required Arguments
------------------

*filenumber*
    If an integer matching one of the files listed in the
    *grdraster.info* file is given we will use that data set, else we
    will match the given text pattern with the data set description in
    order to determine the data set.

.. |Add_-Rgeo| replace:: If **r** is appended, you may also specify a
    map projection to define the shape of your region. The output region
    will be rounded off to the nearest whole grid-step in both dimensions.
.. include:: ../../explain_-Rgeo.rst_

Optional Arguments
------------------

**-G**\ *grdfile*
    Name of output grid file. If not set, the grid will be written as
    ASCII (or binary; see **-bo**\ [*ncols*\ ][*type*\ ]) xyz-triplets
    to stdout instead. 

.. include:: ../../explain_-I.rst_

.. include:: ../../explain_-J.rst_

.. |Add_-V| unicode:: 0x20 .. just an invisible code
.. include:: ../../explain_-V.rst_

.. |Add_-bo| unicode:: 0x20 .. just an invisible code
.. include:: ../../explain_-bo.rst_
    
.. include:: ../../explain_-ocols.rst_
|
|   This option applies only if no **-G** option has been set. 

.. include:: ../../explain_help.rst_

Examples
--------

To extract data from raster 1, taking one point every 30 minutes, in an
area extended beyond 360 degrees to allow later filtering, run

grdraster 1 -R-4/364/-62/62 -I30m -Gdata.nc

To obtain data for an oblique Mercator projection we need to extract
more data that is actually used. This is necessary because the output of
**grdraster** has edges defined by parallels and meridians, while the
oblique map in general does not. Hence, to get all the data from the
ETOPO2 data needed to make a contour map for the region defined by its
lower left and upper right corners and the desired projection, use

grdraster ETOPO2 -R160/20/220/30r -Joc190/25.5/292/69/1 -Gdata.nc

To extract data from the 2 min Geoware relief blend and write it as
binary double precision xyz-triplets to standard output:

grdraster "2 min Geoware" -R20/25/-10/5 -bo > triplets.b

See Also
--------

`gmtdefaults <gmtdefaults.html>`_ , `GMT <GMT.html>`_ ,
`grdsample <grdsample.html>`_ , `grdfilter <grdfilter.html>`_
