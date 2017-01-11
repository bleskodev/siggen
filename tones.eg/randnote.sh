#! /bin/sh
#
# demo to show use of randtone

tones=../tones

./randnote c4 250 100 | $tones -v -i - sin 100
