#!/bin/sh
# demo using ramp
#

tones="../tones -load samples/flute.wav -load samples/organ.wav"
f1=hole_bucket.notes
f2=clementine.notes

echo "there's a hole in my bucket - pure sine tones....."
$tones -i $f1 sin
echo "there's a hole in my bucket - using tone with harmonics....."
$tones -i $f1 flute

echo " "
sleep 1
echo and now one with harmony......
$tones -i $f2 flute,sin,tri,sin

