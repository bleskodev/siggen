#!/bin/sh
# demo prog to test a base sample set......
#

f=hole_bucket.notes
if [ "$1" = "" ]; then
  echo "Usage: $0 SAMPLE_NAME"
  echo " where samples are in the file ./samples/SAMPLE_NAME.wav"
  exit -1
fi

while [ "$1" != "" ]; do
 ../tones -load samples/$1.wav $1 $f
 shift
done


