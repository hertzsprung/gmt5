******
grdfft
******

grdfft - Do mathematical operations on grids in the wavenumber (or
frequency) domain

`Synopsis <#toc1>`_
-------------------

**grdfft** *ingrid* [ *ingrid2* ] **-G**\ *outfile* [ **-A**\ *azimuth*
] [ **-C**\ *zlevel* ] [ **-D**\ [*scale*\ \|\ **g**] ] [
**-E**\ [**r**\ \|\ **x**\ \|\ **y**][\ **w**\ [**k**\ ]] ] [
**-F**\ [**r**\ \|\ **x**\ \|\ **y**]\ *params* ] [
**-I**\ [*scale*\ \|\ **g**] ] [ **-L**\ [**h**\ \|\ **m**] ] [
**-N**\ [**f**\ \|\ **q**\ \|\ **s**\ \|\ *nx/ny*][\ **+e**\ \|\ **n**\ \|\ **m**][\ **+t**\ *width*][\ **+w**\ [*suffix*\ ]][\ **+z**\ [**p**\ ]]
] [ **-S**\ *scale* ] [ **-V**\ [*level*\ ] ] [
**-fg**\ ]

`Description <#toc2>`_
----------------------

**grdfft** will take the 2-D forward Fast Fourier Transform and perform
one or more mathematical operations in the frequency domain before
transforming back to the space domain. An option is provided to scale
the data before writing the new values to an output file. The horizontal
dimensions of the grid are assumed to be in meters. Geographical grids
may be used by specifying the **-fg** option that scales degrees to
meters. If you have grids with dimensions in km, you could change this
to meters using **grdedit** or scale the output with **grdmath**. 

.. include:: explain_commonitems.rst_

`Required Arguments <#toc4>`_
-----------------------------

*ingrid*
    2-D binary grid file to be operated on. (See GRID FILE FORMATS
    below). For cross-spectral operations, also give the second grid
    file *ingrd2*.
**-G**\ *outfile*
    Specify the name of the output grid file or the 1-D spectrum table
    (see **-E**). (See GRID FILE FORMATS below).

`Optional Arguments <#toc5>`_
-----------------------------

**-A**\ *azimuth*
    Take the directional derivative in the *azimuth* direction measured
    in degrees CW from north.
**-C**\ *zlevel*
    Upward (for *zlevel* > 0) or downward (for *zlevel* < 0) continue
    the field *zlevel* meters.
**-D**\ [*scale*\ \|\ **g**]
    Differentiate the field, i.e., take d(field)/dz. This is equivalent
    to multiplying by kr in the frequency domain (kr is radial wave
    number). Append a scale to multiply by (kr \* *scale*) instead.
    Alternatively, append **g** to indicate that your data are geoid
    heights in meters and output should be gravity anomalies in mGal.
    [Default is no scale].
**-E**\ [**r**\ \|\ **x**\ \|\ **y**][\ **w**\ [**k**\ ]]
    Estimate power spectrum in the radial direction [**r**\ ]. Place
    **x** or **y** immediately after **-E** to compute the spectrum in
    the x or y direction instead. No grid file is created. If one grid
    is given then f (i.e., frequency or wave number), power[f],
    and 1 standard deviation in power[f] are written to the file set by
    **-G** [stdout]. If two grids are given we write f and 8 quantities:
    Xpower[f], Ypower[f], coherent power[f], noise power[f], phase[f],
    admittance[f], gain[f], coherency[f].  Each quantity is followed by
    its own 1-std dev error estimate, hence the output is 17 columns wide.
    Append **w** to write wavelength instead of frequency. If your grid
    is geographic you may further append **k** to scale wavelengths from
    meter [Default] to km.
