# main -- main driver and routines for espresso
#---------------------------------------------------------------------------

CAD 	= /cad

P	= main
PSRC	= cofactor.c compl.c contain.c cubestr.c cvrin.c cvrm.c\
	  cvrmisc.c cvrout.c equiv.c espresso.c essen.c exact.c\
	  expand.c gasp.c globals.c hack.c irred.c main.c map.c opo.c\
	  pair.c primes.c reduce.c set.c setc.c sharp.c sminterf.c\
	  sparse.c unate.c verify.c
POBJ	= cofactor.o compl.o contain.o cubestr.o cvrin.o cvrm.o\
	  cvrmisc.o cvrout.o equiv.o espresso.o essen.o exact.o\
	  expand.o gasp.o globals.o hack.o irred.o main.o map.o opo.o\
	  pair.o primes.o reduce.o set.o setc.o sharp.o sminterf.o\
	  sparse.o unate.o verify.o
PHDR	= espresso.h

TARGET	= a.out

INCLUDE	= -I../mincov -I../sparse -I../util

CFLAGS	= -O $(INCLUDE)
LDFLAGS	=
LINTFLAGS = $(INCLUDE)

#---------------------------------------------------------------------------

install: lib$(P).a llib-l$(P).ln

lib$(P).a: $(POBJ)
	ar cr lib$(P).a $(POBJ)

llib-l$(P).ln: $(PSRC) $(PHDR)
	lint $(INCLUDE) -C$(P) -n $(PSRC)

$(TARGET): $(OBJ) $(LIBS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJ) $(LIBS)

lint: $(PSRC) $(PHDR) $(SRC) $(HDR)
	lint $(LINTFLAGS) $(SRC) $(PSRC) $(LINTLIBS)

clean:
	rm -f *.a *.ln *.o $(TARGET) \
	    [Tt]ags [Mm]ake.out malloc.out gmon.out lint

tags: _force
	@for i in $(PSRC) $(PHDR); do			\
	    cwd=`pwd`; ctags -a $$cwd/$$i;		\
	done;

depend:
	sed '/^#--DO NOT CHANGE ANYTHING AFTER THIS LINE/,$$d' Makefile >mktemp
	echo '#--DO NOT CHANGE ANYTHING AFTER THIS LINE' >>mktemp
	$(CAD)/bin/cc-M $(INCLUDE) $(PSRC) | \
	    sed 's|$(CAD)|$$(CAD)|g' | \
	    grep -v '/usr/include' >>mktemp
	mv mktemp Makefile

_force:
