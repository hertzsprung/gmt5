*********
mgd77path
*********

.. only:: not man

    mgd77path - Return paths to MGD77 cruises and directories

Synopsis
-------------------

.. include:: ../../common_SYN_OPTs.rst_

**mgd77path** *NGDC-ids* [ **-A**\ [**-**] ] [ **-D** ]
[ **-I**\ *ignore* ]
[ |SYN_OPT-V| ]

|No-spaces|

Description
-----------

**mgd77path** returns the full pathname to one or more MGD77 files. The
pathname returned for a given cruise may change with time due to
reshuffling of disks/subdirectories. 

Required Arguments
------------------

.. include:: explain_ncid.rst_

Optional Arguments
------------------

**-A**\ [**-**]
    Display the full path to each cruise [Default]. Optionally, append
    **-** which will list just the cruise IDs instead.

**-D**
    Instead of cruise listings, just show the directory paths currently
    used in the search.

**-I**\ *ignore*
    Ignore certain data file formats from consideration. Append
    **a\|c\|t** to ignore MGD77 ASCII, MGD77+ netCDF, or plain
    tab-separated ASCII table files, respectively. The option may be
    repeated to ignore more than one format. [Default ignores none].
    
.. |Add_-V| replace:: Reports the total number of cruises found. 
.. include:: ../../explain_-V.rst_

.. include:: ../../explain_help.rst_

Examples
--------

To obtain pathnames for cruises 01010008 and 01010007, run

   ::

    gmt mgd77path 01010008 01010007

To obtain pathnames for cruises 01010008 and 01010007, but only if there
are MGD77+ version in netCDF, run

   ::

    gmt mgd77path 01010008 01010007 -Ia -It

To see the list of active directories where MGD77 files might be stored, run

   ::

    gmt mgd77path -D

See Also
--------

`GMT <GMT.html>`_ `mgd77info <mgd77info.html>`_
`mgd77list <mgd77list.html>`_
`mgd77manage <mgd77manage.html>`_
`mgd77track <mgd77track.html>`_

`References <#toc8>`_
---------------------

The Marine Geophysical Data Exchange Format - MGD77, see
`*http://www.ngdc.noaa.gov/mgg/dat/geodas/docs/mgd77.txt*. <http://www.ngdc.noaa.gov/mgg/dat/geodas/docs/mgd77.txt.>`_
