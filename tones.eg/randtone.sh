#! /bin/sh
#
# demo to show use of randtone

tones=../tones

./randtone 500 250 100 | $tones -i - sin 100
