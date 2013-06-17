*******
pspolar
*******

**pspolar** - Plot polarities on the inferior focal half-sphere on maps

Synopsis
--------

.. include:: ../../common_SYN_OPTs.rst_

pspolar [ *table* ] **-J**\ *parameters*
|SYN_OPT-R|
**-M**\ *size* **-S**\ *<symbol><size>*
[ |SYN_OPT-B| ]
[ **-C**\ *lon*/*lat*\ [/*dash\_width*/*pointsize*] ] [ **-F**\ *color* ]
[ **-G**\ *fill* ] [ **-K** ] [ **-L** ] [ **-N** ]
[ **-O** ] [ **-Q**\ *mode*\ [*args*] ]
[ **-T**\ *angle*/*form*/*justify*/*fontsize* ]
[ |SYN_OPT-U| ]
[ |SYN_OPT-V| ]
[ **-W**\ *pen* ]
[ |SYN_OPT-X| ]
[ |SYN_OPT-Y| ]
[ |SYN_OPT-c| ]
[ |SYN_OPT-h| ]
[ |SYN_OPT-i| ]
[ |SYN_OPT-:| ]

|No-spaces|

Description
-----------

**pspolar** reads data values from *files* [or standard input] and
generates *PostScript* code that will plot stations on focal mechanisms
on a map. The *PostScript* code is written to standard output.

Parameters are expected to be in the following columns:

    **1**,\ **2**,\ **3**:
        station\_code, azimuth, take-off angle
    **4**:
        polarity:

        - compression can be c,C,u,U,+

        - rarefaction can be d,D,r,R,-

        - not defined is anything else

Required Arguments
------------------

.. |Add_intables| unicode:: 0x20 .. just an invisible code
.. include:: ../../explain_intables.rst_

.. include:: ../../explain_-J.rst_

.. |Add_-Rgeo| unicode:: 0x20 .. just an invisible code
.. include:: ../../explain_-Rgeo.rst_


**-D**\ longitude/latitude)
    Maps the bubble at given longitude and latitude point.

**-M**\ size)
    Sets the size of the beach ball to plot polarities in. *Size* is in
    inch (unless **c**, **i**, **m**, or **p** is appended).

**-S**\ <symbol\_type><size>)
    Selects *symbol\_type* and symbol *size*. Size is in inch (unless
    **c**, **i**, **m**, or **p** is appended). Choose symbol type from
    st(\ *a*)r, (*c*)ircle, (*d*)iamond, (*h*)exagon, (*i*)nverted
    triangle, (*p*)oint, (*s*)quare, (*t*)riangle, (*x*)cross.

Optional Arguments
------------------

.. include:: ../../explain_-B.rst_

**-C**
    Offsets focal mechanisms to the latitude and longitude specified in
    the last two columns of the input file.

**-E**\ fill)
    Selects filling of symbols for stations in extensive quadrants. Set
    the color [Default is 250]. If **-E**\ *fill* is the same as
    **-F**\ *fill*, use **-e** to outline.

**-F**\ *fill*
    Sets background color of the beach ball. Default is no fill.

**-G**\ *fill*
    Selects filling of symbols for stations in compressional quadrants.
    Set the color [Default is black].

.. include:: ../../explain_-K.rst_

**-N**
    Does **not** skip symbols that fall outside map border [Default
    plots points inside border only].

.. include:: ../../explain_-O.rst_
.. include:: ../../explain_-P.rst_

**-Q**\ *mode*\ [*args*]
    Sets one or more attributes; repeatable. The various combinations are

**-Qe**\ [pen])
    Outline symbols in extensive quadrants using *pen* or the default
    pen (see **-W**).

**-Qf**\ [pen])
    Outline the beach ball using *pen* or the default pen (see **-W**).

**-Qg**\ [pen])
    Outline symbols in compressional quadrants using *pen* or the
    default pen (see **-W**).

**-Qh**
    Use special format derived from HYPO71 output

**-Qs**\ *half-size*/[**V**\ [*v\_width/h\_length/h\_width/shape*\ ]][\ **G**\ *color*][**L**\ ]
    Plots S polarity azimuth. S polarity is in last column. It may be a
    vector (**V** option) or a segment. Give
    half-size,v\_width,h\_length,h\_width in inch (unless **c**, **i**,
    **m**, or **p** is appended). [**L**\ ] option is for outline.

**-Qt**\ *pen*
    Set pen color to write station code. Default uses the default pen
    (see **-W**).


**-T**\ *angle/form/justify/fontsize in points*
    To write station code. [Default is 0.0/0/5/12].

.. include:: ../../explain_-U.rst_

.. |Add_-V| unicode:: 0x20 .. just an invisible code
.. include:: ../../explain_-V.rst_

**-W**\ *pen*
    Set current pen attributes [Defaults: width = default, color = black, style = solid].

.. include:: ../../explain_-XY.rst_
.. include:: ../../explain_-c.rst_
.. include:: ../../explain_-icols.rst_
.. include:: ../../explain_colon.rst_
.. include:: ../../explain_help.rst_

Examples
--------

   ::

    gmt pspolar -R239/240/34/35.2 -JM8c -N -Sc0.4 -h1 -D39.5/34.5 -M5 << END > test.ps
    #stat azim ih pol
    0481 11 147 c
    6185 247 120 d
    0485 288 114 +
    0490 223 112 -
    0487 212 109 .
    END

or

   ::

    gmt pspolar -R239/240/34/35.2 -JM8c -N -Sc0.4 -h1 -D239.5/34.5 -M5 <<END > test.ps
    #Date Or. time stat azim ih
    910223 1 22 0481 11 147 ipu0
    910223 1 22 6185 247 120 ipd0
    910223 1 22 0485 288 114 epu0
    910223 1 22 0490 223 112 epd0
    910223 1 22 0487 212 109 epu0
    END

See Also
--------

`GMT <GMT.html>`_, `psbasemap <psbasemap.html>`_, `psxy <psxy.html>`_

References
----------

Bomford, G., Geodesy, 4th ed., Oxford University Press, 1980.

Aki, K. and P. Richards, Quantitative Seismology, Freeman, 1980.

Authors
-------

Genevieve Patau Seismology Dept. Institut de Physique du Globe de Paris
(patau@ipgp.jussieu.fr)