**-F**\ [**r**\ \|\ **x**\ \|\ **y**]\ *params*
    Filter the data. Place **x** or **y** immediately after **-F** to
    filter *x* or *y* direction only; default is isotropic [**r**\ ].
    Choose between a cosine-tapered band-pass, a Gaussian band-pass
    filter, or a Butterworth band-pass filter. Cosine-taper: Specify
    four wavelengths *lc*/*lp*/*hp*/*hc* in correct units (see **-fg**)
    to design a bandpass filter: wavelengths greater than *lc* or less
    than *hc* will be cut, wavelengths greater than *lp* and less than
    *hp* will be passed, and wavelengths in between will be
    cosine-tapered. E.g., **-F**\ 1000000/250000/50000/10000 **-fg**
    will bandpass, cutting wavelengths > 1000 km and < 10 km, passing
    wavelengths between 250 km and 50 km. To make a highpass or lowpass
    filter, give hyphens (-) for *hp*/*hc* or *lc*/*lp*. E.g.,
    **-Fx**-/-/50/10 will lowpass *x*, passing wavelengths > 50 and
    rejecting wavelengths < 10. **-Fy**\ 1000/250/-/- will highpass *y*,
    passing wavelengths < 250 and rejecting wavelengths > 1000. Gaussian
    band-pass: Append *lo*/*hi*, the two wavelengths in correct units
    (see **-fg**) to design a bandpass filter. At the given wavelengths
    the Gaussian filter weights will be 0.5. To make a highpass or
    lowpass filter, give a hyphen (-) for the *hi* or *lo* wavelength,
    respectively. E.g., **-F**-/30 will lowpass the data using a
    Gaussian filter with half-weight at 30, while **-F**\ 400/- will
    highpass the data. Butterworth band-pass: Append *lo*/*hi*/*order*,
    the two wavelengths in correct units (see **-fg**) and the filter
    order (an integer) to design a bandpass filter. At the given
    wavelengths the Butterworth filter weights will be 0.5. To make a
    highpass or lowpass filter, give a hyphen (-) for the *hi* or *lo*
    wavelength, respectively. E.g., **-F**-/30/2 will lowpass the data
    using a 2nd-order Butterworth filter, with half-weight at 30, while
    **-F**\ 400/-/2 will highpass the data.
**-I**\ [*scale*\ \|\ **g**]
    Integrate the field, i.e., compute integral\_over\_z (field \* dz).
    This is equivalent to divide by kr in the frequency domain (kr is
    radial wave number). Append a scale to divide by (kr \* *scale*)
    instead. Alternatively, append **g** to indicate that your data set
    is gravity anomalies in mGal and output should be geoid heights in
    meters. [Default is no scale].
**-L**
    Leave trend alone. By default, a linear trend will be removed prior
    to the transform. Alternatively, append **m** to just remove the
    mean value or **h** to remove the mid-value. 

.. include:: explain_fft.rst_

**-S**\ *scale*
    Multiply each element by *scale* in the space domain (after the
    frequency domain operations). [Default is 1.0]. 

.. |Add_-V| unicode:: 0x20 .. just an invisible code
.. include:: explain_-V.rst_

**-fg**
   Geographic grids (dimensions of longitude, latitude) will be converted to
   meters via a "Flat Earth" approximation using the current ellipsoid parameters.

.. include:: explain_help.rst_

.. include:: explain_grd_inout.rst_

Grid Distance Units
-------------------

If the grid does not have meter as the horizontal unit, append **+u**\ *unit* to the input file name to convert from the
specified unit to meter.  If your grid is geographic, convert distances to meters by supplying **-fg** instead.

Considerations
--------------

netCDF COARDS grids will automatically be recognized as geographic. For
other grids geographical grids were you want to convert degrees into
meters, select **-fg**. If the data are close to either pole, you should
consider projecting the grid file onto a rectangular coordinate system
using `grdproject. <grdproject.html>`_

Examples
--------

To upward continue the sea-level magnetic anomalies in the file
mag\_0.nc to a level 800 m above sealevel:

grdfft mag\_0.nc -C800 -V -Gmag\_800.nc

To transform geoid heights in m (geoid.nc) on a geographical grid to
free-air gravity anomalies in mGal:

grdfft geoid.nc -Dg -V -Ggrav.nc

To transform gravity anomalies in mGal (faa.nc) to deflections of the
vertical (in micro-radians) in the 038 direction, we must first
integrate gravity to get geoid, then take the directional derivative,
and finally scale radians to micro-radians:

grdfft faa.nc -Ig -A38 -S1e6 -V -Gdefl\_38.nc

Second vertical derivatives of gravity anomalies are related to the
curvature of the field. We can compute these as mGal/m^2 by
differentiating twice:

grdfft gravity.nc -D -D -V -Ggrav\_2nd\_derivative.nc

To compute cross-spectral estimates for co-registered bathymetry and
gravity grids, and report result as functions of wavelengths in km, try
br

grdfft bathymetry.nc gravity.grd -Ewk -fg -V > cross\_spectra.txt

To examine the pre-FFT grid after detrending, point-symmetry reflection,
and tapering has been applied, as well as saving the real and imaginary
components of the raw spectrum of the data in topo.nc, try

grdfft topo.nc -N+w+z -fg -V

You can now make plots of the data in topo\_taper.nc, topo\_real.nc, and
topo\_imag.nc.

See Also
--------

`gmt <gmt.html>`_ , `grdedit <grdedit.html>`_ ,
`grdmath <grdmath.html>`_ , `grdproject <grdproject.html>`_
