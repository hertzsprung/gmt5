*********
grdredpol
*********

grdredpol - Compute the Continuous Reduction To the Pole, AKA differential RTP.

`Synopsis <#toc1>`_
-------------------

.. include:: ../../common_SYN_OPTs.rst_

**grdredpol** *anom\_grd* **-G**\ *rtp\_grd* [**-C**\ *<dec/dip>*]
[**-E**\ *<dec\_grd/dip\_grd>*] [**-F**\ *<m/n>*] [**-M**\ *<m\|r>*]
[**-N**\ ] [**-W**\ *<win\_width>*] [
**-R**\ *west*/*east*/*south*/*north*\ [**r**\ ] ] [ **-T<year>** ] [
**-Z<filter>** ] [ **-V**\ [*level*\ ] ]

`Description <#toc2>`_
----------------------

**grdredpol** will take a *.nc* file with a magnetic anomaly and compute
the reduction to the pole (RTP) anomaly. This anomaly is the one that
would have been produce if the bodies were magnetized vertically and the
anomalies were observed at the geomagnetic pole. Standard RTP procedure
assumes the direction of magnetization to be uniform throughout the
causative body, and the geomagnetic field to be uniform in direction
throughout the study region. Although these assumptions are reasonable
for small areas, they do not hold for large areas.

In the method used here computations are carried out in both the
frequency and the space domains. The idea is that a large area may be
decomposed in small size windows where both the ambient field and the
magnetization vector change by a very small amount. Inside each of those
windows, or bins, a set of filter coefficients are calculate and
reconstruct for each individual point the component filter using a first
order Taylor series expansion.

.. include:: ../../explain_commonitems.rst_

`Required Arguments <#toc4>`_
-----------------------------

*anom\_grd*
    The anomaly grid to be converted.
**-G**\ *rtp\_grd*
    is the filename for output grdfile with the RTP solution

`Optional Arguments <#toc5>`_
-----------------------------

**-C**\ *dec/dip*
    Use this (constant) declination and inclination angles for both
    field and magnetization. This option consists in the classical RTP
    procedure.
**-E**\ *dip\_grd/dec\_grd*
    Get magnetization DIP & DEC from these grids [default: use IGRF].
    Note that these two grids do not need to have the same resolution as
    the anomaly grid. Than can be coarser.
**-F**\ *m/n*
    The filter window size in terms of row/columns. The default value is 25x25.
**-M**\ *m\|r*
    Set boundary conditions. m\|r stands for mirror or replicate edges
    (Default is zero padding).
**-N**
    Do NOT use Taylor expansion.
**-R**\ *west*/*east*/*south*/*north*
    defines the Region of the output points. [Default: Same as input.]
**-Y**\ *year*
    Decimal year used by the IGRF routine to compute the declination and
    inclination at each point [default: 2000]
**-W**\ *width*
    The size of the moving window in degrees [5].
**-Z**\ *filter\_grd*
    Write the filter file on disk.

.. |Add_-V| unicode:: 0x20 .. just an invisible code
.. include:: ../../explain_-V.rst_

`Examples <#toc6>`_
-------------------

Suppose that *anom.grd* is a file with the magnetic anomaly reduced to
the 2010 epoch and that the *dec.grd* and *dip.grd* contain the
magnetization declination and inclination respectively for an area that
encloses that of the *anom.grd*, compute the *RTP* using bins of 2
degrees and a filter of 45 coefficients.

    grdredpol anom.grd -Grtp.grd -W2 -F45/45 -T2010 -Edec.grd/dip.grd -V

To compute the same *RTP* but now with the field and magnetization
vectors collinear and computed from IGRF :

    grdredpol anom.grd -Grtp.grd -W2 -F45/45 -T2010 -V

`Reference <#toc7>`_
--------------------

Luis, J.L. and Miranda, J.M. (2008), Reevaluation of magnetic chrons in
the North Atlantic between 35N and 47N: Implications for the formation
of the Azores Triple Junction and associated plateau. *JGR*, VOL.
**113**, B10105, doi:10.1029/2007JB005573
