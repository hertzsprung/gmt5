#!/bin/bash
#
# $Id$
#
# Purpose:      Test all PSL functions at least once
# GMT progs:    libpostscriptlight, psldemo
# Unix progs:   -
#
ps=psldemo.ps
export PSL_SHAREDIR="$GMT_SHAREDIR"
psldemo > $ps
