*********
ps2raster
*********

ps2raster - Convert [E]PS file(s) to other formats using GhostScript

`Synopsis <#toc1>`_
-------------------

**ps2raster** *psfile(s)* [ **-A**\ [**u**\ [*margins*\ ]\|-] ] [
**-C**\ *gs\_option* ] [ **-D**\ *outdir* ] [ **-E**\ *resolution* ] [
**-G**\ *ghost\_path* ] [ **-L**\ *listfile* ] [ **-P** ] [
**-Q**\ [**g**\ \|\ **t**][1\|2\|4] ] [ **-S** ] [
**-Tb**\ \|\ **e**\ \|\ **E**\ \|\ **f**\ \|\ **F**\ \|\ **j**\ \|\ **g**\ \|\ **G**\ \|\ **m**\ \|\ **t**
] [ **-V** ] [
**-W**\ [**+g**\ ][\ **+t**\ *docname*][\ **+n**\ *layername*][\ **+o**\ *foldername*][\ **+a**\ *altmode*\ [*alt*\ ]][\ **+l**\ *minLOD/maxLOD*][\ **+f**\ *minfade/maxfade*][\ **+u**\ *URL*]
]

`Description <#toc2>`_
----------------------

**ps2raster** converts one or more *PostScript* files to other formats
(BMP, EPS, JPEG, PDF, PNG, PPM, TIFF) using GhostScript. Input file
names are read from the command line or from a file that lists them. The
size of the resulting images is determined by the BoundingBox (or
HiResBoundingBox, if present). As an option, a tight (HiRes)BoundingBox
may be computed first. As another option, it can compute ESRI type world
files used to reference, for instance, tif files and make them be
recognized as geotiff. 

.. include:: explain_commonitems.rst_

`Required Arguments <#toc4>`_
-----------------------------

*psfiles*
    Names of *PostScript* files to be converted. The output files will
    have the same name (unless **-F** is used) but with the conventional
    extension name associated to the raster format (e.g., .jpg for the
    jpeg format). Use **-D** to redirect the output to a different
    directory.

`Optional Arguments <#toc5>`_
-----------------------------

**-A**\ [**u**\ ][*margins*\ ][**-**\ ]
    Adjust the BoundingBox and HiResBoundingBox to the minimum required
    by the image content. Append **u** to first remove any GMT-produced
    time-stamps. Optionally, append extra margins to the bounding box.
    Give either one (uniform), two (x and y) or four (individual sides)
    margins; append unit [Default is set by PROJ\_LENGTH\_UNIT].
    Alternatively, use **-A-** to override any automatic setting of
    **-A** by **-W**.
**-C**\ *gs\_option*
    Specify a single, custom option that will be passed on to
    GhostScript as is. Repeat to add several options [none].
**-D**\ *outdir*
    Sets an alternative output directory (which must exist) [Default is
    the same directory as the PS files]. Use **-D.** to place the output
    in the current directory instead.
**-E**\ *resolution*
    Set raster resolution in dpi [default = 720 for PDF, 300 for others].
**-F**
    Force the output file name. By default output names are constructed
    using the input names as base, which are appended with an
    appropriate extension. Use this option to provide a different name,
    but without extension. Extension is still determined automatically.
**-G**\ *ghost\_path*
    Full path to your GhostScript executable. NOTE: For Unix systems
    this is generally not necessary. Under Windows, the ghostscript path
    is now fetched from the registry. If this fails you can still add
    the GS path to system’s path or give the full path here. (e.g.,
    **-G**\ c:\\programs\\gs\\gs9.02\\bin\\gswin64c). WARNING: because
    of the poor decision of embedding the bits on the gs exe name we
    cannot satisfy both the 32 and 64 bits ghostscript executable names.
    So in case of ’get from registry’ failure the default name (when no
    **-G** is used) is the one of the 64 bits version, or gswin64c
**-L**\ *listfile*
    The *listfile* is an ASCII file with the names of the *PostScript*
    files to be converted.
**-N**
    This option is obsolete. Use **-S** to print the GhostScript
    command, if applicable. Use **-Te** to save the intermediate EPS
    file.
**-P**
    Force Portrait mode. All Landscape mode plots will be rotated back
    so that they show unrotated in Portrait mode. This is practical when
    converting to image formats or preparing EPS or PDF plots for
    inclusion in documents.
**-Q**\ [**g**\ \|\ **t**][1\|2\|4]
    Set the anti-aliasing options for **g**\ raphics or **t**\ ext.
    Append the size of the subsample box (1, 2, or 4) [4]. Default is no
    anti-aliasing (same as *bits* = 1).
**-S**
    Print to standard output the GhostScript command after it has been
    executed.
