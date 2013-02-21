*******
kml2gmt
*******

kml2gmt - Extract GMT table data from Google Earth KML files

`Synopsis <#toc1>`_
-------------------

**kml2gmt** [ *kmlfiles* ] [ **-V**\ [*level*\ ] ] [ **-Z** ] [
**-bo**\ [*ncols*\ ][*type*\ ] ] [ **-:**\ [**i**\ \|\ **o**] ]

`Description <#toc2>`_
----------------------

**kml2gmt** reads a Google Earth KML file and outputs a GMT table file.
Only KML files that contain points, lines, or polygons can be processed.
This is a bare-bones operation that aims to extract coordinates and
possibly the name and description tags of each feature. The main use
intended is to capture coordinates modified in Google Earth and then
reinsert the modified data into the original GMT data file. For a more
complete reformatting, consider using **ogr2ogr -f** "GMT" somefile.gmt
somefile.kml. 

.. include:: explain_commonitems.rst_

`Required Arguments <#toc4>`_
-----------------------------

None.

`Optional Arguments <#toc5>`_
-----------------------------

*kmlfiles*
    Name of one or more KML files to work on. If not are given, then
    standard input is read.
**-Z**
    Output the altitude coordinates as GMT z coordinates [Default will
    output just longitude and latitude]. 

.. include:: explain_-V.rst_

.. include:: explain_-bo.rst_

.. include:: explain_colon.rst_

.. include:: explain_help.rst_

`Examples <#toc6>`_
-------------------

To extract the lon,lat values from the KML file google.kml, try

kml2gmt google.kml -V > google.txt

`See Also <#toc7>`_
-------------------

`*gmt*\ (1) <gmt.html>`_ , `*gmt.conf*\ (5) <gmt.conf.html>`_ ,
`*img2google*\ (1) <img2google.html>`_ ,
`*ps2raster*\ (1) <ps2raster.html>`_ `*gmt2kml*\ (1) <gmt2kml.html>`_
