# $Id$
#
# epd -- extended precision
#---------------------------------------------------------------------------
.SUFFIXES: .c .o .u

CC	= gcc
RANLIB	= ranlib

MFLAG	=
ICFLAGS = -g -O6 -Wall
CFLAGS  = $(ICFLAGS) $(MFLAG) $(XCFLAGS)

LINTFLAGS = -u -n

# this is to create the lint library
LINTSWITCH = -o

P	= epd
PSRC	= epd.c
PHDR	= epd.h
POBJ	= epd.o
PUBJ	= epd.u

WHERE	= ..
INCLUDE = $(WHERE)/include

lib$(P).a: $(POBJ)
	ar rv lib$(P).a $(POBJ)
	$(RANLIB) lib$(P).a

.c.o: $(PSRC) $(PHDR)
	$(CC) -c $< -I$(INCLUDE) $(CFLAGS) $(DEBUG)

optimize_dec: lib$(P).b

lib$(P).b: $(PUBJ)
	ar rv lib$(P).b $(PUBJ)
	$(RANLIB) lib$(P).b

.c.u: $(PSRC) $(PHDR)
	cc -j $< -I$(INCLUDE) $(XCFLAGS)

lint: llib-l$(P).ln

llib-l$(P).ln: $(PSRC) $(PHDR)
	lint $(LINTFLAGS) $(LINTSWITCH)$(P) -I$(INCLUDE) $(PSRC)

tags: $(PSRC) $(PHDR)
	ctags $(PSRC) $(PHDR)

all: lib$(P).a lib$(P).b llib-l$(P).ln tags

clean:
	rm -f *.o *.u .pure *.warnings

distclean: clean
	rm -f lib*.a lib$(P).b llib-l$(P).ln tags *~ *.bak *.qv *.qx
