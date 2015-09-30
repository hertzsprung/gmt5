function [ps, path] = decoratedlines(out_path)
% OUT_PATH -> Path to where the PS file will be written
% PS       -> Full name of the created postscript file (built from OUT_PATH)
% PATH     -> Path to where this file lives (usefull for gmtest.m)
%
%	$Id$

	full = mfilename('fullpath');
	[pato, fname] = fileparts(full);
	ps = [out_path fname '.ps'];
	path = [pato filesep];

	gmt('destroy'),		gmt('gmtset -Du'),	gmt('destroy')		% Make sure we start with a clean session

	fid = fopen('vert.txt', 'w');
	fprintf(fid, '90 0\n90 6\n');
	fclose(fid);

	fid = fopen('data.txt', 'w+');
	fprintf(fid, '> The first curve\n');
	t = gmt('gmtmath -T0/180/1 T SIND =');				fprintf(fid, '%g %f\n', t');
	fprintf(fid, '> The second curve -S_n5:+ss0.5i+p0.25p+gcyan\n');
	t = gmt('gmtmath -T0/180/1 T SIND 1 ADD =');		fprintf(fid, '%g %f\n', t');
	fprintf(fid, '> The third curve -S_N15:+sc0.1i+gblue+a0\n');
	t = gmt('gmtmath -T0/180/1 T SIND 2 ADD =');		fprintf(fid, '%g %f\n', t');
	fprintf(fid, '> The fourth curve -S_lLM/RM:+sn0.3i+gblack\n');
	t = gmt('gmtmath -T0/180/1 T SIND 2.5 ADD =');		fprintf(fid, '%g %f\n', t');
	fprintf(fid, '> The fifth curve -S_xvert.txt:+si0.3i+p1p+ggreen -W2p\n');
	t = gmt('gmtmath -T0/180/1 T SIND 3 ADD =');		fprintf(fid, '%g %f\n', t');
	fprintf(fid, '> The sixth curve -S_N15:+sd0.1i+p0.25p,red -Wfaint\n');
	t = gmt('gmtmath -T0/180/1 T SIND 4 ADD =');		fprintf(fid, '%g %f\n', t');
	fprintf(fid, '> The seventh curve -S_n5:+ss0.5i+p0.25p+gpurple+a0 -W1p,orange\n');
	t = gmt('gmtmath -T0/180/1 T SIND 4.5 ADD =');		fprintf(fid, '%g %f\n', t');
	gmt(['psxy -R-5/185/-0.1/6 -JX6i/9i -P -Baf -W1p,red -S_n3:+sa0.5i+p0.25p,green+gblue data.txt > ' ps])
	fclose(fid);
	
	builtin('delete','gmt.conf', 'data.txt', 'vert.txt');