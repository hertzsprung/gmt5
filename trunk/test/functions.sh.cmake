#
# $Id$
#
# Functions to be used with test scripts

# Print the shell script name and purpose and fill out to 72 characters
# and make sure to use US system defaults
header () {
  printf "%-72s\n" "$0: $1"
}

# Convert PS to PDF
function make_pdf()
{
  psfile="${1:-$ps}"
  pdfile="${psfile%.ps}.pdf"
  test -f "${psfile}" || return 1
  ps2raster -Tf -A -P -Ggs "${psfile}" || ((++ERROR))
  test -f "${pdfile}" || ((++ERROR))
}

# Compare the ps file with its original. Check $1.ps (if $1 given) or $ps
pscmp () {
  f=${1:-$(basename $ps .ps)}
  d=$(basename $PWD)
  if ! [ -x "$GRAPHICSMAGICK" ]; then
    echo "[PASS] (without comparison)"
    return
  fi
  # syntax: gm compare [ options ... ] reference-image [ options ... ] compare-image [ options ... ]
  rms=$(${GRAPHICSMAGICK} compare -density 200 -maximum-error 0.001 -highlight-color magenta -highlight-style assign -metric rmse -file ${f}.png $src/${f}.ps ${f}.ps) || pscmpfailed="yes"
  rms=$(sed -nE '/Total:/s/ +Total: ([0-9.]+) .+/\1/p' <<< "$rms")
  if [ -z "$rms" ]; then
    rms="NA"
  else
    rms=$(printf "%.3f\n" $rms)
  fi
  if [ "$pscmpfailed" ]; then
    now=$(date "+%F %T")
    echo "RMS Error = $rms [FAIL]"
    echo "$now ${d}/${f}: RMS Error = $rms" >> ../fail_count.d
    make_pdf ${1:-$ps} # try to make pdf file
    ((++ERROR))
  else
    test -z "$rms" && rms=NA
    echo "RMS Error = $rms [PASS]"
  fi
}

passfail () {
  if [ -s fail ]; then
    now=$(date "+%F %T")
    echo "[FAIL]"
    echo "$now $d/$1: $(wc -l fail)ed lines" >> ../fail_count.d
    mv -f fail $1.log
    ((++ERROR))
  else
    echo "[PASS]"
  fi
}

# Temporary change LANG to C
LANG=C

# Use executables from GMT_BINARY_DIR, fallback to CMAKE_INSTALL_PREFIX/GMT_BINDIR
export GMT_BINARY_DIR="@GMT_BINARY_DIR@"
export GMT_SOURCE_DIR="@GMT_SOURCE_DIR@"
export PATH="@GMT_BINARY_DIR_PATH@:@GMT_SOURCE_DIR@/src:@CMAKE_INSTALL_PREFIX@/@GMT_BINDIR@:${PATH}"
export GMT_SHAREDIR="@GMT_SOURCE_DIR@/share"
export GMT_USERDIR="@GMT_BINARY_DIR@/share"
export HAVE_GMT_DEBUG_SYMBOLS="@HAVE_GMT_DEBUG_SYMBOLS@"
export HAVE_OPENMP="@HAVE_OPENMP@"
export GRAPHICSMAGICK="@GRAPHICSMAGICK@"

# Reset error count
ERROR=0

# Make sure to cleanup at end
function on_exit()
{
  set +e
  trap - EXIT # Restore EXIT trap
  echo "exit status: ${ERROR}"
  [[ ${ERROR} == 0 ]] && rm -rf $dest
  exit ${ERROR}
}
trap on_exit EXIT

# Catch other errors
set -e
function on_err()
{
  set +e
  trap - EXIT ERR SIGSEGV SIGTRAP SIGBUS # Restore trap
  ((++ERROR))
  on_exit
}
trap on_err ERR SIGSEGV SIGTRAP SIGBUS

# Create a temporary directory on the binary side
dir=`dirname $0`
testdir=test/`basename $dir`
export src=$GMT_SOURCE_DIR/$testdir
export dest=$GMT_BINARY_DIR/$testdir/`basename $0 .sh`
mkdir -p $dest
cd $dest

# Start with proper GMT defaults
gmtset -Du

# vim: ft=sh
