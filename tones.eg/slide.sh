#!/bin/sh
#
# Demo run to create and then play a shepard-risset sliding scale.....

r=shepard-risset-rising.tones
f=shepard-risset-falling.tones
#echo "creating rising-slide tones specification file at $t  ....."
#./shepard-risset 700 7 1.2 15 -50 > $t
echo "Now playing ./$r and ./$f ........"
../tones -c 9 :200 sin ./$r ./$f
# use this line with verbose output....
#tones -c 9 -v :200 ./$r ./$f

