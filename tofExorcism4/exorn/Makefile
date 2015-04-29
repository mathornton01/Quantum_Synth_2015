# Makefile for Exorn modification to 
# Exorcism4 based on method :
# ESOP-based Toffoli Gate Cascade Generation
#  by Fazel, Thornton
#
# Updated: Josh Rendon, 4/17/2015
#
# Modified from original makefile by: 
#
# Alan Mishchenko <alanmi@ee.pdx.edu>
# Makefile for Exorcism-4.7, March 15, 2002

BASEDIR = ..

# modify these to point to your lib and include directory
INC_DIRS = ${BASEDIR}/cudd-2.3.1/include \
	${BASEDIR}/extra14 \
	${BASEDIR}/espresso-2.3.3/mincov \
	${BASEDIR}/espresso-2.3.3/sparse \
	${BASEDIR}/espresso-2.3.3/main

LIB_DIRS = ${BASEDIR}/lib \
	${BASEDIR}/cudd-2.3.1/cudd \
	${BASEDIR}/cudd-2.3.1/st \
	${BASEDIR}/cudd-2.3.1/dddmp \
	${BASEDIR}/cudd-2.3.1/mtr \
	${BASEDIR}/cudd-2.3.1/util \
	${BASEDIR}/cudd-2.3.1/epd

CC      = gcc
CFLAGS  = -g -c 
LFLAGS  = -g -lm

INCLUDE = -I$(INC_DIR)
CFILES  = exor.c exorBits.c exorCover.c exorCubes.c\
	exorLink.c exorList.c exorReduce.c exorUtil.c

LIBS    = extra cudd dddmp mtr st util epd

NAME    = e

.SILENT:

all: PROJ

OBJS = $(CFILES:.c=.o)

.SUFFIXES: .o .c

.c.o:
	echo Compiling file $< ...
	echo $(CC) $(CFLAGS) ${foreach inc, ${INC_DIRS}, -I${inc}} $<
	$(CC) $(CFLAGS) ${foreach inc, ${INC_DIRS}, -I${inc}} $<

PROJ: $(OBJS)
	echo "Linking files"
	echo $(CC) -o $(NAME) $(LFLAGS) $(OBJS) ${foreach libdir, ${LIB_DIRS}, -L${libdir}} ${foreach lib, $(LIBS), -l${lib}} -lm
	$(CC) -o $(NAME) $(LFLAGS) $(OBJS) ${foreach libdir, ${LIB_DIRS}, -L${libdir}} ${foreach lib, $(LIBS), -l${lib}} -lm

clean:
	rm *.o

