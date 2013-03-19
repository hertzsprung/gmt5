*****
pslib
*****

PSL 5.0 - A *PostScript* based plotting library

`Description <#toc1>`_
----------------------

**PSL** was created to make the generation of *PostScript* page
description code easier. PS is a page description language developed by
the Adobe for specifying how a printer should render a page of text or
graphics. It uses a reverse Polish notation that puts and gets items
from a stack to draws lines, text, and images and even performs
calculations. **PSL** is a self-contained library that presents a series
of functions that can be used to create plots. The resulting
*PostScript* code is ASCII text (with some exceptions for images if so
desired) and can thus be edited using any text editor. Thus, it is
possible to modify a plot file even after it has been created, e.g., to
change text strings, set new gray shades or colors, experiment with
various pen widths, etc. Furthermore, various tools exist that can parse
*PostScript* and let you make such edits via a graphical user interface
(e.g., Adobe Illustrator). **PSL** is written in C but includes FORTRAN
bindings and can therefore be called from both C and FORTRAN programs.
To use this library, you must link your plotting program with **PSL**.
**PSL** is used by the **GMT** graphics programs to generate PS. **PSL**
output is freeform *PostScript* that conforms to the Adobe *PostScript*
File Specification Version 3.0.

Before any **PSL** calls can be issued, the plotting system must be
initialized. This is done by calling **PSL\_beginsession**, which
initializes a new **PSL** session; then call **PSL\_setdefaults** which
sets internal variables and default settings, accepts settings for
measurement units and character encoding, and returns a pointer to a
struct PSL\_CTRL which must be passed as first argument to all other
**PSL** functions. The measure unit for sizes and positions can be set
to be centimeter (c), inch (i), meter (m), or `points
(p) <points.p.html>`_ . A **PSL** session is terminated by calling
**PSL\_endsession**. You may create one or more plots within the same
session. A new plot is started by calling **PSL\_beginplot**, which
defines macros, sets up the plot-coordinate system, scales, and
[optionally] opens a file where all the PS code will be written.
Normally, the plot code is written to *stdout*. When all plotting to
this file is done, you finalize the plot by calling **PSL\_endplot**.

A wide variety of output devices that support *PostScript* exist,
including many printers and large-format plotters. Many tools exists to
display *PostScript* on a computer screen. Open source tools such as
ghostscript can be used to convert *PostScript* into PDF or raster
images (e.g., TIFF, JPEG) at a user-defined resolution (DPI). In
particular, the GMT tool ps2raster is a front-end to ghostscript and
pre-selects the optimal options for ghostscript that will render quality
PDF and images.

The **PSL** is fully 64-bit compliant. Integer parameters are here
specified by the type **long** to distinguish them from the 32-bit
**int**. Note that under standard 32-bit compilation they are
equivalent. Users of this library under 64-bit mode must make sure they
pass proper **long** variables (under Unix flavors) or **\_\_int64**
under Windows 64.

`Units <#toc2>`_
----------------

**PSL** can be instructed to use centimeters, inches, meters or points
as input units for the coordinates and sizes of elements to be plotted.
Any dimension that takes this setting as a unit is specified as *user
units* or *plot units* in this manual. Excluded from this are line
widths and font sizes which are always measured in *points*. The user
units can be further refined by calling **PSL\_beginaxes**, giving the
user the opportunity to specify any linear coordinate frame. Changing
the coordinate frame only affects the coordinates of plotted material
indicated as measured in *plot units*, not the sizes of symbols (which
remain in *user units*), nor line widths or font sizes (which remain in
*points*).

`Color <#toc3>`_
----------------

**PSL** uses the direct color model where red, green, and blue are given
separately, each must be in the range from 0-1. If red = -1 then no fill
operation takes place. If red = -3, then pattern fill will be used, and
the green value will indicate the pattern to be used. Most plot-items
can be plotted with or without outlines. If outline is desired (i.e.,
set to 1), it will be drawn using the current line width and pattern.
**PSL** uses highly optimized macro substitutions and scales the
coordinates depending on the resolution of the hardcopy device so that
the output file is kept as compact as possible.

`Justification <#toc4>`_
------------------------

Text strings, text boxes and images can be "justified" by specifying the
corner to which the *x* and *y* coordinates of the subroutine call
apply. Nine different values are possible, as shown schematically in
this diagram:

    9------------10----------- 11

    \| \|

    5 6 7

    \| \|

    1------------ 2------------ 3

The box represents the text or image. E.g., to plot a text string with
its center at (*x*, *y*), you must use *justify* == 6. *justify* == 0
means "no justification", which generally means (*x*, *y*) is at the
bottom left. Convenience values PSL\_NONE, PSL\_BL, PSL\_BC, PSL\_BL,
PSL\_ML, PSL\_MC, PSL\_MR, PSL\_TL, PSL\_TC and PSL\_TR are available.

`Initialization <#toc5>`_
-------------------------

These functions initialize or terminate the **PSL** system. We use the
term **PSL** session to indicate one instance of the **PSL** system (a
complicated program could run many **PSL** sessions concurrently as each
would operate via its own control structure). During a single session,
one or more plots may be created. Here are the functions involved in
initialization:

**struct PS\_CTRL \*New\_PSL\_Ctrl** (**char** *\*session*)

    This is the first function that must be called as it creates a new
    **PSL** session. Specifically, it will allocate a new **PSL**
    control structure and initialize the session default parameters. The
    pointer that is returned must be passed to all subsequent **PSL**
    functions.

**long \*PSL\_beginsession** (**struct PS\_CTRL** *\*PSL*, **long**
*search*, **char** *\*sharedir*, **char** *\*userdir*)

    This is the second function that must be called as it initializes
    the new **PSL** session. Here, *search* is an integer that is passed
    as 0 in GMT but should be 1 for other users. If so we will search
    for the environmental parameters PSL\_SHAREDIR and PSL\_USERDIR
    should the corresponding arguments *sharedir* and *userdir* be NULL.

