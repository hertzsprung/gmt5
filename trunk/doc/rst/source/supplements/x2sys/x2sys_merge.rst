***********
x2sys_merge
***********

x2sys_merge - Merge an updated COEs table (smaller) into the main table (bigger)

Synopsis
--------

.. include:: ../../common_SYN_OPTs.rst_

**x2sys_merge** **-A**\ *main_COElist.d* **-M**\ *new_COElist.d*

|No-spaces|

Description
-----------

**x2sys_merge** will read two crossovers data base and output the
contents of the main one updated with the COEs in the second one. The
second file should only contain updated COEs relatively to the first
one. That is, it MUST NOT contain any new two tracks intersections (This
point is NOT checked in the code). This program is useful when, for any
good reason like file editing NAV correction or whatever, one had to
recompute only the COEs between the edited files and the rest of the
database.

Required Arguments
------------------

**-A**\ *main_COElist.d*
    Specify the file *main_COElist.d* with the main crossover error
    data base.
**-M**\ *new_COElist.d*
    Specify the file *new_COElist.d* with the newly computed crossover
    error data base.

Optional Arguments
------------------

Examples
--------

To update the main COE_data.txt with the new COEs estimations saved in
the smaller COE_fresh.txt, try

   ::

    gmt x2sys_merge -ACOE_data.txt -MCOE_fresh.txt > COE_updated.txt

See Also
--------

`x2sys_binlist <x2sys_binlist.html>`_,
`x2sys_cross <x2sys_cross.html>`_,
`x2sys_datalist <x2sys_datalist.html>`_,
`x2sys_get <x2sys_get.html>`_,
`x2sys_init <x2sys_init.html>`_,
`x2sys_list <x2sys_list.html>`_,
`x2sys_put <x2sys_put.html>`_,
`x2sys_report <x2sys_report.html>`_
