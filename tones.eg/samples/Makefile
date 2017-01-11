#
# Makefile for jj's siggen tones.eg/samples files  .....

SHELL		= /bin/sh

# Edit SAMPLES to make the 1Hz + harmonic mixes to simulate different
#  instrument tones.
SAMPLES		= flute.wav organ.wav bassoon.wav bassoonp.wav violin.wav horn.wav att.wav

#
all:	$(SAMPLES)

%.wav:	%.tones
	../../tones -v -i $< -ch 20 -f -w $@ sin

clean:
	rm -rf $(SAMPLES) *~

