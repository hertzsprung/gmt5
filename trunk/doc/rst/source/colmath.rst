*******
colmath
*******

colmath - Do mathematics on columns from data tables

`Synopsis <#toc1>`_
-------------------

**colmath** [ *table* ] [ **-A** ] [ **-N** ] [ **-Q**\ *seg* ] [
**-S**\ [**~**\ ]\ *"search string"* ] [ **-T** ] [ **-V**\ [*level*\ ]
] [ **-b**\ [*ncol*\ ][**t**\ ][\ **+L**\ \|\ **+B**] ] [
**-f**\ [**i**\ \|\ **o**]\ *colinfo* ] [
**-g**\ [**a**\ ]\ **x**\ \|\ **y**\ \|\ **d**\ \|\ **X**\ \|\ **Y**\ \|\ **D**\ \|[*col*\ ]\ **z**\ [+\|-]\ *gap*\ [**u**\ ]
] [ **-h**\ [**i**\ \|\ **o**][*n*\ ] ] [
**-i**\ *cols*\ [**l**\ ][\ **s**\ *scale*][\ **o**\ *offset*][,\ *...*]
] [ **-o**\ *cols*\ [,*...*] ] [ **-s**\ [*cols*\ ][\ **a**\ \|\ **r**]
] [ **-:**\ [**i**\ \|\ **o**] ]

`Description <#toc2>`_
----------------------

**colmath** reads its standard input [or inputfiles] does mathematics in
RPN on the columns and then writes the result to standard output. It can
do a combination of four tasks: (1) convert between binary and ASCII
data tables, (2) paste corresponding records from multiple files
horizontally into a single file, (3) extract a subset of the available
columns, (4) do mathematics on the columns. Input (and hence output) may
have multiple sub-headers, and ASCII tables may have regular headers as
well. 

.. include:: explain_commonitems.rst_

`Required Arguments <#toc4>`_
-----------------------------

`Optional Arguments <#toc5>`_
-----------------------------

.. |Add_intables| unicode:: 0x20 .. just an invisible code
.. include:: explain_intables.rst_

**-A**
    The records from the input files should be pasted horizontally, not
    appended vertically [Default]. All files must have the same number
    of segments and number of rows per segment. Note for binary input,
    all the files you want to paste must have the same number of columns
    (as set with **-bi**\ [*ncols*\ ][*type*\ ]); ascii tables can have
    different number of columns.
**-N**
    Do not write records that only contain NaNs in every field [Default
    writes all records].
**-Q**\ *seg*
    Only write segment number *seg* and skip all others. Cannot be used
    with **-S**.
**-S**\ [**~**\ ]\ *"search string"*
    Only output those segments whose header record contains the
    specified text string. To reverse the search, i.e., to output
    segments whose headers do *not* contain the specified pattern, use
    **-S~**. Should your pattern happen to start with ~ you need to
    escape this character with a backslash  [Default output all
    segments]. Cannot be used with **-Q**.
**-T**
    Suppress the writing of segment headers on output. 

.. |Add_-V| unicode:: 0x20 .. just an invisible code
.. include:: explain_-V.rst_

.. |Add_-bi| unicode:: 0x20 .. just an invisible code
.. include:: explain_-bi.rst_

.. |Add_-bo| replace:: [Default is same as input].
.. include:: explain_-bo.rst_

.. |Add_-f| unicode:: 0x20 .. just an invisible code
.. include:: explain_-f.rst_

.. |Add_-g| unicode:: 0x20 .. just an invisible code
.. include:: explain_-g.rst_

.. |Add_-h| unicode:: 0x20 .. just an invisible code
.. include:: explain_-h.rst_

.. include:: explain_-icols.rst_

.. include:: explain_-ocols.rst_

.. include:: explain_-s.rst_

.. include:: explain_colon.rst_

.. include:: explain_help.rst_

.. include:: explain_precision.rst_

`Examples <#toc7>`_
-------------------

To convert the binary file test.b (single precision) with 4 columns to ASCII:

colmath test.b -bi4f > test.dat

To convert the multiple segment ASCII table test.d to a double precision binary file:

colmath test.d -bo > test.b

You have an ASCII table with 6 columns and you want to plot column 5
versus column 0. Try

colmath table.d -o5,0 \| psxy ...

If the file instead is the binary file results.b which has 9
single-precision values per record, we extract the last column and
columns 4-6 and write ASCII with the command

colmath results.b -o8,4-6 -bi9s \| psxy ...

You want to plot the 2nd column of a 2-column file left.d versus the
first column of a file right.d:

colmath left.d right.d -A -o1,2 \| psxy ...

To extract all segments in the file big\_file.d whose headers contain
the string "RIDGE AXIS", try

colmath big\_file.d -S"RIDGE AXIS" > subset.d

`See Also <#toc8>`_
-------------------

`gmt <gmt.html>`_ , `minmax <minmax.html>`_
