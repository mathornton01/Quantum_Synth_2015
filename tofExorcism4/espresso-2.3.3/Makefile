#
# Espresso -- two-level boolean minimization program ( + package)
#
#  #define options:
#	main.c and cvrm.c accept:
#	    RANDOM -- compiles calls to random() and srandom(); this is
#		to support the -erandom command line option for test purposes
#
#	cofactor.c, expand.c, and set.h accept:
#	    NO_INLINE -- to disable in-line expansion of routines
#
#   IBMPC sets the integer size (in bits) to be 16 and sets NO_INLINE;
#   util should also be compiled with -DIBMPC for safe memory allocation.
#---------------------------------------------------------------------------

#CAD	= /cad/cad/octtools
CAD	= /usr/local
HOST	= eros
UNPACK	= unpack

REQUIRE	= mm

# directories which make up the espresso archive
DIRS	= main sparse mincov util

TARGET	= espresso
TARGETG = espresso-g
TARGETPG= espresso-pg

TARLIB	= libMV.a
TARLIBG = libMV-g.a
TARLIBPG= libMV-pg.a

TARLINT	= llib-lMV.ln

#LIBS	= $(CAD)/lib/libmm.a
#LIBSG	= $(CAD)/lib/libmm.a
#LIBSPG	=

CC	= gcc
CFLAGS	= -O
CFLAGSG = -g
CFLAGSPG= -pg -O

#---------------------------------------------------------------------------

$(TARGET): $(TARLIB) $(LIBS) do_link
$(TARGETG): cleansome $(TARLIBG) $(LIBSG) do_linkg
$(TARGETPG): cleansome $(TARLIBPG) $(LIBSPG) do_linkpg

$(TARLIB): do_make do_repack
$(TARLIBG): do_makeg do_repack
	mv $(TARLIB) $(TARLIBG)
$(TARLIBPG): do_makepg do_repack
	mv $(TARLIB) $(TARLIBPG)

do_link:
	$(CC) -g -o $(TARGET) $(TARLIB) $(LIBS) -lm
do_linkg:
	$(CC) -g -o $(TARGETG) $(TARLIBG) $(LIBSG) -lm
do_linkpg:
	$(CC) -pg -o $(TARGETPG) $(TARLIBPG) $(LIBSPG) -lm

do_make:
	@for dir in $(DIRS); do				\
	    (cd $$dir; 					\
	     echo Making $$dir ...; 			\
	     make CC=$(CC) 'CFLAGS=$(CFLAGS) $$(INCLUDE)' CAD=$(CAD) lib$$dir.a)\
	done

do_makeg:
	@for dir in $(DIRS); do 			\
	    (cd $$dir; 					\
	     echo Making $$dir ...; 			\
	     make CC=$(CC) 'CFLAGS=$(CFLAGSG) $$(INCLUDE)' CAD=$(CAD) lib$$dir.a)\
	done

do_makepg:
	@for dir in $(DIRS); do 			\
	    (cd $$dir; 					\
	     echo Making $$dir ...; 			\
	     make CC=$(CC) 'CFLAGS=$(CFLAGSPG) $$(INCLUDE)' CAD=$(CAD) lib$$dir.a)\
	done

do_repack:
	@rm -rf $(UNPACK) $(TARLIB)
	@mkdir $(UNPACK)
	@for i in $(DIRS); do 				\
	    cwd=`pwd`;					\
	    (cd $(UNPACK); 				\
	     ar x $$cwd/$$i/lib$$i.a);			\
	done
	@(cd $(UNPACK);					\
	    ar crl temp.a *.o;				\
	    ranlib temp.a)			
	cp $(UNPACK)/temp.a $(TARLIB)
	ranlib -t $(TARLIB)
	@rm -rf $(UNPACK)

$(TARLINT): _force
	@rm -f _temp;
	@for i in $(DIRS); do 				\
	    cwd=`pwd`;					\
	    (cd $$i; 					\
	       echo Linting $$i ...; 			\
	       make CAD=$(CAD) llib-l$$i.ln);		\
	    cat $$cwd/$$i/llib-l$$i.ln >>_temp;		\
	done
	@mv _temp $(TARLINT)