**long PSL\_endsession** (**struct PS\_CTRL** *\*PSL*)

    This function terminates the active **PSL** session; it is the last
    function you must call in your program. Specifically, this function
    will deallocate memory used and free up resources.

**struct PS\_CTRL \*PSL\_beginlayer** (**struct PS\_CTRL** *\*PSL*,
**long** *layer*)

    Adds a DSC comment by naming this layer; give a unique integer
    value. Terminate layer with PSL\_endlayer

**struct PS\_CTRL \*PSL\_endlayer** (**struct PS\_CTRL** *\*PSL*)

    Terminate current layer with a DSC comment.

**long PSL\_fopen** (**char** *\*file*, **char** *\*mode*)

    This function simply opens a file, just like fopen. The reason it is
    replicated here is that under Windows, file pointers must be
    assigned within the same DLL as they are being used. Yes, this is
    retarded but if we do not do so then PSL will not work well under
    Windows. Under non-Windows this functions is just a macro that
    becomes fopen.

**void PSL\_free** (**void** *\*ptr*)

    This function frees up the memory allocated inside **PSL**.
    Programmers using C/C++ should now this is a macro and there is no
    need to cast the pointer to *void \** as this will be done by the
    macro. Fortran programmers should instead call
    **PSL\_freefunction**.

**void PSL\_beginaxes** (**struct PS\_CTRL** *\*PSL*, **double** *llx*,
**double** *lly*, **double** *width*, **double** *height*, **double**
*x0*, **double** *y0*, **double** *x1*, **double** *y1*)

    This function sets up the mapping that takes the users data
    coordinates and converts them to the positions on the plot in
    *PostScript* units. This should be used when plotting data
    coordinates and is terminated with **PSL\_endaxes**, which returns
    **PSL** to the default measurement units and scaling. Here, *llx*
    and *lly* sets the lower left position of the mapping region, while
    *width* and *height* sets the dimension of the plot area in user
    units. Finally, *x0*, *x1* and *y0*, *y1* indicate the range of the
    users x- and y-coordinates, respectively. Specify a reverse axis
    direction (e.g., to let the y-axis be positive down) by setting *y0*
    larger than *y1*, and similarly for an x-axis that increases to the
    left.

**void PSL\_endaxes** (**struct PS\_CTRL** *\*PSL*)

    Terminates the map scalings initialized by **PSL\_beginaxes** and
    returns **PSL** to standard scaling in measurement units.

**long PSL\_beginplot** (**struct PSL\_CTRL** *\*P*, **FILE** *\*fp*,
**long** *orientation*, **long** *overlay*, **long** *color\_mode*,
**char** *origin*\ [], **double** *offset*\ [], **double**
*page\_size*\ [], **char** *\*title*, **long** *font\_no*\ [])

    Controls the initiation (or continuation) of a particular plot
    within the current session. Pass file pointer *fp* where the
    *PostScript* code will be written; if NULL then the output is
    written to *stdout*. The Fortran interface always sends to *stdout*.
    The *orientation* may be landscape (PSL\_LANDSCAPE or 0) or portrait
    (PSL\_PORTRAIT or 1). Set *overlay* to PSL\_OVERLAY (0) if the
    following *PostScript* code should be appended to an existing plot;
    otherwise pass `PSL\_INIT (1) <PSL_INIT.html>`_ to start a new plot.
    Let *colormode* be one of PSL\_RGB (0), `PSL\_CMYK
    (1) <PSL_CMYK.html>`_ , `PSL\_HSV (2) <PSL_HSV.2.html>`_ or
    `PSL\_GRAY (3) <PSL_GRAY.html>`_ ; this setting controls how colors
    are presented in the *PostScript* code. The *origin* setting
    determines for x and y separately the origin of the specified
    offsets (next argument). Each of the two characters are either ’r’
    for an offset relative to the current origin, ’a’ for a temporaty
    adjustment of the origin which is undone during BD(PSL\_endplot),
    ’f’ for a placement of the origin relative to the lower left corner
    of the page, ’c’ for a placement of the origin relative to the
    center of the page. The array *offset* specifies the offset of the
    new origin relative to the position indicated by **origin**.
    *page\_size* means the physical width and height of the plotting
    media in points (typically 612 by 792 for Letter or 595 by 842 for
    A4 format). The character string *title* can be used to specify the
    **%%Title:** header in the *PostScript* file (or use NULL for the
    default). The array *font\_no* specifies all fonts used in the plot
    (by number), or use NULL to leave out the
    **%%DocumentNeededResources:** comment in the *PostScript* file.

**long PSL\_endplot** (**struct PSL\_CTRL** *\*P*, **long**
*last\_page*)

    Terminates the plotting sequence and closes plot file (if other than
    *stdout*). If *last\_page* == `PSL\_FINALIZE
    (1) <PSL_FINALIZE.html>`_ , then a *PostScript* *showpage* command
    is issued, which initiates the printing process on hardcopy devices.
    Otherwise, pass PSL\_OVERLAY (0).

**long PSL\_setorigin** (**struct PSL\_CTRL** *\*P*, **double**
*xorigin*, **double** *yorigin*, **double** *angle*, **long** *mode*)

    Changes the coordinate system by translating by
    (*xorigin*,\ *yorigin*) followed by a *angle*-degree rotation
    (*mode*\ =PSL\_FWD or 0) or alternatively the rotation followed by
    translation (*mode*\ =PSL\_INV or 1).

`Changing Settings <#toc6>`_
----------------------------

The following functions are used to change various **PSL** settings and
affect the current state of parameters such as line and fill attributes.

**long PSL\_define\_pen** (**struct PSL\_CTRL** *\*P*, **char**
*\*name*, **long** *width*, **char** *\*style*, **double** *offset*,
**double** *rgb*\ [])

    Stores the specified pen characteristics in a *PostScript* variable
    called *name*. This can be used to place certain pen attributes in
    the *PostScript* file and then retrieve them later with
    **PSL\_load\_pen**. This makes the stored pen the current pen.

