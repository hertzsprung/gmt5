function [ps, path] = oblsuite(out_path)
% OUT_PATH -> Path to where the PS file will be written
% PS       -> Full name of the created postscript file (built from OUT_PATH)
% PATH     -> Path to where this file lives (usefull for gmtest.m)
%
%	Same as oblsuite_N.sh but with no hemisphere restriction on where the pole is
%	$Id$

	full = mfilename('fullpath');
	[pato, fname] = fileparts(full);
	ps = [out_path fname '.ps'];
	path = [pato filesep];

	gmt('destroy'),		gmt('gmtset -Du'),	gmt('destroy')		% Make sure we start with a clean session
	gmt(['pscoast -Rk-2000/2000/-1000/1000 -JoA-30/60/-180/1:60000000 -Ba0fg -P -Gred -K -X1.25i -Y9i > ' ps])
	gmt(['pscoast -Rk-2000/2000/-1000/1000 -JoA-30/60/-150/1:60000000 -Ba0fg -O -K -Gred -Y-1.7i >> ' ps])
	gmt(['pscoast -Rk-2000/2000/-1000/1000 -JoA-30/60/-120/1:60000000 -Ba0fg -O -K -Gred -Y-1.7i >> ' ps])
	gmt(['pscoast -Rk-2000/2000/-1000/1000 -JoA-30/60/-90/1:60000000  -Ba0fg -O -K -Gred -Y-1.7i >> ' ps])
	gmt(['pscoast -Rk-2000/2000/-1000/1000 -JoA-30/60/-60/1:60000000  -Ba0fg -O -K -Gred -Y-1.7i >> ' ps])
	gmt(['pscoast -Rk-2000/2000/-1000/1000 -JoA-30/60/-30/1:60000000  -Ba0fg -O -K -Gred -Y-1.7i >> ' ps])
	gmt(['pscoast -Rk-2000/2000/-1000/1000 -JoA-30/60/0/1:60000000    -Ba0fg -O -K -Gred -X3.45i -Y8.5i >> ' ps])
	gmt(['pscoast -Rk-2000/2000/-1000/1000 -JoA-30/60/30/1:60000000   -Ba0fg -O -K -Gred -Y-1.7i >> ' ps])
	gmt(['pscoast -Rk-2000/2000/-1000/1000 -JoA-30/60/60/1:60000000   -Ba0fg -O -K -Gred -Y-1.7i >> ' ps])
	gmt(['pscoast -Rk-2000/2000/-1000/1000 -JoA-30/60/90/1:60000000   -Ba0fg -O -K -Gred -Y-1.7i >> ' ps])
	gmt(['pscoast -Rk-2000/2000/-1000/1000 -JoA-30/60/120/1:60000000  -Ba0fg -O -K -Gred -Y-1.7i >> ' ps])
	gmt(['pscoast -Rk-2000/2000/-1000/1000 -JoA-30/60/150/1:60000000  -Ba0fg -O -K -Gred -Y-1.7i >> ' ps])
	gmt(['pstext  -R0/8/0/10 -Jx1i -F+f14p+jRM -O -K -X-3.45i -N >> ' ps], {
		'-0.1 0.7 az: -30'
		'-0.1 2.4 az: -60'
		'-0.1 4.1 az: -90'
		'-0.1 5.8 az: -120'
		'-0.1 7.5 az: -140'
		'-0.1 9.2 az: -180'})
	gmt(['pstext -R0/8/0/10 -Jx1i -F+f14p+jLM -O -K -N >> ' ps], {
		'6.2 0.7 az: 150'
		'6.2 2.4 az: 120'
		'6.2 4.1 az: 90'
		'6.2 5.8 az: 60'
		'6.2 7.5 az: 30'
		'6.2 9.2 az: 0'})

	gmt(['psxy -R -J -O -T >> ' ps])
	builtin('delete','gmt.conf');