**-Tb**\ \|\ **e**\ \|\ **E**\ \|\ **f**\ \|\ **F**\ \|\ **j**\ \|\ **g**\ \|\ **G**\ \|\ **m**\ \|\ **t**
    Sets the output format, where **b** means BMP, **e** means EPS,
    **E** means EPS with PageSize command, **f** means PDF, **F** means
    multi-page PDF, **j** means JPEG, **g** means PNG, **G** means
    transparent PNG (untouched regions are transparent), **m** means
    PPM, and **t** means TIFF [default is JPEG]. For **bjgt** you can
    append - to get a grayscale image only. The EPS format can be
    combined with any of the other formats. For example, **-Tef**
    creates both an EPS and a PDF file. The **-TF** creates a multi-page
    PDF file from the list of input PS or PDF files. It requires **-F**
    option. 

.. |Add_-V| unicode:: 0x0C .. just an invisible code
.. include:: explain_-V.rst_

**-W**\ [**+g**\ ][\ **+t**\ *docname*][\ **+n**\ *layername*][\ **+o**\ *foldername*][\ **+a**\ *altmode*\ [*alt*\ ]][\ **+l**\ *minLOD/maxLOD*][\ **+f**\ *minfade/maxfade*][\ **+u**\ *URL*]
    Write a ESRI type world file suitable to make (e.g) .tif files be
    recognized as geotiff by softwares that know how to do it. Be aware,
    however, that different results are obtained depending on the image
    contents and if the **-B** option has been used or not. The trouble
    with the **-B** option is that it creates a frame and very likely
    its annotations. That introduces pixels outside the map data extent,
    and therefore the map extents estimation will be wrong. To avoid
    this problem use --MAP\_FRAME\_TYPE=inside option which plots all
    annotations and ticks inside the image and therefore does not
    compromise the coordinate computations. Pay attention also to the
    cases when the plot has any of the sides with whites only because
    than the algorithm will fail miserably as those whites will be eaten
    by the GhostScript. In that case you really must use **-B** or use a
    slightly off-white color.

    Together with **-V** it prints on screen the gdal\_translate
    (gdal\_translate is a command line tool from the GDAL package)
    command that reads the raster + world file and creates a true
    geotiff file. Use **-W+g** to do a system call to gdal\_translate
    and create a geoTIFF image right away. The output file will have a
    .tiff extension.

    The world file naming follows the convention of jamming a ’w’ in the
    file extension. So, if output is tif **-Tt** the world file is a
    .tfw, for jpeg we have a .jgw and so on. This option automatically
    sets **-A** **-P**.

    Use **-W+k** to create a minimalist KML file that allows loading the
    image in GoogleEarth. Note that for this option the image must be in
    geographical coordinates. If not, a warning is issued but the KML
    file is created anyway. Several modifier options are available to
    customize the KML file in the form of **+**\ *opt* strings. Append
    **+t**\ *title* to set the document title [GMT KML Document],
    **+n**\ *layername* to set the layer name, and
    **+a**\ */altmode*\ [*altitude*\ ] to select one of 5 altitude modes
    recognized by Google Earth that determines the altitude (in m) of
    the image: **G** clamped to the ground, **g** append altitude
    relative to ground, **a** append absolute altitude, **s** append
    altitude relative to seafloor, and **S** clamp it to the seafloor.
    Control visibility of the layer with the **+l**\ *minLOD/maxLOD* and
    **+f**\ *minfade/maxfade* options. Finally, if you plan to leave the
    image itself on a server and only distribute the KML, use
    **+u**\ *URL* to prepend the URL to the image reference. If you are
    building a multi-component KML file then you can issue a KML snipped
    without the KML header and trailer by using the **+o**\ *foldername*
    modification; it will enclose the image and associated KML code
    within a KML folder of the specified name. See the KML documentation
    for further explanation
    (http://code.google.com/apis/kml/documentation/).

    Further notes on the creation of georeferenced rasters.
    **ps2raster** can create a georeferenced raster image with a world
    file OR uses GDAL to convert the GMT *PostScript* file to geotiff.
    GDAL uses Proj.4 for it’s projection library. To provide with the
    information it needs to do the georeferencing, GMT 4.5 embeds a
    comment near the start of the *PostScript* file defining the
    projection using Proj.4 syntax. Users with pre-GMT v4.5 *PostScript*
    files, or even non-GMT ps files, can provide the information
    **ps2raster** requires by manually editing a line into the
    *PostScript* file, prefixed with %%PROJ.

    For example the command **pscoast** **-JM0/12c** **-R**-10/-4/37/43
    **-W1** **-Di** **-Bg30m** --MAP\_FRAME\_TYPE=inside > cara.ps

    adds this comment line

    %%PROJ: merc -10.0 -4.0 37.0 43.0 -1113194.908 -445277.963
    4413389.889 5282821.824 +proj=merc +lon\_0=0 +k=-1 +x\_0=0 +y\_0=0
    +a=6378137.0 +b=6356752.314245

    where ’merc’ is the keyword for the coordinate conversion; the 2 to
    5th elements contain the map limits, 6 to 9th the map limits in
    projected coordinates and the rest of the line has the regular proj4
    string for this projection. 

.. include:: explain_help.rst_

`Notes <#toc6>`_
----------------

The conversion to raster images (BMP, JPEG, PNG, PPM or TIFF) inherently
results in loss of details that are available in the original
*PostScript* file. Choose a resolution that is large enough for the
application that the image will be used for. For web pages, smaller dpi
values suffice, for Word documents and PowerPoint presentations a higher
dpi value is recommended. **ps2raster** uses the loss-less Flate
compression technique when creating JPEG, PNG and TIFF images.

EPS is a vector, not a raster format. Therefore, the **-E** option has
no effect on the creation of EPS files. Using the option **-Te** will
remove PageSize commands from the *PostScript* file and will adjust the
BoundingBox when the **-A** option is used. Note the original and
required BoundingBox is limited to integer points, hence Adobe added the
optional HiResBoundingBox to add more precision in sizing. The **-A**
option calculates both and writes both to the EPS file used in the
rasterization (and output if **-Te** is set).

Although PDF is also a vector format, the **-E** option has an effect on
the resolution of pattern fills and fonts that are stored as bitmaps in
the document. **ps2raster** therefore uses a larger default resolution
when creating PDF files. In order to obtain high-quality PDF files, the
*/prepress* options are in effect, allowing only loss-less Flate
compression of raster images embedded in the *PostScript* file.

Although **ps2raster** was developed as part of the **GMT**, it can be
used to convert *PostScript* files created by nearly any graphics
program. However, **-Au** is **GMT**-specific.

See Appendix C of the **GMT Technical Reference and Cookbook** for more
information on how **ps2raster** is used to produce graphics that can be
inserted into other documents (articles, presentations, posters, etc.).

`Examples <#toc7>`_
-------------------

To convert the file psfile.ps to PNG using a tight BoundingBox and
rotating it back to normal orientation in case it was in Landscape mode:

ps2raster psfile.ps -A -P -Tg

To create a 3 pages PDF file from 3 individual PS files

ps2raster -TF -Fabc a.ps b.ps c.ps

To create a simple linear map with pscoast and convert it to tif with a
.tfw the tight BoundingBox computation.

pscoast -JX12cd -R-10/-4/37/43 -W1 -Di -Bg30m -P -G200
--MAP\_FRAME\_TYPE=inside > cara.ps

ps2raster cara -Tt -W

To create a Mercator version of the above example and use GDAL to
produce a true geotiff file.

pscoast -JM0/12c -R-10/-4/37/43 -W1 -Di -Bg30m -P -G200
--MAP\_FRAME\_TYPE=inside > cara.ps

gdalwarp -s\_srs +proj=merc cara.tif carageo.tiff

To create a Polar Stereographic geotiff file of Patagonia

pscoast -JS-55/-60/15c -R-77/-55/-57.5/-48r -Di -Gred -P -Bg2
--MAP\_FRAME\_TYPE=inside > patagonia.ps

ps2raster patagonia.ps -Tt -W+g -V

To create a simple KMZ file for use in Google Earth, try

grdimage lonlatgrid.nc -Jx1 -Ccolors.cpt -P -B0g2
--MAP\_FRAME\_TYPE=inside > tile.ps

ps2raster tile.ps -Tg -W+k+t"my title"+l256/-1 -V

(These commands assume that GhostScript can be found in your system’s path.)

`Binary Data <#toc8>`_
----------------------

**GMT** programs can produce binary *PostScript* image data and this is
determined by the default setting PS\_IMAGE\_FORMAT. Because
**ps2raster** needs to process the input files on a line-by-line basis
you need to make sure the image format is set to *ascii* and not *bin*.

`Ghostscript Options <#toc9>`_
------------------------------

Most of the conversions done in **ps2raster** are handled by
GhostScript. On most Unixes this program is available as **gs**; for
Windows there is a version called **gswin32c**. GhostScript accepts a
rich selection of command-line options that modify its behavior. Many of
these are set indirectly by the options available above. However,
hard-core usage may require some users to add additional options to
fine-tune the result. Use **-S** to examine the actual command used, and
add custom options via one or more instances of the **-C** option. For
instance, to turn on image interpolation for all images, improving image
quality for scaled images at the expense of speed, use
**-C**-dDOINTERPOLATE. See www.ghostscript.com for complete
documentation.

`See Also <#toc10>`_
--------------------

`gmt <gmt.html>`_
