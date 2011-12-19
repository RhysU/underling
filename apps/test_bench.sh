#!/bin/bash
set -eu

# Initialize test infrastructure
source "`dirname $0`/test_setup.sh"

: ${FIELD:=--field-global=19x23x29}
: ${CHECK:=--check=1e-15}
: ${PATIENCE:=--estimate}
: ${FILTER:=tee -a $testdir/output | grep error}

# Shorthand
bench="./underling_bench $PATIENCE"

# Run each test case in this file under the following circumstances
# (which can be overridden by providing the environment variable METACASES).
for METACASE in ${METACASES:= 'NP=1' } #FIXME 'NP=3;P=--dims=3x0' 'NP=3;P=--dims=0x3' 'NP=4'}
do

NP=
P=
eval "$METACASE"

banner "Transposes only"
(
    prun $bench $FIELD $P $CHECK --howmany=1 | tee -a $testdir/output | grep error
)

done
