#!/bin/bash
#
# $Id$
#
# Purpose:      Test all PSL functions at least once
# GMT progs:    libpslib, psldemo
# Unix progs:   -
#
header "Test pslib capabilities"
psldemo > $ps
pscmp
