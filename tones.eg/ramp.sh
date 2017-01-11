#!/bin/sh
# demo using ramp
#

tones=../tones

echo "executing: ./ramp 300 800 20 10 | $tones -i - sq 20"
./ramp 300 800 20 10 | $tones -i - sq 20

echo "executing: (./ramp 300 800 5 1 ; ./ramp 800 300 5 1) | $tones -i - sin 20"
(./ramp 300 800 5 1 ; ./ramp 800 300 5 1) | $tones -i - sin 20

echo "executing: ./ramp 200 1000 5 4 | $tones -i - sin 10"
./ramp 200 1000 5 4 | $tones -i - sin 10

