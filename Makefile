#
# Makefile for jj's siggen  .....

SHELL		= /bin/sh

# Version of siggen

V	= 2.3

# Edit PROGS to make the programs you want. You may wish to omit smix
#  if you do not want yet another mixer program.
PROGS		= tones sgen swgen siggen sweepgen fsynth soundinfo smix

#
TEXTS		= tones.txt sgen.txt swgen.txt siggen.txt sweepgen.txt soundinfo.txt smix.txt fsynth.txt siggen.conf.txt

#
# simple command line programs......
TONES		= tones.o tonesgen.o generator.o misc.o wavfile.o wavsubs.o DAC.o configsubs.o
SGEN		= sgen.o generator.o misc.o wavfile.o wavsubs.o DAC.o configsubs.o
SWGEN		= swgen.o generator.o misc.o wavfile.o wavsubs.o DAC.o configsubs.o
SMIX		= smix.o mixer.o configsubs.o
#
# curses based programs......
FSYNTH		= fsynth.o fsynscr.o scrsubs.o generator.o misc.o DAC.o scfio.o configsubs.o
SIGGEN		= siggen.o sigscr.o scrsubs.o generator.o misc.o DAC.o scfio.o configsubs.o
SWEEPGEN	= sweepgen.o sweepscr.o scrsubs.o generator.o misc.o DAC.o scfio.o configsubs.o
#
srcdir		= .
includedir	= /usr/include/ncurses
INSDIR		= /usr/local/bin
MANDIR		= /usr/local/man
LOCALINS	= $(HOME)/bin
LOCALMAN	= $(HOME)/man

CC		= gcc
CFLAGS		=  -O2  
CPPFLAGS	= -I. -I$(includedir)

CCFLAGS		= $(CFLAGS) $(CPPFLAGS)

LINK		= $(CC)
LDFLAGS		= -lncurses -lm

.c.o:	config.h
	$(CC) -c $(CCFLAGS) $<

all:	$(PROGS) $(TEXTS) 

text:	$(TEXTS)

%.txt:	%.5
	nroff -man $< | col -b -x > $@

%.txt:	%.1
	nroff -man $< | col -b -x > $@

mixer.o:	mixer.c mixer.h

smix.o:		smix.c mixer.h config.h

scfio.o:	scfio.c scfio.h

scrsubs.o:	scrsubs.c scfio.h config.h

sigscr.o:	sigscr.c scfio.h config.h siggen.h

sigscr-1.o:	sigscr-1.c scfio.h config.h

sweepscr.o:	sweepscr.c scfio.h config.h sweepgen.h

fsynscr.o:	fsynscr.c scfio.h fsynth.h config.h

fsynth.o:	fsynth.c fsynth.h config.h

install:
	@echo "2 install options :-"
	@echo "    make sysinstall"
	@echo "         into $(INSDIR) and $(MANDIR)"
	@echo "    make localinstall"
	@echo "         into $(LOCALINS) and $(LOCALMAN)"

localinstall: $(PROGS)
	@strip $(PROGS)
	@chmod 755 $(PROGS)
	@echo "Copying $(PROGS) to $(LOCALINS)"
	@cp -p $(PROGS) $(LOCALINS)
	@for n in $(PROGS) ; do \
	chmod 644 $$n.1 ; \
	echo "Copying $$n.1 to $(LOCALMAN)/man1/$$n.1" ; \
	cp -p $$n.1 $(LOCALMAN)/man1/$$n.1 ; \
	done
	@cp -p siggen.conf.5 $(LOCALMAN)/man5
	@chmod 644 $(LOCALMAN)/man5/siggen.conf.5

sysinstall: $(PROGS)
	@strip $(PROGS)
	@chmod 755 $(PROGS)
	@echo "Copying $(PROGS) to $(INSDIR)"
	@cp -p $(PROGS) $(INSDIR)
	@for n in $(PROGS) ; do \
	chmod 644 $$n.1 ; \
	echo "Copying $$n.1 to $(MANDIR)/man1/$$n.1" ; \
	cp -p $$n.1 $(MANDIR)/man1/$$n.1 ; \
	done
	@cp -p siggen.conf.5 $(MANDIR)/man5
	@chmod 644 $(MANDIR)/man5/siggen.conf.5

nodac:
	make -f Makefile.NODAC all

vu:	$(VU)
	$(CC)  $(VU) -o $@

soundinfo:	soundinfo.o
	$(CC)  $@.o -o $@

sgen:	$(SGEN)
	$(CC)  $(SGEN) -lm -o $@

swgen:	$(SWGEN)
	$(CC)  $(SWGEN) -lm -o $@

tones:	$(TONES)
	$(CC)  $(TONES) -lm -o $@

fsynth:	$(FSYNTH) fsynth.h
	$(CC) $(FSYNTH) $(LDFLAGS) -o $@

siggen:	$(SIGGEN) siggen.h
	$(CC) $(SIGGEN) $(LDFLAGS) -o $@

siggen-1: $(SIGGEN1) siggen.h
	$(CC) -DVERSION1 $(SIGGEN1) $(LDFLAGS) -o $@

sweepgen: $(SWEEPGEN) sweepgen.h
	$(CC) $(SWEEPGEN) $(LDFLAGS) -o $@

smix:	$(SMIX) mixer.h
	$(CC) $(SMIX) $(LDFLAGS) -o $@

clean:
	rm -rf *.o $(PROGS) $(TEXTS) *~ 

dist:
	make clean
	(cd tones.eg; make clean)
	(cd contrib; make clean)
	(d=`basename $$PWD` ; cd .. ; tar cfz $$d.tgz $$d)

