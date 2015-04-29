# sparse -- sparse matrix package
#---------------------------------------------------------------------------

CAD 	= /cad

P	= sparse
PSRC	= rows.c cols.c matrix.c
POBJ	= rows.o cols.o matrix.o
PHDR	= sparse.h

TARGET	= a.out

LIBS	= 

LINTLIBS=

INCLUDE	= -I../util
CFLAGS	= -O $(INCLUDE)
LDFLAGS	=
LINTFLAGS = $(INCLUDE)

#----------------------------------------------------------------------
foo: install

.c.o:
	$(CC) $(CFLAGS) -c -DFAST_AND_LOOSE $*.c

cols.o: cols.c
	$(CC) $(CFLAGS) -c -DFAST_AND_LOOSE -DCOLS cols.c
cols.c: rows.c
	swap_row_col <rows.c >cols.c

sparse.dvi: sparse.tex
	-latex sparse

#----------------------------------------------------------------------

install: lib$(P).a llib-l$(P).ln

lib$(P).a: $(POBJ)
	@rm -f lib$(P).a
	ar cr lib$(P).a $(POBJ)
	ranlib lib$(P).a

llib-l$(P).ln: $(PSRC) $(PHDR)
	lint $(INCLUDE) -C$(P) -n $(PSRC)

$(TARGET): $(POBJ) $(LIBS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(POBJ) $(LIBS) -lm

lint: $(PSRC) $(PHDR) $(HDR)
	lint $(LINTFLAGS) $(PSRC) $(LINTLIBS)

clean:
	rm -f $(TARGET) *.a *.ln *.o \
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

