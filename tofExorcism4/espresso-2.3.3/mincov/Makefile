# mincov -- weighted minimum cover package
#---------------------------------------------------------------------------

CAD 	= /cad

P	= mincov
PSRC	= mincov.c solution.c gimpel.c indep.c dominate.c part.c
POBJ	= mincov.o solution.o gimpel.o indep.o dominate.o part.o
PHDR	= mincov.h

TARGET	= mincov
SRC	= main.c
OBJ	= main.o
HDR	=

INCLUDE	= -I../sparse -I../util

LIBS	= ../sparse/libsparse.a 	\
	  ../util/libutil.a

LINTLIBS= ../sparse/llib-lsparse.ln 	\
	  ../util/llib-lutil.ln 

MISC	= Makefile mincov_int.h pi/sqr6.pi pi/Z5xp1.pi pi/mlp4.pi

CFLAGS	= -O $(INCLUDE)
LDFLAGS	=
LINTFLAGS = $(INCLUDE)

#---------------------------------------------------------------------------

install: lib$(P).a llib-l$(P).ln

lib$(P).a: $(POBJ)
	ar cr lib$(P).a $(POBJ)
	ranlib lib$(P).a

llib-l$(P).ln: $(PSRC) $(PHDR)
	lint $(INCLUDE) -C$(P) -n $(PSRC)

$(TARGET): $(OBJ) $(POBJ) $(LIBS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJ) $(POBJ) $(LIBS)

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
