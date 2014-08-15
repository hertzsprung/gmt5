# Makefile for sdsc test dir
tar:
	rm -f 1/*.pdf 2/*.pdf 3/*.pdf
	cd .. ;\
	COPYFILE_DISABLE=true gnutar --owner 0 --group 0 --mode a=rX,u=rwX -czf gmt-sdsc.tgz \
		sdsc/1 sdsc/2 sdsc/3 sdsc/topo.* sdsc/README.TXT
	chmod og+r ../gmt-sdsc.tgz

place:
	scp ../gmt-sdsc.tgz imina.soest.hawaii.edu:/export/imina2/httpd/htdocs/pwessel/sdsc
	scp index.html imina.soest.hawaii.edu:/export/imina2/httpd/htdocs/pwessel/sdsc
