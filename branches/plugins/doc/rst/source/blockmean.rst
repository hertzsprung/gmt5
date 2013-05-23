*********
blockmean
*********

blockmean - Block average (*x*,\ *y*,\ *z*) data tables by L2 norm

Synopsis
--------

.. include:: common_SYN_OPTs.rst_

**blockmean** [ *table* ]
|SYN_OPT-I|
|SYN_OPT-R| [ **-C** ]
[ **-E**\ [**p**] ] [ **-S**\ [**m**\ \|\ **n**\ \|\ **s**\ \|\ **w**] ]
[ |SYN_OPT-V| ] [ **-W**\ [**i**\ \|\ **o**] ]
[ |SYN_OPT-b| ]
[ |SYN_OPT-f| ] [ |SYN_OPT-h| ]
[ |SYN_OPT-i| ]
[ |SYN_OPT-o| ] [ **-r** ] [ |SYN_OPT-:| ]

|No-spaces|

Description
-----------

**blockmean** reads arbitrarily located (*x*,\ *y*,\ *z*) triples [or
optionally weighted quadruples (*x*,\ *y*,\ *z*,\ *w*)] from standard
input [or *table*] and writes to standard output a mean position and
value for every non-empty block in a grid region defined by the **-R**
and **-I** arguments. Either **blockmean**, `blockmedian <blockmedian.html>`_, or
`blockmode <blockmode.html>`_ should be used as a pre-processor before running
`surface <surface.html>`_ to avoid aliasing short wavelengths. These routines are also
generally useful for decimating or averaging (*x*,\ *y*,\ *z*) data. You
can modify the precision of the output format by editing the
**FORMAT_FLOAT_OUT** parameter in your `gmt.conf <gmt.conf.html>`_ file, or you may
choose binary input and/or output to avoid loss of precision.

Required Arguments
------------------

.. include:: explain_-I.rst_

.. |Add_-R| unicode:: 0x20 .. just an invisible code
.. include:: explain_-R.rst_

Optional Arguments
------------------

*table*
    3 [or 4, see **-W**] column ASCII data table file(s) [or binary, see
    **-bi**\ [*ncols*][*type*]] holding (*x*,\ *y*,\ *z*\ [,\ *w*])
    data values. [\ *w*] is an optional weight for the data. If no file
    is specified, **blockmean** will read from standard input.
**-C**
    Use the center of the block as the output location [Default uses the
    mean location].
**-E**\ [**p**]
    Provide Extended report which includes **s** (the standard deviation
    about the mean), **l**, the lowest value, and **h**, the high value
    for each block. Output order becomes
    *x*,\ *y*,\ *z*,\ *s*,\ *l*,\ *h*\ [,\ *w*]. [Default outputs
    *x*,\ *y*,\ *z*\ [,\ *w*]. See **-W** for *w* output.
    If **-Ep** is used we assume weights are 1/(sigma squared) and *s*
    becomes the propagated error of the mean.
**-S**\ [**m**\ \|\ **n**\ \|\ **s**\ \|\ **w**]
    Use **-Sn** to report the number of points inside each block,
    **-Ss** to report the sum of all *z*-values inside a block, **-Sw**
    to report the sum of weights [Default (or **-Sm** reports mean value]. 

.. |Add_-V| unicode:: 0x20 .. just an invisible code
.. include:: explain_-V.rst_

**-W**\ [**i**\ \|\ **o**]
    Weighted modifier[s]. Unweighted input and output has 3 columns
    *x*,\ *y*,\ *z*; Weighted i/o has 4 columns *x*,\ *y*,\ *z*,\ *w*.
    Weights can be used in input to construct weighted mean values in
    blocks. Weight sums can be reported in output for later combining
    several runs, etc. Use **-W** for weighted i/o, **-Wi** for weighted
    input only, **-Wo** for weighted output only. [Default uses
    unweighted i/o]. 

.. |Add_-bi| replace:: [Default is 3 (or 4 if **-Wi** is set)].
.. include:: explain_-bi.rst_
   
 
.. |Add_-bo| replace:: [Default is 3 (or 4 if **-Wo** is set)]. **-E** adds 3 additional columns.
.. include:: explain_-bo.rst_
 
.. |Add_-f| unicode:: 0x20 .. just an invisible code
.. include:: explain_-f.rst_

.. |Add_-h| unicode:: 0x20 .. just an invisible code
.. include:: explain_-h.rst_

.. include:: explain_-icols.rst_
.. include:: explain_-ocols.rst_

.. |Add_nodereg| replace:: 
    Each block is the locus of points nearest the grid value location. For example, with
    **-R**\ 10/15/10/15 and **-I**\ 1: with the **-r** option 10 <=
    (*x*,\ *y*) < 11 is one of 25 blocks; without it 9.5 <= (*x*,\ *y*)
    < 10.5 is one of 36 blocks.
.. include:: explain_nodereg.rst_

.. include:: explain_colon.rst_
.. include:: explain_help.rst_
.. include:: explain_precision.rst_

Examples
--------

To find 5 by 5 minute block mean values from the ASCII data in hawaii.xyg, run

   ::

    blockmean hawaii.xyg -R198/208/18/25 -I5m > hawaii_5x5.xyg

See Also
--------

`blockmedian <blockmedian.html>`_,
`blockmode <blockmode.html>`_, `gmt <gmt.html>`_,
`gmt.conf <gmt.conf.html>`_,
`nearneighbor <nearneighbor.html>`_,
`surface <surface.html>`_,
`triangulate <triangulate.html>`_
