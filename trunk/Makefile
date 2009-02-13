#	$Id: Makefile,v 1.69 2009-02-13 19:54:52 remko Exp $
#
#	Copyright (c) 1991-2009 by P. Wessel and W. H. F. Smith
#	See COPYING file for copying and redistribution conditions.
#
#	This program is free software; you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation; version 2 of the License.
#
#	This program is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	Contact info: gmt.soest.hawaii.edu
#-------------------------------------------------------------------------------
#		Makefile for GMT Version 4.x
#		GNU, Sys V, and BSD Compatible
#
#	Follow the instructions in this makefile to customize your setup.
#	Commands marked *** are optional
#	1. run configure in the main GMT directory.
#	2. make all
#	3. make suppl    ***
#	4. make examples ***
#
#	Then, if you have the necessary directory permissions:
#
#	5. make install-all
#
#	or any combination of
#
#	make install-gmt
#	make install-suppl
#	make install-data
#	make install-man
#	make install-doc
#
#	When done, clean out directory with "make clean".  To clean
#	the entire build distribution, do "make distclean"
#
#	To uninstall the installed files, try
#
#	make uninstall-all
#
#	or any combination of
#
#	make uninstall-gmt
#	make uninstall-suppl
#	make uninstall-data
#	make uninstall-man
#	make uninstall-doc
#
#	Authors:	Paul Wessel, SOEST, U. of Hawaii
#			Walter H. F. Smith, Lab for Satellite Altimetry, NOAA
#
#	Date:		15-MAY-2008
#-------------------------------------------------------------------------------
#	Get Default Macros
#-------------------------------------------------------------------------------

include src/makegmt.macros	# GMT-specific settings determined by user & install_gmt

#-------------------------------------------------------------------------------
#	!! STOP EDITING HERE, THE REST IS FIXED !!
#-------------------------------------------------------------------------------
SUPPL	=	dbase gshhs imgsrc meca mex mgd77 mgg misc segyprogs spotter x2sys x_system xgrid
SUPPL_M	=	dbase imgsrc meca mgd77 mgg misc segyprogs spotter x2sys x_system

.PHONY:		all gmt suppl update gmtmacros \
		install uninstall install-all uninstall-all install-gmt uninstall-gmt \
		install-suppl uninstall-suppl install-data uninstall-data install-man uninstall-man \
		install-doc uninstall-doc examples run-examples animations run-animations clean spotless distclean $(SUPPL)

all:		gmt suppl

help::
		@grep '^#!' Makefile | cut -c3-
#!-------------------- MAKE HELP FOR GMT --------------------
#!
#!make <target>, where <target> can be:
#!
#!all           : Compile both GMT and supplements
#!gmt           : Compile main GMT source only
#!suppl         : Compile supplements only
#!install       : Compile & install GMT and supplements
#!install-gmt   : Compile & install main GMT programs
#!install-suppl : Compile & install supplements
#!install-data  : Install GMT data files
#!install-man   : Install man pages
#!install-doc	: Install PDF and HTML documentation
#!install-all   : Compile & install everything, including data & man pages
#!examples      : Run examples
#!animations    : Run animations
#!spotless      : Clean up and remove created files of all types
#!

install:	install-gmt install-suppl
uninstall:	uninstall-gmt uninstall-suppl

install-all:	install-gmt install-suppl install-data install-man install-doc
uninstall-all:	uninstall-gmt uninstall-suppl uninstall-data uninstall-man uninstall-doc

gmt:		gmtmacros
		cd src ; $(MAKE) all

install-gmt:	gmtmacros
		cd src ; $(MAKE) install

uninstall-gmt:
		cd src ; $(MAKE) uninstall

suppl:		gmtmacros
		cd src ; $(MAKE) libs
		$(MAKE) TARGET=all $(SUPPL)

install-suppl suppl-install:	gmtmacros
		cd src ; $(MAKE) libs
		$(MAKE) TARGET=install $(SUPPL)

gmtmacros:
		@if [ ! -s src/makegmt.macros ]; then \
			echo "src/makegmt.macros is empty - you must rerun configure in the main GMT directory"; \
			exit 1; \
		fi

uninstall-suppl:
		$(MAKE) TARGET=uninstall $(SUPPL)

install-data:
		@if [ ! $(rootdir)/share = $(datadir) ]; then \
			for dir in coast conf cpt custom dbase mgd77 mgg pattern pslib time x2sys; do \
				if [ -d $(rootdir)/share/$$dir ]; then \
					mkdir -p $(datadir)/$$dir; \
					\cp -p `ls -d $(rootdir)/share/$$dir/* | grep -v "\.in$$" | grep -v "CVS"` $(datadir)/$$dir ; \
				fi; \
			done; \
		else \
			echo "Install share directory the same as distribution share directory - nothing copied"; \
		fi

uninstall-data:
		@if [ ! $(rootdir)/share = $(datadir) ]; then \
			\rm -rf $(datadir); \
		else \
			echo "Install share directory the same as distribution share directory - nothing removed"; \
		fi

install-man uninstall-man:
		cd src ; $(MAKE) $@
		$(MAKE) TARGET=$@ $(SUPPL)

install-doc::
		@if [ ! $(rootdir)/share/doc/gmt = $(docdir) ]; then \
			mkdir -p $(docdir)
			cp -pr $(rootdir)/{html,pdf,examples,tutorial} $(docdir)
			rm -rf $(docdir)/*/{CVS,orig} $(docdir)/*/*/{CVS,.gmt*,*.ps,.cvs*}; \
		else \
			echo "Install doc directory the same as distribution doc directory - docs not copied"; \
		fi

uninstall-doc:
		@if [ ! $(rootdir)/share/doc = $(docdir) ]; then \
			\rm -rf $(docdir); \
		else \
			echo "Install doc directory the same as distribution doc directory - nothing deleted"; \
		fi

# Run examples with the binaries from the src directory, not the installation directory.

examples run-examples:
		@if [ -d examples ]; then \
			cd examples; sh do_examples.sh $(srcdir) $(srcdir) $(NETCDF_LIB); \
		else \
			echo "examples directory not installed"; \
		fi

animations run-animations:
		@if [ -d examples ]; then \
			cd examples; sh do_animations.sh $(srcdir) $(srcdir); \
		else \
			echo "examples directory not installed"; \
		fi

clean::
		$(MAKE) TARGET=$@ $(SUPPL)
		cd src ; $(MAKE) $@

spotless::
		\rm -f config.cache config.status config.log
		$(MAKE) TARGET=$@ $(SUPPL)
		cd src ; $(MAKE) $@

distclean:	spotless

$(SUPPL):
		@set -e; if [ -d src/$@ ] && [ ! -f src/$@/.skip ]; then \
			echo "Making $(TARGET) in src/$@"; \
			cd src/$@; $(MAKE) $(TARGET); \
		fi