lint: $(TARLINT) do_lint

do_lint:
	@for dir in $(DIRS) $(PROGS); do		\
	    (cd $$dir; 					\
	     echo; 					\
	     echo "lint $$dir ..."; 			\
	     make CAD=$(CAD) lint)			\
	done


clean: cleansome
	rm -f $(TARLIBPG) $(TARGETPG) $(TARLIBG) $(TARGETG)

cleansome:
	rm -f $(TARLIB) $(TARLINT) $(TARGET) tags malloc.out gmon.out
	@for dir in $(DIRS); do				\
	    (cd $$dir; 					\
	     make -i CAD=$(CAD) clean >/dev/null)	\
	done

depend:
	@for dir in $(DIRS); do 			\
	    (cd $$dir; 					\
	     make -i CAD=$(CAD) depend)			\
	done

wc:
	wc */*.[cly] | tail -1
	grep ';' */*.[cly] | wc | tail -1

tags: _force
	rm -f tags
	touch tags
	for dir in $(DIRS); do				\
	    cwd=`pwd`;					\
	    (cd $$dir; 					\
	     rm -f tags; 				\
	     ln -s $$cwd/tags tags)			\
	done
	for dir in $(DIRS); do				\
	    (cd $$dir; 					\
	     echo $$dir;				\
	     make CAD=$(CAD) tags)			\
	done
	sort -o tags tags


# Installation is a little long ...
install: $(CAD)/bin/espresso \
	 $(CAD)/man/man1/espresso.1 \
	 $(CAD)/man/man5/espresso.5 \
	 $(CAD)/lib/libMV.a \
	 $(CAD)/lib/llib-lMV.ln \
	 $(CAD)/include/espresso.h \
	 copy_examples \
	 copy_libs

install_ic:
	rsh $(HOST) -n -l cad "rdist -cw \
	    /cad/bin/espresso \
	    /cad/lib/espresso \
	    /cad/lib/libMV.a /cad/lib/libMV-g.a /cad/lib/libMV-pg.a \
	    /cad/lib/llib-lMV.ln \
	    /cad/include/espresso.h \
	    /cad/man/man1/espresso.1 /cad/man/man5/espresso.5 ic:"

$(CAD)/bin/espresso: espresso
	cp espresso $(CAD)/bin
	strip $(CAD)/bin/espresso
$(CAD)/lib/libMV.a: libMV.a
	cp libMV.a $(CAD)/lib
	ranlib $(CAD)/lib/libMV.a
$(CAD)/lib/llib-lMV.ln: llib-lMV.ln
	cp llib-lMV.ln $(CAD)/lib
$(CAD)/man/man1/espresso.1: espresso.1
	cp espresso.1 $(CAD)/man/man1/espresso.1
$(CAD)/man/man5/espresso.5: espresso.5
	cp espresso.5 $(CAD)/man/man5/espresso.5
$(CAD)/include/espresso.h: main/espresso.h
	cp main/espresso.h $(CAD)/include
copy_examples:
	-mkdir $(CAD)/lib/espresso
	-cp -r ex $(CAD)/lib/espresso
copy_libs:
	-cp libMV-g.a $(CAD)/lib
	-ranlib $(CAD)/lib/libMV-g.a
	-cp libMV-pg.a $(CAD)/lib
	-ranlib $(CAD)/lib/libMV-pg.a

compile: 
	make $(TARGETG)
	make $(TARGETPG)
	make cleansome $(TARGET)
	make $(TARLINT)
	make lint

espresso.doc1: espresso.1
	nroff -man espresso.1 | col >espresso.doc1
espresso.doc5: espresso.5
	nroff -man espresso.5 | col >espresso.doc5

test: espresso
	cd examples; make >../test 2>&1

require:
	@echo $(REQUIRE)

_force:
