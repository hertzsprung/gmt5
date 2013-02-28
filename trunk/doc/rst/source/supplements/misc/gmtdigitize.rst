***********
gmtdigitize
***********

gmtdigitize - Digitizing and Inverse map transformation of map x/y
coordinates

`Synopsis <#toc1>`_
-------------------

**gmtdigitize** **-J**\ *parameters*
**-R**\ *west*/*east*/*south*/*north*\ [**r**\ ] [ **-A** ] [
**-C**\ *device* ] [ **-D**\ *limit* ] [ **-F** ] [ **-L**\ *lpi* ] [
**-N**\ *namestem* ] [ **-S** ] [ **-V**\ [*level*\ ] ] [
**-Zk**\ \|\ **v** ] [ **-bo**\ [*ncols*\ ][*type*\ ] ] [
**-h**\ [**i**\ \|\ **o**][*n*\ ] ] [ > output.d ]

`Description <#toc2>`_
----------------------

**gmtdigitize** digitizes points from a digitizer via a serial line
connection and computes map coordinates using the specified map
projection. The program is interactive and will take you through the
setup procedure and how you will digitize points. The program will
determine the actual map scale as well as rotation of the paper that is
taped to the digitizer table. By default the output will go to stdout.

.. include:: ../../explain_commonitems.rst_

`Required Arguments <#toc4>`_
-----------------------------

.. include:: ../../explain_-J.rst_
|
|   For geographic projections you can give 1 as the scale will be solved for anyway.

.. |Add_-R| unicode:: 0x20 .. just an invisible code
.. include:: ../../explain_-R.rst_

`Optional Arguments <#toc5>`_
-----------------------------

**-A**
    Give an audible signal each time the digitizer mouse/puck is clicked
    [Default is silent].
**-C**\ *device*
    Specify the device (port) to read from [Default is /dev/ttyS0].
**-D**\ *limit*
    Only output a point if it is further than *limit* units from the
    previous point. Append **c**, **i**, **m**, **p** for cm, inch,
    meter, or point, respectively [Default is no limit].
**-F**
    Force the program to ask for 4 arbitrary calibration points [Default
    is to use the 4 corners of the map, if possible].
**-H**
    This option allows you to write out any number of header records to
    the beginning of the output file. Each record will automatically
    start with a #-character to indicate comment. Headers are not
    written if multiple output files are selected with **-N** **-m**.
**-L**\ *lpi*
    Set the digitizer table resolution in lines per inch [2540].
**-N**\ *namestem*
    Set name for output file(s). If a regular filename is given, then
    all digitized data will be written to that file. If the file
    contains a C-format for an integer (i.e., %d) then the file is used
    as a format statement to create unique filenames based on the
    current segment number (e.g., line\_%d.d will yield files line\_0.d,
    line\_1.d, etc). By default, all output is written to stdout.
    Multiple segment files requires specifying the **-m** option.
**-S**
    Suppress points that fall outside the specified map region [Default
    outputs all points].

.. |Add_-V| unicode:: 0x20 .. just an invisible code
.. include:: ../../explain_-V.rst_

**-Zk**\ \|\ **v**
    Append **v** to prompt for a *z*-value and output it as a third data
    column. Append **k** to output the button key as the final data
    column. Both **-Zk** and **-Zv** can be specified. [Default is just
    2 column x,y output].

.. |Add_-bo| unicode:: 0x20 .. just an invisible code
.. include:: ../../explain_-bo.rst_

.. include:: ../../explain_help.rst_

`Examples <#toc6>`_
-------------------

To digitize lines from a mercator map made for a given region, and save
each line segment in individual files called segment\_000.xy,
segment\_001.xy etc, try

gmtdigitize -R20/50/12/25 -Jm1:1 -Nsegment\_%03d.xy

To digitize seismically defined interfaces from a multichannel seismic
section, with horizontal distances from 130 to 970, and vertical times
from 0 to 10 seconds, write out the button code, and save all line
segment to a single multisegment file, and beep at each click, try

gmtdigitize -R130/970/0/10 -Jx1/-1 -A -Z > interfaces.d

`System Setup <#toc7>`_
-----------------------

This applies to the Calcomp DrawingBoard III hooked up to a RedHat Linux
workstation. We use /dev/ttyS0 as the serial port and change permissions
so that it is world read/write-able. Then, stty -F /dev/ttyS0 evenp will
set the terminal settings, which can be checked with stty -F /dev/ttyS0
-a. Setup of digitizer: We use the CalComp 2000 ASCII (Save 3) setup,
which has:
 Mode: Point
 Baud Rate: 9600
 Data Bits: 7
 Parity: Even
 Data Rate: 125 pps
 Resolution: 200 lpi
 Output Format: Format 0
 Emulation: CalComp 2000 ASCII
 (A)We need to make a slight modification to the Preset No 3 settings:
(1) 2450 LPI instead of 200, `and (2) <and.2.html>`_ None instead of yes
for added CR. These modifications can be changed and saved to Preset 3
on the digitizer but a power outage may reset in back to the factory
defaults, necessitating a manual reset of those two settings. (B) Setup
tty port. stty -F /dev/ttyS0 evenp (C) Run gmtdigitize. Map scale does
not matter; it is computed from the region and plot size.

`See Also <#toc8>`_
-------------------

`gmtdefaults <gmtdefaults.l.html>`_ , `GMT <GMT.l.html>`_
, `gmtstitch <gmtstitch.l.html>`_ ,
`mapproject <mapproject.l.html>`_ , `project <project.l.html>`_
