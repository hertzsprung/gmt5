*********
grdhisteq
*********

grdhisteq - Perform histogram equalization for a grid

`Synopsis <#toc1>`_
-------------------

.. include:: common_SYN_OPTs.rst_

**grdhisteq** *in\_grdfile* [ **-G**\ *out\_grdfile* ] [
**-C**\ *n\_cells* ] [ **-D**\ [*file*\ ] ] [ **-N**\ [*norm*\ ] ] [
**-Q** ] [ **-R**\ *west*/*east*/*south*/*north*\ [**r**\ ] ] [
**-V**\ [*level*\ ] ]

|No-spaces|

`Description <#toc2>`_
----------------------

**grdhisteq** allows the user to find the data values which divide a
given grid file into patches of equal area. One common use of
**grdhisteq** is in a kind of histogram equalization of an image. In
this application, the user might have a grid of flat topography with a
mountain in the middle. Ordinary gray shading of this file (using
grdimage/grdview) with a linear mapping from topography to graytone will
result in most of the image being very dark gray, with the mountain
being almost white. One could use **grdhisteq** to write to stdout an
ASCII list of those data values which divide the range of the data into
*n\_cells* segments, each of which has an equal area in the image. Using
**awk** or **makecpt** one can take this output and build a cpt file;
using the cptfile with grdimage will result in an image with all levels
of gray occurring equally. Alternatively, see **grd2cpt**.

The second common use of **grdhisteq** is in writing a grid with
statistics based on some kind of cumulative distribution function. In
this application, the output has relative highs and lows in the same
(x,y) locations as the input file, but the values are changed to reflect
their place in some cumulative distribution. One example would be to
find the lowest 10% of the data: Take a grid, run **grdhisteq** and make
a grid using *n\_cells* = 10, and then contour the result to trace the 1
contour. This will enclose the lowest 10% of the data, regardless of
their original values. Another example is in equalizing the output of
**grdgradient**. For shading purposes it is desired that the data have a
smooth distribution, such as a gaussian. If you run **grdhisteq** on
output from **grdgradient** and make a grid file output with the
Gaussian option, you will have a grid whose values are distributed
according to a gaussian distribution with zero mean and unit variance.
The locations of these values will correspond to the locations of the
input; that is, the most negative output value will be in the (x,y)
location of the most negative input value, and so on. 

`Required Arguments <#toc4>`_
-----------------------------

*in_grdfile*
    2-D binary grid file to be equalized. (See GRID FILE FORMATS below).

`Optional Arguments <#toc5>`_
-----------------------------

**-C**\ *n\_cells*
    Sets how many cells (or divisions) of data range to make.
**-D**
    Dump level information to *file*, or standard output if no file is
    provided.
**-G**\ *out\_grdfile*
    Name of output 2-D grid file. Used with **-N** only. (See GRID FILE
    FORMATS below).
**-N**\ [*norm*\ ]
    Gaussian output. Use with **-G** to make an output grid with
    standard normal scores. Append *norm* to force the scores to fall in
    the <-1,+1> range [Default is standard normal scores].
**-Q**
    Use quadratic intensity scaling. [Default is linear]. 

.. |Add_-R| replace:: Using the **-R** option
    will select a subsection of *in\_grdfile* grid. If this subsection
    exceeds the boundaries of the grid, only the common region will be extracted. 
.. include:: explain_-R.rst_

.. |Add_-V| unicode:: 0x20 .. just an invisible code
.. include:: explain_-V.rst_

.. include:: explain_help.rst_

.. include:: explain_grd_inout.rst_

`Examples <#toc7>`_
-------------------

To find the height intervals that divide the file heights.nc into 16
divisions of equal area:

    grdhisteq heights.nc -C16 -D > levels.d

To make the poorly distributed intensities in the file raw\_intens.nc
suitable for use with **grdimage** or **grdview**, run

    grdhisteq raw\_intens.nc -Gsmooth\_intens.nc -N -V

`Restrictions <#toc8>`_
-----------------------

If you use **grdhisteq** to make a gaussian output for gradient shading
in **grdimage** or **grdview**, you should be aware of the following:
the output will be in the range [-x, x], where x is based on the number
of data in the input grid (nx \* ny) and the cumulative gaussian
distribution function F(x). That is, let N = nx \* ny. Then x will be
adjusted so that F(x) = (N - 1 + 0.5)/N. Since about 68% of the values
from a standard normal distribution fall within +/- 1, this will be true
of the output grid. But if N is very large, it is possible for x to be
greater than 4. Therefore, with the **grdimage** program clipping
gradients to the range [-1, 1], you will get correct shading of 68% of
your data, while 16% of them will be clipped to -1 and 16% of them
clipped to +1. If this makes too much of the image too light or too
dark, you should take the output of **grdhisteq** and rescale it using
**grdmath** and multiplying by something less than 1.0, to shrink the
range of the values, thus bringing more than 68% of the image into the
range [-1, 1]. Alternatively, supply a normalization factor with **-N**.

`See Also <#toc9>`_
-------------------

`gmt <gmt.html>`_, `gmt.conf <gmt.conf.html>`_,
`grd2cpt <grd2cpt.html>`_,
`grdgradient <grdgradient.html>`_,
`grdimage <grdimage.html>`_, `grdmath <grdmath.html>`_,
`grdview <grdview.html>`_, `makecpt <makecpt.html>`_
