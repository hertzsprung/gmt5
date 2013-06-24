**********
x2sys_list
**********

x2sys_list - Extract subset from crossover data base

Synopsis
--------

.. include:: ../../common_SYN_OPTs.rst_

**x2sys_list** **-C**\ *column* **-T**\ *TAG* [ *coedbase.txt* ]
[ **-A**\ *asymm_max* ] [ **-E** ] [ **-F**\ *acdhiInNtTvwxyz* ]
[ **-I**\ [*list*] ] [ **-L**\ [*corrtable*] ] [ **-N**\ *nx_min* ]
[ **-Qe**\ \|\ **i** ]
[ |SYN_OPT-R| ]
[ **-S**\ *track* ]
[ |SYN_OPT-V| ]
[ **-W**\ [*list*] ]
[ |SYN_OPT-bo| ]

|No-spaces|

Description
-----------

**x2sys_list** will read the crossover ASCII data base *coedbase.txt*
(or *stdin*) and extract a subset of the crossovers based on the other
arguments. The output may be ASCII or binary.

Required Arguments
------------------

**-C**\ *column*
    Specify which data column you want to process. Crossovers related to
    this column name must be present in the crossover data base.

.. include:: explain_tag.rst_

Optional Arguments
------------------

*coedbase.txt*
    The name of the input ASCII crossover error data base as produced by
    **x2sys_cross**. If not given we read standard input instead.
**-A**\ *asymm_max*
    Specifies maximum asymmetry in the distribution of crossovers
    relative to the mid point in time (or distance, if not time is
    available). Asymmetry is computed as (n_right - n_left)/(n_right
    + n_left), referring the the number of crossovers that falls in the
    left or right half of the range. Symmetric distributions will have
    values close to zero. If specified, we exclude tracks whose
    asymmetry exceeds the specify cutoff in absolute value [1, i.e.,
    include all].
**-E**
    Enhance ASCII output by writing GMT segment headers with name of the
    two tracks and their total number of cross-overs [no segment
    headers].
**-F**\ *acdhiInNtTvwxyz*
    Specify your desired output using any combination of
    *acdhiInNtTvwxyz*, in any order. Do not use space between the
    letters, and note your selection is case-sensitive. The output will
    be ASCII (or binary, **-bo**) columns of
    values. Description of codes: **a** is the angle (< 90) defined by
    the crossing tracks, **c** is crossover value of chosen observation
    (see **-C**), **d** is distance along track, **h** is heading along
    track, **i** is the signed time interval between the visit at the
    crossover of the two tracks involved, **I** is same as **i** but is
    unsigned, **n** is the names of the two tracks, **N** is the id
    numbers of the two tracks, **t** is time along track in
    *date*\ **T**\ *clock* format (NaN if not available), **T** is
    elapsed time since start of track along track (NaN if not
    available), **v** is speed along track, **w** is the composite
    weight, **x** is *x*-coordinate (or longitude), **y** is
    *y*-coordinate (or latitude), and **z** is observed value (see
    **-C**) along track. If **-S** is not specified then
    **d**,\ **h**,\ **n**,\ **N**,\ **t**,\ **T**,\ **v** results in two
    output columns each: first for track one and next for track two (in
    lexical order of track names); otherwise, they refer to the
    specified track only (except for **n**,\ **N** which then refers to
    the other track). The sign convention for **c**,\ **i** is track one
    minus track two (lexically sorted). Time intervals will be returned
    according to the **TIME_UNIT** GMT defaults setting.
**-I**\ [*list*\ ]
    Name of ASCII file with a list of track names (one per record) that
    should be excluded from consideration [Default includes all tracks].
**-L**\ [*corrtable*\ ]
    Apply optimal corrections to the chosen observable. Append the
    correction table to use [Default uses the correction table
    *TAG*\ \_corrections.txt which is expected to reside in the
    **$X2SYS_HOME**/*TAG* directory]. For the format of this file, see
    **x2sys_solve**.
**-N**\ *nx_min*
    Only report data from pairs that generated at least *nx_min*
    crossovers between them [use all pairs].
**-Qe**\ \|\ **i**
    Append **e** for external crossovers or **i** for internal
    crossovers only [Default is all crossovers].

.. |Add_-Rgeo| replace:: For Cartesian
    data just give *xmin/xmax/ymin/ymax*. This option bases the
    statistics on those COE that fall inside the specified domain.
.. include:: ../../explain_-Rgeo.rst_

**-S**\ *track*
    Name of a single track. If given we restrict output to those
    crossovers involving this track [Default output is crossovers
    involving any track pair].

.. |Add_-V| unicode:: 0x20 .. just an invisible code
.. include:: ../../explain_-V.rst_

**-W**\ [*list*\ ]
    Name of ASCII file with a list of track names and their relative
    weights (one track per record) that should be used to calculate the
    composite crossover weight (output code **w** above). [Default sets
    weights to 1].

.. |Add_-bo| unicode:: 0x20 .. just an invisible code
.. include:: ../../explain_-bo.rst_

.. include:: ../../explain_help.rst_

Examples
--------

To find all the magnetic crossovers associated with the tag MGD77 from
the file COE_data.txt, restricted to occupy a certain region in the
south Pacific, and return location, time, and crossover value, try

   ::

    gmt x2sys_list COE_data.txt -V -TMGD77 -R180/240/-60/-30 -Cmag -Fxytz > mag_coe.txt

To find all the faa crossovers globally that involves track 12345678 and
output time since start of the year, using a binary double precision
format, try

   ::

    gmt x2sys_list COE_data.txt -V -TMGD77 -Cfaa -S12345678 -FTz -bod > faa_coe.b

See Also
--------

`x2sys_binlist <x2sys_binlist.html>`_,
`x2sys_cross <x2sys_cross.html>`_,
`x2sys_datalist <x2sys_datalist.html>`_,
`x2sys_get <x2sys_get.html>`_,
`x2sys_init <x2sys_init.html>`_,
`x2sys_put <x2sys_put.html>`_,
`x2sys_report <x2sys_report.html>`_,
`x2sys_solve <x2sys_solve.html>`_
