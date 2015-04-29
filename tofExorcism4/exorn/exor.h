////////////////////////////////////////////////////////////////////////
///                                                                  ///
///                    Interface of EXORCISM - 4                     ///
///              An Exclusive Sum-of-Product Minimizer               ///
///               Alan Mishchenko  <alanmi@ee.pdx.edu>               ///
///                                                                  ///
////////////////////////////////////////////////////////////////////////
///                                                                  ///
///                         Main Module                              ///
///                                                                  ///
///  Ver. 1.0. Started - July 15, 2000. Last update - July 20, 2000  ///
///  Ver. 1.4. Started - Aug 10, 2000.  Last update - Aug 10, 2000   ///
///  Ver. 1.7. Started - Sep 20, 2000.  Last update - Sep 23, 2000   ///
///                                                                  ///
////////////////////////////////////////////////////////////////////////
///   This software was tested with the BDD package "CUDD", v.2.3.0  ///
///                          by Fabio Somenzi                        ///
///                  http://vlsi.colorado.edu/~fabio/                ///
////////////////////////////////////////////////////////////////////////

#ifndef __EXORMAIN_H__
#define __EXORMAIN_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

// the number of bits per integer (can be 16, 32, 64 - tested for 32)
#define BPI                 32 
#define BPIMASK             31 
#define LOGBPI               5

// the maximum number of input variables
#define MAXVARS           1000

// the number of cubes that are allocated additionally
#define ADDITIONAL_CUBES    33

// the factor showing how many cube pairs will be allocated
#define CUBE_PAIR_FACTOR    20
// the following number of cube pairs are allocated:
// nCubesAlloc/CUBE_PAIR_FACTOR

#if BPI == 64
#define DIFFERENT   0x5555555555555555
#define BIT_COUNT(w)   (BitCount[(w)&0xffff] + BitCount[((w)>>16)&0xffff] + BitCount[((w)>>32)&0xffff] + BitCount[(w)>>48])
#elif BPI == 32
#define DIFFERENT   0x55555555
#define BIT_COUNT(w)   (BitCount[(w)&0xffff] + BitCount[(w)>>16])
#else
#define DIFFERENT   0x5555
#define BIT_COUNT(w)   (BitCount[(w)])
#endif


#define VarWord(element)  ((element)>>LOGBPI)
#define VarBit(element)   ((element)&BPIMASK)

#define TICKS_TO_SECONDS(time) ((float)(time)/(float)(CLOCKS_PER_SEC))

////////////////////////////////////////////////////////////////////////
///                  CUBE COVER and CUBE typedefs                    ///
////////////////////////////////////////////////////////////////////////

typedef enum { MULTI_OUTPUT = 1, SINGLE_NODE, MULTI_NODE  } type;

// infomation about the cover
typedef struct cinfo_tag 
{
	int nVarsIn;        // number of input binary variables in the cubes
	int nVarsOut;       // number of output binary variables in the cubes
	int nWordsIn;       // number of input words used to represent the cover
	int nWordsOut;      // number of output words used to represent the cover
	int nCubesAlloc;    // the number of allocated cubes 
	int nCubesBefore;   // number of cubes before simplification
	int nCubesInUse;    // number of cubes after simplification
	int nCubesFree;     // number of free cubes
	int nLiteralsBefore;// number of literals before
	int nLiteralsAfter; // number of literals before
	int cIDs;           // the counter of cube IDs

	int Verbosity;      // verbosity level
	int Quality;        // quality

	int TimeRead;       // reading time
	int TimeStart;      // starting cover computation time
	int TimeMin;        // pure minimization time
} cinfo;

// representation of one cube (24 bytes + bit info)
typedef unsigned int  word;
typedef unsigned char byte;
typedef struct cube 
{
  byte  fMark;        // the flag which is TRUE if the cubes is enabled
  byte  ID;           // (almost) unique ID of the cube
  short a;            // the number of literals
  short z;            // the number of 1's in the output part
  word* pCubeDataIn;  // a pointer to the bit string representing literals
  word* pCubeDataOut; // a pointer to the bit string representing literals
  struct cube* Prev;  // pointers to the previous/next cubes in the list/ring 
  struct cube* Next;
} Cube;


////////////////////////////////////////////////////////////////////////
///              VARVALUE and CUBEDIST enum typedefs                 ///
////////////////////////////////////////////////////////////////////////

// literal values
typedef enum { VAR_NEG = 1, VAR_POS, VAR_ABS  } varvalue;

// the flag in some function calls can take one of the follwing values
typedef enum { DIST2, DIST3, DIST4 } cubedist;

#endif
