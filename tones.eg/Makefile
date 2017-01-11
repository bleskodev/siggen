#
# Makefile for jj's siggen tones.eg files  .....

SHELL		= /bin/sh

# Edit PROGS to make the programs you want. You may wish to omit smix
#  if you do not want yet another mixer program.
PROGS		= randtone randnote ramp shepard-risset

# Edit SAMPLES to make the 1Hz + harmonic mixes to simulate different
#  instrument tones.
SAMPLES		= flute organ

#
srcdir		= .
includedir	= /usr/include/ncurses
INSDIR		= /usr/local/bin
MANDIR		= /usr/local/man/man1
LOCALINS	= $(HOME)/bin
LOCALMAN	= $(HOME)/man/man1

CC		= gcc
CFLAGS		=  -O2  
CPPFLAGS	= -I. -I$(includedir)

CCFLAGS		= $(CFLAGS) $(CPPFLAGS)

LINK		= $(CC)
LDFLAGS		= -lncurses -lgpm -lm

.c.o:
	$(CC) -c $(CCFLAGS) $<

all:	$(PROGS)
	(cd samples ; make )

%.txt:	%.1
	nroff -man $< | col -b -x > $@

randtone:	randtone.o
	$(CC)  $@.o -o $@

randnote:	randnote.o
	$(CC)  $@.o -o $@

ramp:	ramp.o
	$(CC)  $@.o -o $@

shepard-risset:	shepard-risset.o
	$(CC)  $@.o -o $@ -lm

flute:	$@.notes
	../tones -ch 20 -i $@.tones -w samples/$@.wav -v

organ:	
	../tones -ch 20 -i $@.tones -w samples/$@.wav -v

clean:
	rm -rf *.o $(PROGS) *~
	( cd samples ; make clean )
