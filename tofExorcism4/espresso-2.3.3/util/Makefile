# util -- miscellaneous utility routines
#---------------------------------------------------------------------------

P	= util
PSRC	= cpu_time.c cpu_stats.c getopt.c safe_mem.c strsav.c texpand.c \
	  ptime.c prtime.c pipefork.c pathsearch.c stub.c \
	  tmpfile.c
POBJ	= cpu_time.o cpu_stats.o getopt.o safe_mem.o strsav.o texpand.o \
	  ptime.o prtime.o pipefork.o pathsearch.o stub.o \
	  tmpfile.o
PHDR	= util.h

TARGET	= test-restart
SRC	= test-restart.c
OBJ	= test-restart.o
HDR	=

LIBS	=
LINTLIBS=
INCLUDE	=

CAD 	= /cad
FLAGS	= -DBSD -DUNIX
CFLAGS	= -O $(INCLUDE)
LDFLAGS	=
LINTFLAGS = $(INCLUDE)

.c.o:
	$(CC) -c $(CFLAGS) $(FLAGS) $*.c
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
	rm -f $(TARGET) *.a *.ln *.o \
	    [Tt]ags [Mm]ake.out lint malloc.out gmon.out __.SYMDEF

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

