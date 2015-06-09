# Makefile for xsede test dir
tar:
	rm -f 1/*.pdf 2/*.pdf 3/*.pdf
	cd .. ;\
	COPYFILE_DISABLE=true gnutar --owner 0 --group 0 --mode a=rX,u=rwX -czf gmt-xsede.tgz \
		xsede/1 xsede/2 xsede/3 xsede/topo* xsede/README.TXT
	chmod og+r ../gmt-xsede.tgz

place:
	scp ../gmt-xsede.tgz imina.soest.hawaii.edu:/export/imina2/httpd/htdocs/pwessel/xsede
	scp index.html imina.soest.hawaii.edu:/export/imina2/httpd/htdocs/pwessel/xsede
