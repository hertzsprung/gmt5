**********
grdspotter
**********

grdspotter - Create CVA image from a gravity or topography grid

Synopsis
--------

.. include:: ../../common_SYN_OPTs.rst_

**grdspotter** [*grdfile*] **-E**\ *rotfile* **-G**\ *CVAgrid*
|SYN_OPT-I|
|SYN_OPT-R|
[ **-A**\ *agegrid* ]
[ **-D**\ *DIgrid* ] [ **-L**\ *IDgrid* ] [ **-M** ]
[ **-N**\ *upper_age* ] [ **-P**\ *PAgrid* ] [ **-Q**\ *IDinfo* ] [
**-S** ] [ **-T**\ **t**\ \|\ **u**\ *fixed_val* ] [
[ |SYN_OPT-V| ]
[ **-W**\ *n\_try* ]] [ **-Z**\ *z_min*\ [/*z_max*\ [/*z_inc*]] ] [
**-r** ]

|No-spaces|

Description
-----------

**grdspotter** reads a grid file with residual bathymetry or gravity and
calculates flowlines from each node that exceeds a minimum value using
the specified rotations file. These flowlines are then convolved with
the volume of the prism represented by each grid node and added up to
give a Cumulative Volcano Amplitude grid (CVA).

Required Arguments
------------------

*grdfile*
    Data grid to be processed, typically residual bathymetry or free-air anomalies.

**-E**\ *rotfile*
    Give file with rotation parameters. This file must contain one
    record for each rotation; each record must be of the following
    format:

    *lon lat tstart [tstop] angle* [ *khat a b c d e f g df* ]

    where *tstart* and *tstop* are in Myr and *lon lat angle* are in
    degrees. *tstart* and *tstop* are the ages of the old and young ends
    of a stage. If *tstop* is not present in the record then a total
    reconstruction rotation is expected and *tstop* is implicitly set to
    0 and should not be specified for any of the records in the file. If
    a covariance matrix **C** for the rotation is available it must be
    specified in a format using the nine optional terms listed in
    brackets. Here, **C** = (*g*/*khat*)\*[ *a b d; b c e; d e f* ]
    which shows **C** made up of three row vectors. If the degrees of
    freedom (*df*) in fitting the rotation is 0 or not given it is set
    to 10000. Blank lines and records whose first column contains # will
    be ignored. You may prepend a leading + to the filename to indicate
    you wish to invert the rotations.
    Alternatively, give the filename composed of two plate IDs
    separated by a hyphen (e.g., PAC-MBL) and we will instead extract
    that rotation from the GPlates rotation database. We return an error
    if the rotation cannot be found.

**-G**
    Specify name for output CVA grid file.

.. include:: ../../explain_-I.rst_

.. |Add_-Rgeo| unicode:: 0x20 .. just an invisible code
.. include:: ../../explain_-Rgeo.rst_

Optional Arguments
------------------

**-A**\ *agegrid*
    Supply a crustal age grid that is co-registered with the input data
    grid. These ages become the upper ages to use when constructing
    flowlines [Default extend flowlines back to oldest age found in the
    rotation file; but see **-N**].

**-D**\ *DIgrid*
    Use flowlines to determine the maximum CVA encountered along each
    flowline and create a Data Importance (DI) grid with these values at
    the originating nodes.

**-L**\ *IDgrid*
    Supply a co-registered grid with seamount chain IDs for each node.
    This option requires that you also use **-Q**.

**-M**
    Do not attempt to keep all flowlines in memory when using **-D**
    and/or **-P**. Should you run out of memory you can use this option
    to compute flowlines on-the-fly. It will be slower as we no longer
    can reuse the flowlines calculated for the CVA step. Cannot be used
    with **-W** or the multi-slice mode in **-Z**.

**-N**\ *upper_age*
    Set the upper age to assign to nodes whose crustal age is unknown
    (i.e., NaN) [no upper age]. Also see **-A**.

**-P**\ *PAgrid*
    Use flowlines to determine the flowline age at the CVA maximum for
    each node and create a Predicted Age (PA) grid with these values at
    the originating nodes.

**-Q**\ *IDinfo*
    Either `give (1) <give.html>`_ a single ID to use `or
    (2) <or.2.html>`_ the name of a file with a list of IDs to use
    [Default uses all IDs]. Each line would be TAG ID [w e s n]. The
    *w/e/s/n* zoom box is optional; if specified it means we only trace
    the flowline if inside this region [Default uses region set by
    **-R**]. Requires **-L**.

**-S**
    Normalize the resulting CVA grid to percentages of the CVA maximum.
    This also normalizes the DI grid (if requested).

**-T**\ **t**\ \|\ **u**\ *fixed_val*
    Selects ways to adjust ages; repeatable. Choose from **-Tt** to
    truncate crustal ages given via the **-A** option that exceed the
    upper age set with **-N** [no truncation], or **-Tu**\ *fixed_val*
    which means that after a node passes the test implied by **-Z**, we
    use this *fixed_val* instead in the calculations. [Default uses
    individual node values].

.. |Add_-V| unicode:: 0x20 .. just an invisible code
.. include:: ../../explain_-V.rst_

**-W**\ *n\_try*
    Get *n\_try* bootstrap estimates of the maximum CVA location; the
    longitude and latitude results are written to stdout [Default is no
    bootstrapping]. Cannot be used with **-M**.

**-Z**\ *z_min*\ [/*z_max*\ [/*z_inc*]]
    Ignore nodes with z-values lower than *z_min* [0] and optionally
    larger than *z_max* [Inf]. Give *z_min/z_max/z_inc* to make
    separate CVA grids for each *z*-slice [Default makes one CVA grid].
    Multi-slicing cannot be used with **-M**.

.. |Add_nodereg| unicode:: 0x20 .. just an invisible code
.. include:: ../../explain_nodereg.rst_

.. include:: ../../explain_help.rst_

Examples
--------

To create a CVA image from the Pacific topography grid
Pac_res_topo.nc, using the DC85.d Euler poles, and only output a grid
for the specified domain, run

   ::

    gmt grdspotter Pac_res_topo.nc -EDC85.d -GCVA.nc -R190/220/15/25 -I2m -N145 -Tt -V

This file can then be plotted with `grdimage <../../grdimage.html>`__.

See Also
--------

`GMT <GMT.html>`_, `grdimage <grdimage.html>`__,
`project <project.html>`_,
`mapproject <mapproject.html>`_,
`backtracker <backtracker.html>`_,
`hotspotter <hotspotter.html>`_,
`originator <originator.html>`_

References
----------

Wessel, P., 1999, "Hotspotting" tools released, EOS Trans. AGU, 80 (29), p. 319.

Wessel, P., 2008, Hotspotting: Principles and properties of a plate
tectonic Hough transform, Geochem. Geophys. Geosyst. 9(Q08004):
doi:10.1029/2008GC002058.
