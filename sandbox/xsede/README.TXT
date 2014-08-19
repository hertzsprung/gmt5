#
# Also see http://www.soest.hawaii.edu/pwessel/xsede

This directory was set up for the 3 test cases that demonstrate GMT
usage that XSEDE might be able to speed up using parallelization. The
top level dir has the demonstration data sets topo.nc and topo.xyz

It is assumed XSEDE will be using the xsede branch of GMT which must
be installed via svn:

svn checkout svn://gmtserver.soest.hawaii.edu/branches/xsede gmt5-xsede

Follow the "Building GMT" instructions on the gmt.soest.hawaii.edu wiki
but ignore the svn command to get source from trunk since you will be
using the xsede branch instead.

Case 1 [Directory 1]:
  Demonstrates use of greenspline which involves two labor-intensive
  tasks: a) We must solve a Ax = b system that in this case is 1000x1000.
  Currently, this is done using a Gauss-Jordan approach.  Alternatively,
  for a relaxed fit we do a SVD and only use some of the eigenvalues.
  b) Given the solution x, we must evaluate a loop over all Green's functions
  for each output node.  This can be done in parallel with Open MP.

Case 2 [Directory 2]
  Demonstrate use of grdfilter which is a space-domain filtering tool.
  First example uses a Gaussian 100 km-diameter filter while the second
  and slower example uses a 100 km-diameter median filter. You can see
  in grdfilter.c that there have been attempts at doing Open MP but did
  not finish yet.

Case 3 [Directory 3]
  Demonstrate use of surface which is a gridder using finite difference
  solution to the biharmonic equation on a grid.  The first example uses
  the default values while the second tightens the convergence criteria
  and adds a bit of over-relaxation.  We don't know how to make this go
  fast in parallel.

For more information contact Paul Wessel at the University of Hawaii:
	pwessel@hawaii.edu, 808-956-4778

