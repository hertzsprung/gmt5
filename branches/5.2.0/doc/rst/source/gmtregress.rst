.. index:: ! gmtregress

**********
gmtregress
**********

.. only:: not man

    gmtregress - Linear regression of 1-D data sets

Synopsis
--------

.. include:: common_SYN_OPTs.rst_

**gmtregress** [ *table* ] [ **-A**\ *min*\ /*max*\ /*inc* ] [ **-C**\ *level* ]
[ **Ex**\ \|\ **y**\ \|\ **o**\ \|\ **r** ]
[ **-F**\ *flags* ] [ **N1**\ \|\ **2**\ \|\ **r**\ \|\ **w** ]  [ **-S**\ [**r**] ]
[ **-T**\ *min*\ /*max*\ /*inc* \| **T**\ *n* ] [ **-W**\ [**w**]\ [**x**]\ [**y**]\ [**r**] ]
[ |SYN_OPT-V| ]
[ |SYN_OPT-a| ]
[ |SYN_OPT-b| ]
[ |SYN_OPT-g| ]
[ |SYN_OPT-h| ]
[ |SYN_OPT-i| ]
[ |SYN_OPT-o| ]

|No-spaces|

Description
-----------

**gmtregress** reads one or more data tables [or *stdin*]
and determines the best linear regression model *y* = *a* + *b*\ \* *x* for each segment using the chosen parameters.
The user may specify which data and model components should be reported.  By default, the model will be evaluated at the
input points, but alternatively you can specify an equidistant range over which to evaluate
the model, or turn of evaluation completely.  Instead of determining the best fit we can
perform a scan of all possible regression lines
(for a range of slope angles) and examine how the chosen misfit measure varies with slope.
This is particularly useful when analyzing data with many outliers.  Note: If you
actually need to work with log10 of *x* or *y* you can accomplish that transformation via **-i**.


Required Arguments
------------------

None

Optional Arguments
------------------

.. |Add_intables| unicode:: 0x20 .. just an invisible code
.. include:: explain_intables.rst_
   The first two columns are expected to contain the required *x* and *y* data.  Depending on
   your **-W** and **-E** settings we may expect an additional 1-3 columns with error estimates
   of one of both of the data coordinates, and even their correlation.

**-A**\ *min*\ /*max*\ /*inc*
    Instead of determining a best-fit regression we explore the full range of regressions.
    Examine all possible regression lines with slope angles between *min* and *max*,
    using steps of *inc* degrees [-90/+90/1].  For each slope the optimum intercept
    is determined based on your regression type (**-E**) and misfit norm (**-N**) settings.  
    For each segment we report the four columns *angle*, *E*, *slope*, *intercept*, for
    the range of specified angles. The best model parameters within this range 
    are written into the segment header and reported in verbose mode (**-V**).

**-C**\ *level*
    Set the confidence level (in %) to use for the optional calculation of confidence bands
    on the regression [95].  This is only used if **-F** includes the output column **c**.

**Ex**\ \|\ **y**\ \|\ **o**\ \|\ **r**
    Type of linear regression, i.e., select the type of misfit we should calculate.
    Choose from **x** (regress *x* on *y*; i.e., the misfit is measured horizontally from data point to regression line),
    **y** (regress *y* on *x*; i.e., the misfit is measured vertically [Default]), **o** (orthogonal regression;
    i.e., the misfit is measured from data point orthogonally to nearest point on the line), or **r** (Reduced Major
    Axis regression; i.e., the misfit is the product of both vertical and horizontal misfits) [**y**].

**-F**\ *flags*
    Append a combination of the columns you wish returned; the output order will match the order specified.  Choose from
    **x** (observed *x*), **y** (observed *y*), **m** (model prediction), **r** (residual = data minus model),
    **c** (symmetrical confidence interval on the regression; see **-C**
    for specifying the level), **z** (standardized residuals or so-called *z-scores*) and **w** (outlier weights 0 or 1, for
    **-Nw** these are the Reweighted Least Squares weights) [**xymrczw**].

**N1**\ \|\ **2**\ \|\ **r**\ \|\ **w**
    Selects the norm to use for the misfit calculation.  Choose among **1** (L-1 measure; the mean of the
    absolute residuals), **2** (Least-squares; the mean of the squared residuals), 
    **r** (LMS; The least median of the squared residuals), or **w** (RLS; reweighted least squares: the
    mean of the squared residuals after outliers identify via LMS have been removed) [Default is **2**].
    Traditional regression uses L-2 while L-1 and in particular LMS are more robust in how they handle outliers.
    As alluded to, RLS implies an initial LMS regression which is then used to identify outliers in the data,
    assign these a zero weight, and then redo the regression using a L-2 norm.

**-S**\ [**r**]
    Restricts which records will be output.  By default all data records will be output in the format specified
    by **-F**.  Use **-S** to exclude data points identified as outliers by the regression.  Alternatively,
    use **-Sr** to reverse this and only output the outlier records.

**-T**\ *min*\ /*max*\ /*inc* \| **T**\ *n*
    Evaluate the best-fit regression model at the equidistant points implied by the arguments.  If
    **-T**\ *n* is given instead we will reset *min* and *max* to the extreme *x*-values for each segment and determine *inc*
    so that there are exactly *n* output values for each segment.  To skip the model evaluation entirely, simply provide **-T**\ 0.

**-W**\ [**w**]\ [**x**]\ [**y**]\ [**r**]
    Specifies weighted regression and which weights will be provided.
    Append **x** if giving 1-sigma uncertainties in the *x*-observations, **y** if giving 1-sigma uncertainties in *y*, and
    **r** if giving correlations between *x* and *y* observations, in the order these columns appear in the input (after the
    two required and leading *x*, *y* columns).
    Giving both **x** and **y** (and optionally **r**) implies an orthogonal regression, otherwise giving
    **x** requires **-Ex** and **y** requires **-Ey**.
    We convert uncertainties in *x* and *y* to regression weights via the relationship weight = 1/sigma.
    Use **-Ww** if the we should interpret the input columns to have precomputed weights instead.  Note: residuals
    with respect to the regression line will be scaled by the given weights.  Most norms will then square this weighted
    residual (**-N1** is the only exception).

.. |Add_-V| unicode:: 0x20 .. just an invisible code
.. include:: explain_-V.rst_

.. include:: explain_-aspatial.rst_

.. |Add_-bi| unicode:: 0x20 .. just an invisible code
.. include:: explain_-bi.rst_

.. |Add_-bo| replace:: [Default is same as input].
.. include:: explain_-bo.rst_

.. |Add_-g| unicode:: 0x20 .. just an invisible code
.. include:: explain_-g.rst_

.. |Add_-h| unicode:: 0x20 .. just an invisible code
.. include:: explain_-h.rst_

.. include:: explain_-icols.rst_

.. include:: explain_-ocols.rst_

.. include:: explain_help.rst_

.. include:: explain_precision.rst_


Examples
--------

To do a standard least-squares regression on the *x-y* data in points.txt and return
x, y, and model prediction with 99% confidence intervals, try 

   ::

    gmt gmtregress points.txt -Fxymc -C99 > points_regressed.txt

To do a reweighted least-squares regression on the data rough.txt and return
x, y, model prediction and the RLS weights, try 

   ::

    gmt gmtregress rough.txt -Fxymw > points_regressed.txt

To do an orthogonal least-squares regression on the data crazy.txt but first take
the logarithm of both x and y, then return x, y, model prediction and the normalized
residuals (z-scores), try 

   ::

    gmt gmtregress crazy.txt -Eo -Fxymz -i0-1l > points_regressed.txt

To examine how the orthogonal LMS misfits vary with angle between 0 and 90
in steps of 0.2 degrees for the same file, try 

   ::

    gmtregress points.txt -A0/90/0.2 -Eo -Nr > points_analysis.txt


References
----------

Draper, N. R., and H. Smith, 1998, *Applied regression analysis*, 3rd ed., 736 pp.,
John Wiley and Sons, New York.

Rousseeuw, P. J., and A. M. Leroy, 1987, *Robust regression and outlier detection*,
329 pp., John Wiley and Sons, New York.

York, D., N. M. Evensen, M. L. Martinez, and J. De Basebe Delgado, 2004, Unified
equations for the slope, intercept, and standard errors of the best straight line,
*Am. J. Phys.*, 72(3), 367-375.

See Also
--------

:doc:`gmt`,
:doc:`trend1d`,
:doc:`trend2d`
