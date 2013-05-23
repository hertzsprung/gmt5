***********
gmtdefaults
***********

gmtdefaults - List current **GMT** default parameters

Synopsis
--------

.. include:: common_SYN_OPTs.rst_

**gmtdefaults** [ **-D**\ [**u**\ \|\ **s**] ]

|No-spaces|

Description
-----------

**gmtdefaults** lists the **GMT** parameter defaults if the option
**-D** is used. There are three ways to change some of the settings: (1)
Use the command `gmtset <gmtset.html>`_, (2) use any texteditor to edit the file
`gmt.conf <gmt.conf.html>`_ in your home, ~/.gmt or current directory (if you do not
have this file, run `gmtset <gmtset.html>`_ **-D** to get one with the system default
settings), `or (3) <or.html>`_ override any parameter by specifying one
or more **--PARAMETER**\ =\ *VALUE* statements on the commandline of any
**GMT** command (**PARAMETER** and *VALUE* are any combination listed
below). The first two options are permanent changes until explicitly
changed back, while the last option is ephemeral and only applies to the
single **GMT** command that received the override. **GMT** can provide
default values in US or SI units. This choice is determined at compile
time.

Required Arguments
------------------

None.

Optional Arguments
------------------

**-D**
    Print the system **GMT** defaults to standard output. Append **u**
    for US defaults or **s** for SI defaults. [**-D** alone gives the
    version selected at compile time; If **-D** is omitted, the user’s
    currently active defaults are printed.] 

.. include:: explain_help.rst_

Your currently active defaults come from the `gmt.conf <gmt.conf.html>`_ file in
the current working directory, if present; else from the
`gmt.conf <gmt.conf.html>`_ file in your home directory, if present; else from the
file **~/.gmt/gmt.conf** if present; else from the system defaults
set at the time **GMT** was compiled.

GMT PARAMETERS
--------------

Read the `gmt.conf <gmt.conf.html>`_ man page for a full list of the parameters that
are user-definable in **GMT**.

Examples
--------

To get a copy of the **GMT** parameter defaults in your home directory, run

    gmtdefaults -D > ~/gmt.conf

You may now change the settings by editing this file using a text editor
of your choice, or use **gmtset** to change specified parameters on the
command line.

Bugs
----

If you have typographical errors in your `gmt.conf <gmt.conf.html>`_ file(s), a warning
message will be issued, and the **GMT** defaults for the affected
parameters will be used.

See Also
--------

`gmt <gmt.html>`_, `gmt.conf <gmt.conf.html>`_,
`gmtcolors <gmtcolors.html>`_, `gmtget <gmtget.html>`_,
`gmtset <gmtset.html>`_