**long PSL\_define\_rgb** (**struct PSL\_CTRL** *\*P*, **char**
*\*name*, **double** *rgb*\ [])

    Stores the specified color in a *PostScript* variable called *name*.
    This can be used to place certain color values in the *PostScript*
    file and then retrieve them later with **PSL\_load\_rgb**. This
    makes the stored color the current color.

**long PSL\_setcolor** (**struct PSL\_CTRL** *\*P*, **double**
*rgb*\ [], **long** *mode*)

    Sets the current color for all stroked (mode = PSL\_IS\_STROKE (0))
    or filled (mode = `PSL\_IS\_FILL (1) <PSL_IS_FILL.html>`_ ) material
    to follow (lines, symbol outlines, text). *rgb* is a triplet of red,
    green and blue values in the range 0.0 through 1.0. Set the red
    color to -3.0 and the green color to the pattern number returned by
    **PSL\_setpattern** to select a pattern as current paint color. For
    PDF transparency, set *rgb*\ [3] to a value between 0 (opaque) and 1
    (fully transparent).

**long PSL\_setpattern** (**struct PSL\_CTRL** *\*P*, **long**
*image\_no*, **char** *\*imagefile*, **long** *dpi*, **double**
*f\_rgb*\ [], **double** *b\_rgb*\ [])

    Sets up the specified image pattern as the fill to use for polygons
    and symbols. Here, *image\_no* is the number of the standard PSL
    fill patterns (1-90; use a negative number when you specify an image
    *filename* instead. The scaling (i.e., resolution in dots per inch)
    of the pattern is controlled by the image *dpi*; if set to 0 it will
    be plotted at the device resolution. The two remaining settings
    apply to 1-bit images only and are otherwise ignored: You may
    replace the foreground color (the set bits) with the *f\_rgb* color
    and the background color (the unset bits) with *b\_rgb*.
    Alternatively, pass either color with the red component set to -1.0
    and we will instead issue an image mask that is see-through for the
    specified fore- or background component. To subsequently use the
    pattern as a pen or fill color, use **PSL\_setcolor** or
    DB(PSL\_setfill) with the a color *rgb* code made up of *r* = -3,
    and *b* = the pattern number returned by **PSL\_setpattern**.

**long PSL\_setdash** (**struct PSL\_CTRL** *\*P*, **char** *\*pattern*,
**double** *offset*)

    Changes the current pen style attributes. The character string
    *pattern* contains the desired pattern using a series of lengths in
    points specifying the alternating lengths of dashes and gaps in
    points. E.g., "4 2" and *offset* = 1 will plot like

        x ---- ---- ----

    where x is starting point of a line (The x is not plotted). That is,
    the line is made up of a repeating pattern of a 4 points long solid
    line and a 2 points long gap, starting 1 point after the x. To reset
    to solid line, specify *pattern* = NULL ("") and *offset* = 0.

**long PSL\_setfill** (**struct PSL\_CTRL** *\*P*, **double** *rgb*\ [],
**long** *outline*)

    Sets the current fill color and whether or not outline is needed for
    symbols. Special cases are handled by passing the red color as -1.0
    (no fill), -2.0 (do not change the outline setting) or -3.0 (select
    the image pattern indicated by the second (green) element of *rgb*).
    For PDF transparency, set *rgb*\ [3] to a value between 0 (opaque)
    and 1 (fully transparent). Set outline to `PSL\_OUTLINE
    (1) <PSL_OUTLINE.html>`_ to draw the outlines of polygons and
    symbols using the current pen.

**long PSL\_setfont** (**struct PSL\_CTRL** *\*P*, **long** *fontnr*)

    Changes the current font number to *fontnr*. The fonts available
    are: 0 = Helvetica, 1 = H. Bold, 2 = H. Oblique, 3 = H.
    Bold-Oblique, 4 = Times, 5 = T. Bold, 6 = T. Italic, 7 = T. Bold
    Italic, 8 = Courier, 9 = C. Bold, 10 = C Oblique, 11 = C Bold
    Oblique, 12 = Symbol, 13 = AvantGarde-Book, 14 = A.-BookOblique, 15
    = A.-Demi, 16 = A.-DemiOblique, 17 = Bookman-Demi, 18 =
    B.-DemiItalic, 19 = B.-Light, 20 = B.-LightItalic, 21 =
    Helvetica-Narrow, 22 = H-N-Bold, 23 = H-N-Oblique, 24 =
    H-N-BoldOblique, 25 = NewCenturySchlbk-Roman, 26 = N.-Italic, 27 =
    N.-Bold, 28 = N.-BoldItalic, 29 = Palatino-Roman, 30 = P.-Italic, 31
    = P.-Bold, 32 = P.-BoldItalic, 33 = ZapfChancery-MediumItalic, 34 =
    ZapfDingbats, 35 = Ryumin-Light-EUC-H, 36 = Ryumin-Light-EUC-V, 37 =
    GothicBBB-Medium-EUC-H, and 38 = GothicBBB-Medium-EUC-V. If *fontnr*
    is outside this range, it is reset to 0.

**long PSL\_setformat** (**struct PSL\_CTRL** *\*P*, **long**
*n\_decimals*)

    Sets the number of decimals to be used when writing color or gray
    values. The default setting of 3 gives 1000 choices per red, green,
    and blue value, which is more than the 255 choices offered by most
    24-bit platforms. Choosing a lower value will make the output file
    smaller at the expense of less color resolution. Still, a value of 2
    gives 100 x 100 x 100 = 1 million colors, more than most eyes can
    distinguish. For a setting of 1, you will have 10 nuances per
    primary color and a total of 1000 unique combinations.

**long PSL\_setlinewidth** (**struct PSL\_CTRL** *\*P*, **double**
*linewidth*)

    Changes the current line width in points. Specifying 0 gives the
    thinnest line possible, but this is implementation-dependent (seems
    to work fine on most *PostScript* printers).

**long PSL\_setlinecap** (**struct PSL\_CTRL** *\*P*, **long** *cap*)

    Changes the current line cap, i.e., what happens at the beginning
    and end of a line segment. PSL\_BUTT\_CAP (0) gives butt line caps
    [Default], `PSL\_ROUND\_CAP (1) <PSL_ROUND_CAP.html>`_ selects round
    caps, while `PSL\_SQUARE\_CAP (2) <PSL_SQUARE_CAP.2.html>`_ results
    in square caps. THus, the two last options will visually lengthen a
    straight line-segment by half the line width at either end.

**long PSL\_setlinejoin** (**struct PSL\_CTRL** *\*P*, **long** *join*)

    Changes the current linejoin setting, which handles how lines of
    finite thickness are joined together when the meet at different
    angles. PSL\_MITER\_JOIN (0) gives a mitered joint [Default],
    `PSL\_ROUND\_JOIN (1) <PSL_ROUND_JOIN.html>`_ makes them round,
    while `PSL\_BEVEL\_JOIN (2) <PSL_BEVEL_JOIN.2.html>`_ produces bevel
    joins.

**long PSL\_setmiterlimit** (**struct PSL\_CTRL** *\*P*, **long**
*limit*)

    Changes the current miter limit used for mitered joins.
    PSL\_MITER\_DEFAULT (35) gives the default PS miter; other values
    are interpreted as the cutoff acute angle (in degrees) when mitering
    becomes active.

**long PSL\_settransparencymode** (**struct PSL\_CTRL** *\*P*, **char**
*\*mode*)

    Changes the current PDF transparency rendering mode [Default is
    Normal]. Choose among Color, ColorBurn, ColorDodge, Darken,
    Difference, Exclusion, HardLight, Hue, Lighten, Luminosity,
    Multiply, Normal, Overlay, Saturation, SoftLight, and Screen.

**long PSL\_setdefaults** (**struct PSL\_CTRL** *\*P*, **double**
*xyscales*\ [], **double** *pagergb*\ [], **char** *\*encoding*)

    Allows changes to the PSL session settings and should be called
    immediately after **PSL\_beginsession**. The *xyscales* array affect
    an overall magnification of your plot [1,1]. This can be useful if
    you design a page-sized plot but would then like to magnify (or
    shrink) it by a given factor. Change the default paper media color
    [white; 1/1/1] by specifying an alternate page color. Passing zero
    (or NULL for *pagergb*) will leave the setting unchanged. Finally,
    pass the name of the character set encoding (if NULL we select
    Standard).

**long PSL\_defunits** (**struct PSL\_CTRL** *\*P*, **char** *\*name*,
**double** *value*)

    Creates a *PostScript* variable called *name* and initializes it to
    the equivalent of *value* user units.

**long PSL\_defpoints** (**struct PSL\_CTRL** *\*P*, **char** *\*name*,
**double** *fontsize*)

    Creates a *PostScript* variable called *name* and initializes it to
    the value that corresponds to the font size (in points) given by
    *fontsize*.

`Plotting Lines And Polygons <#toc7>`_
--------------------------------------

Here are functions used to plot lines and closed polygons, which may
optionally be filled. The attributes used for drawing and filling are
set prior to calling these functions; see CHANGING SETTINGS above.

**long PSL\_plotarc** (**struct PSL\_CTRL** *\*P*, **double** *x*,
**double** *y*, **double** *radius*, **double** *angle1*, **double**
*angle2*, **long** *type*)

    Draws a circular arc with its center at plot coordinates (*x*, *y*),
    starting from angle *angle1* and end at *angle2*. Angles must be
    given in decimal degrees. If *angle1* > *angle2*, a negative arc is
    drawn. The *radius* is in user units. The *type* determines how the
    arc is interpreted: `PSL\_MOVE (1) <PSL_MOVE.html>`_ means set new
    anchor point, `PSL\_STROKE (2) <PSL_STROKE.2.html>`_ means stroke
    the arc, PSL\_MOVE + `PSL\_STROKE (3) <PSL_STROKE.html>`_ means
    both, whereas PSL\_DRAW (0) justs adds to arc path to the current
    path.

**long PSL\_plotline** (**struct PSL\_CTRL** *\*P*, **double** *x*,
**double** *y*, **long** *n*, **long** *type*)

    Assemble a continuous line through *n* points whose the plot
    coordinates are in the *x*, *y* arrays. To continue an existing
    line, use *type* = PSL\_DRAW (0), or if this is the first segment in
    a multisegment path, set *type* = `PSL\_MOVE (1) <PSL_MOVE.html>`_ .
    To end the segments and draw the lines, add `PSL\_STROKE
    (2) <PSL_STROKE.2.html>`_ . Thus, for a single segment, *type* must
    be PSL\_MOVE + `PSL\_STROKE (3) <PSL_STROKE.html>`_ . The line is
    drawn using the current pen attributes. Add `PSL\_CLOSE
    (8) <PSL_CLOSE.8.html>`_ to *type* to close the first and last point
    by the *PostScript* operators.

**long PSL\_plotpoint** (**struct PSL\_CTRL** *\*P*, **double** *x*,
**double** *y*, **long** *type*)

    Moves the pen from the current to the specified plot coordinates
    (*x*, *y*) and optionally draws and strokes the line, depending on
    *type*. Specify *type* as either a move (PSL\_MOVE, 1), or draw
    (PSL\_DRAW, 2), or draw and stroke (PSL\_DRAW + PSL\_STOKE, 3) using
    current pen attributes. It the coordinates are relative to the
    current point add `PSL\_REL (4) <PSL_REL.4.html>`_ to *type*.

**long PSL\_plotbox** (**struct PSL\_CTRL** *\*P*, **double** *x0*,
**double** *y0*, **double** *x1*, **double** *y1*)

    Creates a closed box with opposite corners at plot coordinates
    (*x0*,\ *y1*) and (*x1*,\ *y1*). The box may be filled and its
    outline stroked depending on the current settings for fill and pen
    attributes.

**long PSL\_plotpolygon** (**struct PSL\_CTRL** *\*P*, **double** *x*,
**double** *y*, **long** *n*)

    Creates a closed polygon through *n* points whose plot coordinates
    are in the *x*, *y* arrays. The polygon may be filled and its
    outline stroked depending on the current settings for fill and pen
    attributes.

**long PSL\_plotsegment** (**struct PSL\_CTRL** *\*P*, **double** *x0*,
**double** *y0*, **double** *x1*, **double** *y1*)

    Draws a line segment between the two points (plot coordinates) using
    the current pen attributes.

`Plotting Symbols <#toc8>`_
---------------------------

Here are functions used to plot various geometric symbols or constructs.

**long PSL\_plotaxis** (**struct PSL\_CTRL** *\*P*, **double**
*tickval*, **char** *\*label*, **double** *fontsize*, **long** *side*)

    Plots a basic axis with tick marks, annotations, and label. Assumes
    that **PSL\_beginaxes** has been called to set up positioning and
    user data ranges. Annotations will be set using the *fontsize* in
    points. *side* can be 0, 1, 2, or 3, which selects lower x-axis,
    right y-axis, upper x-axis, or left y-axis, respectively. The
    *label* font size is set to 1.5 times the *fontsize*.

**long PSL\_plotsymbol** (**struct PSL\_CTRL** *\*P*, **double** *x*,
**double** *y*, **double** *size*\ [], **long** *symbol*)

    Plots a simple geometric symbol centered on plot coordinates (*x*,
    *y*). The argument *symbol* selects the geometric symbol to use.
    Most symbols are scaled to fit inside a circle of diameter given as
    *size*\ [0], but some symbols take additional parameters. Choose
    from these 1-parameter symbols using the predefined self-explanatory
    integer values PSL\_CIRCLE, PSL\_DIAMOND, PSL\_HEXAGON,
    PSL\_INVTRIANGLE, PSL\_OCTAGON, PSL\_PENTAGON, PSL\_SQUARE,
    PSL\_STAR, and PSL\_TRIANGLE; these may all be filled and stroked if
    **PSL\_setfill** has been called first. In addition, you can choose
    several line-only symbols that cannot be filled. They are
    PSL\_CROSS, PSL\_DOT, PSL\_PLUS, PSL\_XDASH, and PSL\_YDASH.
    Finally, more complicated symbols require more than one parameter to
    be passed via *size*. These are PSL\_ELLIPSE (*size* is expected to
    contain the three parameter *angle*, *major*, and *minor* axes,
    which defines an ellipse with its major axis rotated by *angle*
    degrees), PSL\_MANGLE (*size* is expected to contain the 8
    parameters *radius*, *angle1*, and *angle2* for the math angle
    specification, followed by *tailwidth*, *headlength*, *headwidth*,
    *shape*, and *status* (see PSL\_VECTOR below for explanation),
    PSL\_WEDGE (*size* is expected to contain the three parameter
    *radius*, *angle1*, and *angle2* for the sector specification),
    PSL\_RECT (*size* is expected to contain the two dimensions *width*
    and *height*), PSL\_RNDRECT (*size* is expected to contain the two
    dimensions *width* and *height* and the *radius* of the corners),
    PSL\_ROTRECT (*size* is expected to contain the three parameter
    *angle*, *width*, and *height*, with rotation relative to the
    horizontal), and PSL\_VECTOR (*size* is expected to contain the 7
    parameters *x\_tip*, *y\_tip*, *tailwidth*, *headlength*,
    *headwidth*, *shape*, and *status*. Here (*x\_tip*,\ *y\_tip*) are
    the coordinates to the head of the vector, while (*x*, *y*) are
    those of the tail. *shape* can take on values from 0-1 and specifies
    how far the intersection point between the base of a straight vector
    head and the vector line is moved toward the tip. 0.0 gives a
    triangular head, 1.0 gives an arrow shaped head. The *status* value
    is a bit-flag being the sum of several possible contributions:
    `PSL\_VEC\_RIGHT (2) <PSL_VEC_RIGHT.2.html>`_ = only draw right half
    of vector head, `PSL\_VEC\_BEGIN (4) <PSL_VEC_BEGIN.4.html>`_ =
    place vector head at beginning of vector, `PSL\_VEC\_END
    (8) <PSL_VEC_END.8.html>`_ = place vector head at end of vector,
    PSL\_VEC\_JUST\_B (0) = align vector beginning at (x,y),
    PSL\_VEC\_JUST\_C (16) = align vector center at (x,y),
    PSL\_VEC\_JUST\_E (32) = align vector end at (x,y),
    PSL\_VEC\_JUST\_S (64) = align vector center at (x,y),
    PSL\_VEC\_OUTLINE (128) = draw vector head outline using default
    pen, PSL\_VEC\_FILL (512) = fill vector head using default fill,
    PSL\_VEC\_MARC90 (2048) = if angles subtend 90, draw straight angle
    symbol (PSL\_MANGLE only). The symbol may be filled and its outline
    stroked depending on the current settings for fill and pen
    attributes.

`Plotting Images <#toc9>`_
--------------------------

Here are functions used to read and plot various images.

**long PSL\_plotbitimage** (**struct PSL\_CTRL** *\*P*, **double** *x*,
**double** *y*, **double** *xsize*, **double** *ysize*, **int**
*justify*, **unsigned char** *buffer*, **long** *nx*, **long** *ny*,
**double** *f\_rgb*\ [], **double** *b\_rgb*\ [])

    Plots a 1-bit image image at plot coordinates (*x*, *y*) justified
    as per the argument *justify* (see **JUSTIFICATION** for details).
    The target size of the image is given by *xsize* and *ysize* in user
    units. If one of these is specified as zero, the corresponding size
    is adjusted to the other such that the aspect ratio of the original
    image is retained. *buffer* is an unsigned character array in
    scanline orientation with 8 pixels per byte. *nx*, *ny* refers to
    the number of pixels in the image. The rowlength of *buffer* must be
    an integral number of 8; pad with zeros. *buffer*\ [0] is upper left
    corner. You may replace the foreground color (the set bits) with the
    *f\_rgb* color and the background color (the unset bits) with
    *b\_rgb*. Alternatively, pass either color with the red component
    set to -1.0 and we will instead issue an image mask that is
    see-through for the specified fore- or background component. See the
    Adobe Systems *PostScript* Reference Manual for more details.

**long PSL\_plotcolorimage** (**struct PSL\_CTRL** *\*P*, **double**
*x*, **double** *y*, **double** *xsize*, **double** *ysize*, **int**
*justify*, **unsigned char** *\*buffer*, **long** *nx*, **long** *ny*,
**long** *depth*)

    Plots a 1-, 2-, 4-, 8-, or 24-bit deep image at plot coordinates
    (*x*, *y*) justified as per the argument *justify* (see
    **JUSTIFICATION** for details). The target size of the image is
    given by *xsize* and *ysize* in user units. If one of these is
    specified as zero, the corresponding size is adjusted to the other
    such that the aspect ratio of the original image is retained. This
    functions sets up a call to the *PostScript* colorimage or image
    operators. The pixel values are stored in *buffer*, an unsigned
    character array in scanline orientation with gray shade or r/g/b
    values (0-255). *buffer*\ [0] is the upper left corner. *depth* is
    number of bits per pixel (24, 8, 4, 2, or 1). *nx*, *ny* refers to
    the number of pixels in image. The rowlength of *buffer* must be an
    integral number of 8/\ *Idepth*. E.g. if *depth* = 4, then
    *buffer*\ [j]/16 gives shade for pixel[2j-1] and *buffer*\ [j%16
    (mod 16) gives shade for pixel[2j]. When *-depth* is passed instead
    then "hardware" interpolation of the image is requested (this is
    implementation dependent). If *-nx* is passed with 8- (or 24-) bit
    images then the first one (or three) bytes of *buffer* holds the
    gray (or r/g/b) color for pixels that are to be masked out using the
    PS Level 3 Color Mask method. See the Adobe Systems *PostScript*
    Reference Manual for more details.

**long PSL\_plotepsimage** (**struct PSL\_CTRL** *\*P*, **double** *x*,
**double** *y*, **double** *xsize*, **double** *ysize*, **int**
*justify*, **unsigned char** *\*buffer*, **long** *size*, **long** *nx*,
**long** *ny*, **long** *ox*, **long** *oy*)

    Plots an Encapsulated *PostScript* (EPS) image at plot coordinates
    (*x*, *y*) justified as per the argument *justify* (see
    **JUSTIFICATION** for details). The target size of the image is
    given by *xsize* and *ysize* in user units. If one of these is
    specified as zero, the corresponding size is adjusted to the other
    such that the aspect ratio of the original image is retained. The
    EPS file is stored in *buffer* and has *size* bytes. This function
    simply includes the image in the *PostScript* output stream within
    an appropriate wrapper. Specify position of lower left corner and
    size of image. *nx*, *ny*, *ox*, *oy* refers to the width, height
    and origin (lower left corner) of the BoundingBox in points.

**long PSL\_loadimage** (**struct PSL\_CTRL** *\*P*, **FILE** *\*fp*,
**struct imageinfo** *\*header*, **unsigned char** *\*\*image*)

    Reads the image contents of the EPS file or a raster image pointed
    to by the open file pointer *fp*. The routine can handle
    Encapsulated *PostScript* files or 1-, 8-, 24-, or 32-bit raster
    images in old, standard, run-length encoded, or RGB-style Sun
    format. Non-Sun rasters are automatically reformatted to Sun rasters
    via a system call to ImageMagick’s BD(convert), if installed. The
    image is returned via the IT(image) pointer.

`Plotting Text <#toc10>`_
-------------------------

Here are functions used to read and plot text strings and paragraphs.
This can be somewhat complicated since we rely on the *PostScript*
interpreter to determine the exact dimensions of text items given the
font chosen. For perfect alignment you may have to resort to calculate
offsets explicitly using **long PSL\_deftextdim**, **PSL\_set\_height**
and others and issue calculations with **PSL\_setcommand**.

**long PSL\_plottext** (**struct PSL\_CTRL** *\*P*, **double** *x*,
**double** *y*, **double** *fontsize*, **char** *\*text*, **double**
*angle*, **long** *justify*, **long** *mode*)

    The *text* is plotted starting at plot coordinates (*x*, *y*) and
    will make an *angle* with the horizontal. The point (*x*, *y*) maps
    onto different points of the text-string by giving various values
    for *justify* (see **JUSTIFICATION** for details). If *justify* is
    negative, then all leading and trailing blanks are stripped before
    plotting. Certain character sequences (flags) have special meaning
    to **PSL\_plottext**. @~ toggles between current font and the
    Mathematical Symbols font. @%\ *no*\ % selects font *no* while @%%
    resets to the previous font. @- turns subscript on/off, @+ turns
    superscript on/off, @# turns small caps on/off, and @\\ will make a
    composite character of the following two character. @;\ *r/g/b*;
    changes the font color while @;; resets it [optionally append
    =\ *transparency* to change the transparency (0--100) of the text
    (the Default is opaque or 0)], @:\ *size*: changes the font size
    (@:: resets it), and @\_ toggles underline on/off. If *text* is NULL
    then we assume **PSL\_plottextbox** was called first. Give
    *fontsize* in points. Normally, the text is typed using solid
    characters in the current color (set by **PSL\_setcolor**). To draw
    outline characters, set *mode* == 1; the outline will get the
    current color and the text is filled with the current fill color
    (set by **PSL\_setfill**). Use *mode* == 2 if the current fill is a
    pattern. If *fontsize* is negative it means that the current point
    has already been set before **PSL\_plottext** was called and that
    (*x*, *y*) should be ignored.

**long PSL\_plottextbox** (**struct PSL\_CTRL** *\*P*, **double** *x*,
**double** *y*, **double** *fontsize*, **char** *\*text*, **double**
*angle*, **long** *justify*, **double** *offset*\ [], **long** *mode*)

    This function is used in conjugation with **PSL\_plottext** when a
    box surrounding the text string is desired. Taking most of the
    arguments of **PSL\_plottext**, the user must also specify *mode* to
    indicate whether the box needs rounded (PSL\_YES = 1) or straight
    (PSL\_NO = 0) corners. The box will be colored with the current fill
    style set by **PSL\_setfill**. That means, if an outline is desired,
    and the color of the inside of the box should be set with that
    routine. The outline will be drawn with the current pen color (and
    width). The *offset* array holds the horizontal and vertical
    distance gaps between text and the surrounding text box in distance
    units. The smaller of the two determined the radius of the rounded
    corners (if requested).

**long PSL\_plottextclip** (**struct PSL\_CTRL** *\*P*, **double** *x*,
**double** *y*, **long** *n*, **double** *fontsize*, **char**
*\*text*\ [], **double** *angle*\ [], **long** *justify*, **double**
*offset*\ [], **long** *mode*)

    This function is called twice: First time we pass the text strings
    and other parameters and use *PostScript* to compute clip paths so
    that no feature plotted after this call will be visible in areas
    where text will be plotted. The second call actually plots the texts
    in the predetermined locations (NULL may be passed for all arrays
    for the second call). All labels have a straight baseline (for
    plotting along curved text, see **PSL\_plottextpath**). The *x* and
    *y* arrays contain the plot coordinates where labels will be
    plotted; there are *n* such labels and locations. Each label has its
    own entry in the *angle* array. The *text* is an array of text
    pointers to the individual text items, which will all appear using
    the current font and scaled to specified *fontsize* in points. The
    *offset* array holds the horizontal and vertical distance gaps
    between text and the surrounding text box in user units (the clip
    path is the combination of all these text boxes). Use *justify* to
    specify how the text string relates to the coordinates (see
    **JUSTIFICATION** for details). Finally, *mode* is a bit pattern
    that controls how the function does its work; pass *mode* as the sum
    of the values you need: 0 = lay down clip path, 1 = place the text,
    2 = turn off clipping, 4 = draw the *x-y* line (useful for
    debugging), 8 = reuse the previous parameters (so pass NULL as
    args), 16 = construct rounded text boxes [Default is rectangular],
    128 = fill the text box (this requires you to first define the text
    box rgb color with **PSL\_define\_rgb** by setting a local
    *PostScript* variable that must be called PSL\_setboxrgb), and 256 =
    draw the text box outlines (this requires you to first define the
    text box pen with **PSL\_define\_pen** by setting a local
    *PostScript* variable that must be called PSL\_setboxpen). For font
    color you must use **PSL\_define\_rgb** and create a *PostScript*
    variable called PSL\_settxtrgb. If not set we default to black.

**long PSL\_deftextdim** (**struct PSL\_CTRL** *\*P*, **char**
*\*prefix*, **double** *fontsize*, **char** *\*text*)

    Computes the dimensions (width and height) required by the selected
    *text* given the current font and its *fontsize* (in points). The
    values are stored as *PostScript* variables called *prefix*\ \_w and
    *prefix*\ \_h, respectively. This function can be used to compute
    dimensions and, via BF(PSL\_setcommand), calculate chances to
    position a particular item should be plotted. For instance, if you
    compute a position this way and wish to plot the text there, pass
    the coordinates to **PSL\_plottext** as NaNs. If *prefix* is BF(-w),
    BF(-h), BF(-d) or BF(-b), no *PostScript* variables will be
    assigned, but the values of width, height, depth, or both width and
    height will be left on the *PostScript* stack.

**long PSL\_setparagraph** (**struct PSL\_CTRL** *\*P*, **double**
*line\_space*, **double** *par\_width*, **long** *par\_just*)

    Initialize common settings to be used when typesetting paragraphs of
    text with **PSL\_plotparagraph**. Specify the line spacing (1 equals
    the font size) and paragraph width (in distance units). Text can be
    aligned left (PSL\_BL), centered (PSL\_BC), right (PSL\_BR), or
    justified (PSL\_JUST) and is controlled by *par\_just*.

    **long PSL\_plotparagraphbox** (**struct PSL\_CTRL** *\*P*,
    **double** *x*, **double** *y*, **double** *fontsize*, **char**
    *\*text*, **double** *angle*, **long** *justify*, **double**
    *offset*\ [], **long** *mode*)

        Computes and plots the text rectangle for a paragraph using the
        specified *fontsize* (in points). Here, *text* is an array of
        the text to be typeset, using the settings initialized by
        **PSL\_setparagraph**. The escape sequences described for
        **PSL\_plottext** can be used to modify the text. Separate text
        into several paragraphs by appending \\r to the last item in a
        paragraph. The whole text block is positioned at plot
        coordinates *x*, *y*, which is mapped to a point on the block
        specified by *justify* (see **JUSTIFICATION** for details). The
        whole block is then shifted by the amounts *shift*\ []. The box
        will be plotted using the current fill and outline settings. The
        *offset* array holds the horizontal and vertical distance gaps
        between text and the surrounding text box in distance units. Use
        *mode* to indicate whether the box should be straight
        (PSL\_RECT\_STRAIGHT = 0), rounded (PSL\_RECT\_ROUNDED = 1),
        convex (PSL\_RECT\_CONVEX = 2) or concave (PSL\_RECT\_CONCAVE =
        3).

    **long PSL\_plotparagraph** (**struct PSL\_CTRL** *\*P*, **double**
    *x*, **double** *y*, **double** *fontsize*, **char** *\*text*,
    **double** *angle*, **long** *justify*, **long** *mode*)

        Typesets paragraphs of text using the specified *fontsize* (in
        points). Here, *text* is an array of the text to be typeset,
        using the settings initialized by **PSL\_setparagraph**. The
        escape sequences described for **PSL\_plottext** can be used to
        modify the text. Separate text into several paragraphs by
        appending \\r to the last item in a paragraph. The whole text
        block is positioned at plot coordinates *x*, *y*, which is
        mapped to a point on the block specified by *justify* (see
        **JUSTIFICATION** for details). See **PSL\_plotparagraphbox**
        for laying down the surrounding text rectangle first.

    **long PSL\_plottextpath** (**struct PSL\_CTRL** *\*P*, **double**
    *x*, **double** *y*, **long** *n*, **long** *node*\ [], **double**
    *fontsize*, **char** *\*text*\ [], **long** *m*, **double**
    *angle*\ [], **long** *justify*, **double** *offset*\ [], **long**
    *mode*)

        Please text along a curved path. This function is also called
        twice: First time we pass the text strings and locations and
        *PostScript* will compute clip paths so that no features plotted
        after this call will be visible in areas where text will be
        plotted. The second call actually plots the texts in the
        predetermined locations (NULL may be passed for all arrays for
        the second call). All labels will follow the path specified by
        the plot coordinates in the *x*, *y* arrays (for plotting
        straight text with clipping, see **PSL\_plottextclip**). The
        *node* array contains the index numbers into the *x* and *y*
        arrays where each labels will be plotted; there are *n* such
        labels and node locations. Each label has its own entry in the
        *angle* array. The *text* is an array of text pointers to the
        individual text items, which will all appear using the current
        font and scaled to specified *fontsize* (in points). The
        *offset* array holds the x and y distance gaps between text and
        the surrounding text box in user units (the clip path is the
        combination of all these text boxes). Use *justify* to specify
        how the text string relates to the coordinates (see
        BF(JUSTIFICATION) for details). Finally, *mode* is a bit pattern
        that controls how the function does its work; pass *mode* as the
        sum of the values you need: 0 = lay down clip path, 1 = place
        the text, 2 = turn off clipping, 4 = draw the *x-y* line (useful
        for debugging), 8 = reuse the previous parameters (so pass NULL
        as args), 16 = construct rounded text boxes [Default is
        rectangular], 32 = set the first time **PSL\_plottextpath** is
        called (if you are placing text several times), 64 = set the
        last time **PSL\_plottextpath** is called, 128 = fill the text
        box (this requires you to first define the text box rgb color
        with **PSL\_define\_rgb** by setting a local *PostScript*
        variable that must be called PSL\_setboxrgb), and 256 = draw the
        text box outlines (this requires you to first define the text
        box pen with **PSL\_define\_pen** by setting a local
        *PostScript* variable that must be called PSL\_setboxpen). For
        font color you must use **PSL\_define\_rgb** and create a
        *PostScript* variable called PSL\_settxtrgb. If not set we
        default to black.

`Clipping <#toc11>`_
--------------------

Here are functions used to activate and deactivate clipping regions.

**long PSL\_beginclipping** (**struct PSL\_CTRL** *\*P*, **double** *x*,
**double** *y*, **long** *n*, **double** *rgb*\ [], **long** *flag*)

    Sets up a user-definable clip path as a series on *n* points with
    plot coordinates (*x*, *y*). Plotting outside this polygon will be
    clipped until **PSL\_endclipping** is called. If *rgb*\ [0] = -1 the
    inside of the path is left empty, otherwise it is filled with the
    specified color. *flag* is used to create complex clip paths
    consisting of several disconnected regions, and takes on values 0-3.
    *flag* = `PSL\_PEN\_MOVE\_ABS (1) <PSL_PEN_MOVE_ABS.html>`_ means
    this is the first path in a multisegment clip path. *flag* =
    `PSL\_PEN\_DRAW\_ABS (2) <PSL_PEN_DRAW_ABS.2.html>`_ means this is
    the last segment. Thus, for a single path, *flag* =
    `PSL\_PEN\_DRAW\_AND\_STROKE\_ABS
    (3) <PSL_PEN_DRAW_AND_STROKE_ABS.html>`_ .

**long PSL\_endclipping** (**struct PSL\_CTRL** *\*P*, **long** *mode*)

    Depending on the *mode* it restores the clip path. The *mode* values
    can be: -*n* will restore *n* levels of text-based clipping, *n*
    will restore *n* levels of polygon clipping, PSL\_ALL\_CLIP\_TXT
    will undo all levels of text-based clipping, and PSL\_ALL\_CLIP\_POL
    will undo all levels of polygon-based clipping.

`Miscellaneous Functions <#toc12>`_
-----------------------------------

Here are functions used to issue comments or to pass custom *PostScript*
commands directly to the output *PostScript* file. In C these functions
are declared as macros and they can accept a variable number of
arguments. However, from FORTRAN only a single text argument may be
passed.

**long PSL\_setcommand** (**struct PSL\_CTRL** *\*P*, **char** *\*text*)
    Writes a raw *PostScript* command to the *PostScript* output file,
    e.g., "1 setlinejoin0.

**long PSL\_comment** (**struct PSL\_CTRL** *\*P*, **char** *\*text*)
    Writes a comment (*text*) to the *PostScript* output file, e.g.,
    "Start of graph 20. The comment are prefixed with with %% .

`Authors <#toc13>`_
-------------------

Paul Wessel, School of Ocean and Earth Science and Technology,
`http://www.soest.hawaii.edu. <http://www.soest.hawaii.edu.>`_

Remko Scharroo, Altimetrics,
`http://www.altimetrics.com. <http://www.altimetrics.com.>`_

`Bugs <#toc14>`_
----------------

Caveat Emptor: The authors are **not** responsible for any disasters,
suicide attempts, or ulcers caused by correct **or** incorrect use of
**PSL**. If you find bugs, please report them to the authors by
electronic mail. Be sure to provide enough detail so that we can
recreate the problem.

References
----------

Adobe Systems Inc., 1990, *PostScript* language reference manual, 2nd
edition, Addison-Wesley, (ISBN 0-201-18127-4).
