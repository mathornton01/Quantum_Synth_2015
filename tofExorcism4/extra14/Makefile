# Makefile for Extra14 additions to the cudd-2.3.1 library
# Updated: Josh Rendon, 4/17/2015
#
# Modified from original makefile by: 
# Makefile for EXTRA library of DD-based procedures - Mar 16, 2001, by alanmi

# comment out the following line to see what's happening
.SILENT:

BASEDIR = ..

# modify these macros to point to your include and lib directory
INCDIRS = ${BASEDIR}/cudd-2.3.1/include \
	${BASEDIR}/extra14 \
	${BASEDIR}/espresso-2.3.3/mincov \
	${BASEDIR}/espresso-2.3.3/sparse \
	${BASEDIR}/espresso-2.3.3/main \
	${BASEDIR}/espresso-2.3.3

LIBDIR = ${BASEDIR}/lib/

# modify this line to compile with compilers other than g++
CC       = gcc

# Flags :
#      -O    = optimize
#      -Wall = display all warnings
#      -c    = compile or assemble source but don't link 
#      -g    = produce debugging information
#      -a    = generate extra code to write profile information;
#              this records the number of times each block is executed

#CFLAGS = -O3 -pedantic -Wall -ansi
#CFLAGS = -c -g -Wall -D_UNIX

# these are compiler flags for HP
#CFLAGS  = -c -O -D_UNIX -D_HP_

# these are compiler flags for SunOS
CFLAGS  = -c -O -D_UNIX

#LFLAGS = -pg -Wall
LFLAGS  =

NAME = extra

CFILES = aCoverStats.c aMisc.c aSpectra.c bCache.c bDecomp.c \
	bEncoding.c bMisc.c bImage2.c bNet.c bNetRead.c \
	bNodePaths.c bSupp.c bSymm.c bTransfer.c bVisUtils.c \
	bzShift.c bSPFD.c hmEssen.c hmExpand.c hmIrred.c \
	hmLast.c hmRandom.c hmReduce.c hmRondo.c hmUtil.c \
        zCC.c zCovers.c zGraphFunc.c zGraphIO.c zLitCount.c \
        zMaxMin.c zMisc.c zPerm.c zSubSup.c zUCP1.c zUCP2.c \
        zEspresso.c zISOP.c zExor.c zFactor.c zCoverPrint.c 
                                                
HEADER = extra.h

OBJS	= $(CFILES:.c=.o)

.SUFFIXES: .o .c

.c.o:
	echo Compiling file $< ...
	echo $(foreach inc, $(INCDIRS), -I$(inc))
	$(CC) $(CFLAGS) $(foreach inc,$(INCDIRS), -I$(inc)) $<

all: lib$(NAME).a

lib$(NAME).a: $(OBJS)
	echo Creating library lib$(NAME).a ...
	ar -r lib$(NAME).a $(OBJS)
	ranlib lib$(NAME).a
	cp lib$(NAME).a $(LIBDIR)/lib$(NAME).a
	#cp $(HEADER)    $(INCDIR)/$(HEADER)

clean:
	rm *.o                


