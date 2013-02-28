*******
testapi
*******

testapi - Test API i/o methods for any data type

`Synopsis <#toc1>`_
-------------------

**testapi**
**-I**\ **c**\ \|\ **d**\ \|\ **f**\ \|\ **r**\ \|\ **s**\ [/**m**\ \|\ **v**]
**-T**\ **c**\ \|\ **d**\ \|\ **g**\ \|GD(i)\|GD(t)
**-W**\ **c**\ \|\ **d**\ \|\ **f**\ \|\ **r**\ \|\ **s**\ [/**m**\ \|\ **v**]
[ **-V**\ [*level*\ ] ]

`Description <#toc2>`_
----------------------

**testapi** will test the API for all i/o combinations. In general, data
types (DATASET, TEXTSET, CPT, GRID, and IMAGE) can be read from or
written to 5 different ways (file, stream, file descriptor, copy from
memory, reference from memory). We use this tool to check the various
possibilities, for each data type. 

.. include:: explain_commonitems.rst_

`Required Arguments <#toc4>`_
-----------------------------

**-I**\ **c**\ \|\ **d**\ \|\ **f**\ \|\ **r**\ \|\ **s**\ [/**m**\ \|\ **v**]
    Set input method; choose from **c**\ opy from memory, file
    **d**\ escriptor, **f**\ ile, memory **r**\ eference, or
    **s**\ tream. For methods **c**\ \|\ **r**, optionally append
    /**m**\ \|\ **v**. Then, we obtain the values for the input dataset
    or grid via either a user matrix (**m**) or a set of user vectors
    (**v**). We simulate this internally by filling out a matrix or
    vectors and pass that as the source instead of a data file. **-Td**
    may take either **m**\ \|\ **v** while **-Tg** can only take the
    **m** modifier.
**-T**\ **c**\ \|\ **d**\ \|\ **g**\ \|GD(i)\|GD(t)
    Specify data type; choose from **c**\ pt, **d**\ ataset, **g**\ rid,
    **i**\ mage, or **t**\ extset.
**-W**\ **c**\ \|\ **d**\ \|\ **f**\ \|\ **r**\ \|\ **s**\ [/**m**\ \|\ **v**]
    Set output method; choose from **c**\ opy to memory, file
    **d**\ escriptor, **f**\ ile, memory **r**\ eference, or
    **s**\ tream. For methods **c**\ \|\ **r**, optionally append
    /**m**\ \|\ **v**. Then, we first write the values of the output
    dataset or grid via either a user matrix (**m**) or a set of user
    vectors (**v**). Finally, the matrix or vectors are written out to
    file. **-Td** may take either **m**\ \|\ **v** while **-Tg** can
    only take the **m** modifier.

`Optional Arguments <#toc5>`_
-----------------------------

.. |Add_-V| unicode:: 0x20 .. just an invisible code
.. include:: explain_-V.rst_

`Examples <#toc6>`_
-------------------

To check if reading and writing the test dataset file yields an
identical copy, try

testapi -Td -If -Wf -V

To check if reading the test grid from memory and writing it to file
yields an identical copy, try

testapi -Tg -Ic -Wf -V

To read the test grid via user matrix memory and writing it via another
user matrix before saving to a grid file, try

testapi -Tg -Ic/m -Wf/m -V

`See Also <#toc7>`_
-------------------

`gmt <gmt.html>`_ ,
