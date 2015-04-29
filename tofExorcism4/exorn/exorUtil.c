////////////////////////////////////////////////////////////////////////
///                                                                  ///
///                  Implementation of EXORCISM - 4                  ///
///              An Exclusive Sum-of-Product Minimizer               ///
///               Alan Mishchenko  <alanmi@ee.pdx.edu>               ///
///                                                                  ///
////////////////////////////////////////////////////////////////////////
///                                                                  ///
///                        Utility Functions                         ///
///                                                                  ///
///       1) allocating memory for and creating the ESOP cover       ///
///       2) writing the resultant cover into an ESOP PLA file       ///
///                                                                  ///
///  Ver. 1.0. Started - July 15, 2000. Last update - July 20, 2000  ///
///  Ver. 1.4. Started -  Aug 10, 2000. Last update -  Aug 10, 2000  ///
///  Ver. 1.5. Started -  Aug 19, 2000. Last update -  Aug 19, 2000  ///
///  Ver. 1.7. Started -  Sep 20, 2000. Last update -  Sep 23, 2000  ///
///                                                                  ///
//   Bugfixes:                                                       ///
//     Josh Rendon: 4/29/15                                          ///
//     Fixed bug where memset was corrupting heap                    ///
////////////////////////////////////////////////////////////////////////
///   This software was tested with the BDD package "CUDD", v.2.3.0  ///
///                          by Fabio Somenzi                        ///
///                  http://vlsi.colorado.edu/~fabio/                ///
////////////////////////////////////////////////////////////////////////

#include "exor.h"
#include "extra.h"

////////////////////////////////////////////////////////////////////////
///                      EXTERNAL VARIABLES                         ////
////////////////////////////////////////////////////////////////////////

// information about the options, the function, and the cover

extern float alphaC ;
extern float betaC ;
extern int hasNots ;
extern int costFunc ;
extern int removeNots ;

extern BFunc g_Func;
extern cinfo g_CoverInfo;

//statics
static long reorderTicks = 0 ;
static long notRemovalTicks = 0 ;
static long numGates = 0 ;
static long numNots = 0 ;

static tmpNumNots = 0 ;

///////////////////////////////////////////////////////////////////////
///                        EXTERNAL FUNCTIONS                        ///
////////////////////////////////////////////////////////////////////////

// Cube Cover Iterator
// starts an iterator that traverses all the cubes in the ring
extern Cube* IterCubeSetStart();
// returns the next cube in the ring
extern Cube* IterCubeSetNext();

// retrieves the variable from the cube
extern varvalue GetVar( Cube* pC, int Var );

// converts Value into a string
extern char* itoa( int Value, char* Buff, int Radix );

////////////////////////////////////////////////////////////////////////
///                      FUNCTION DECLARATIONS                       ///
////////////////////////////////////////////////////////////////////////

// debugging function
//void PrintCoverDebug( ostream& DebugStream );
//void PrintCube( ostream& DebugStream, Cube* pC );
//void PrintCoverProfile( ostream& DebugStream );

///////////////////////////////////////////////////////////////////
////////////       Cover Service Procedures       /////////////////
///////////////////////////////////////////////////////////////////

int CountLiterals()
// nCubesAlloc is the number of allocated cubes 
{
	Cube* p;
	int Value, v;
	int LitCounter = 0;
	int LitCounterControl = 0;

	for ( p = IterCubeSetStart( ); p; p = IterCubeSetNext() )
	{
		LitCounterControl += p->a;

		assert( p->fMark == 0 );

		// write the input variables
		for ( v = 0; v < g_Func.nInputs; v++ )
		{
			Value = GetVar( p, v );
			if ( Value == VAR_NEG )
				LitCounter++;
			else if ( Value == VAR_POS )
				LitCounter++;
			else if ( Value != VAR_ABS )
			{
				assert(0);
			}
		}
	}

	if ( LitCounterControl != LitCounter )
		printf( "Warning! The recorded number of literals (%d) differs from the actual number (%d)\n", LitCounterControl, LitCounter );
	return LitCounter;
}

/*global cube list set defined in exorList.c*/
extern Cube *s_List ;
extern Cube *s_pCubeLast ;

void calcVarCost(Cube *startCube, Cube *endCube, char *ignoreVar, int *bestVar) {
  Cube *cube = startCube ;
  int *alpha = (int*)malloc(sizeof(int)*g_Func.nInputs) ;
  int *beta = (int*)malloc(sizeof(int)*g_Func.nInputs) ;

  memset(alpha, 0, sizeof(int)*g_Func.nInputs) ;
  memset(beta, 0, sizeof(int)*g_Func.nInputs) ;

  int i ;

  //do this so I can pass in null to mean the last cube in the list
  Cube *_end = 0 ; 
  if(endCube==0) {
    _end = 0 ;
  }
  else {
    _end = endCube->Next ;
  }

  for(cube=startCube ; cube != _end ; cube=cube->Next) {
    for(i=0 ; i<g_Func.nInputs ; i++) {
      if(!ignoreVar[i]) {
	int Value = GetVar( cube, i );
	if ( Value == VAR_NEG ) {
	  alpha[i] +=  1 ;
	  beta[i]  += -1 ;
	}
	else if ( Value == VAR_POS ) {
	  alpha[i] +=  1 ;
	  beta[i]  +=  1 ;
	}
      }
    }
  }

  float cost = -1e10 ;
  *bestVar = -1 ;

  for(i=0 ; i<g_Func.nInputs ; i++) {
    if(!ignoreVar[i]) {
      float tscore ; 
      switch(costFunc) {
      case 1:
	tscore = alphaC*(alpha[i]!=0 ? 1.0/alpha[i] : 0) + betaC*beta[i] ;
	break ;
      default:
	tscore = alpha[i] + beta[i] ;
	break ;
      }
      if(tscore > cost) {
	cost = tscore;
	*bestVar = i ;
      }
    }
  }

  free(alpha) ;
  free(beta) ;
}

void splitCubes(Cube *start, Cube *end, int bestVar, Cube **start1, Cube **start2, Cube **end1, Cube **end2) {
  Cube *_end ;
  if(end==0) {
    _end = 0 ;
  }
  else {
    _end = end->Next ;
  }

  Cube *cube ;
  Cube *cube1 = 0 ;
  Cube *cube2 = 0 ;
  Cube *nextCube ;

  if(cube != 0) {
    nextCube = start->Next ;
  }
  else {
    nextCube = 0 ;
  }

  *start1 = 0 ;
  *start2 = 0 ;


  for(cube = start ; cube != _end ; cube=nextCube) {
    nextCube = cube->Next ;
    int Value = GetVar( cube, bestVar );
    if ( Value == VAR_NEG ) {
      if(*start2==0) {
	*start2 = cube ;
	cube2 = cube ;
	cube2->Prev = 0 ;
	cube2->Next = 0 ;
      }
      else {
	cube2->Next = cube ;
	cube->Prev = cube2 ;
	cube->Next = 0 ;
	cube2 = cube ;
      }
    }
    else if ( Value == VAR_POS || Value == VAR_ABS ) {
      if(*start1==0) {
	*start1 = cube ;
	cube1 = cube ;
	cube1->Prev = 0 ;
	cube1->Next = 0 ;
      }
      else {
	cube1->Next = cube ;
	cube->Prev = cube1 ;
	cube->Next = 0 ;
	cube1 = cube ;
      }
    }
    else
      assert(0);
  }

  *end1 = cube1 ;
  *end2 = cube2 ;
}

Cube* connectLists(Cube *start1, Cube *start2, Cube *end1, Cube *end2, Cube **newStart, Cube **newEnd) {
  if(start1!=0) {
    *newStart = start1 ;
    start1->Prev = 0 ;
    if(start2 != 0) {
      end1->Next = start2 ;
      start2->Prev = end1 ;
      end2->Next = 0 ;
      *newEnd = end2 ;
    }
    else {
      end1->Next = 0 ;
      *newEnd = end1 ;
    }
  }
  else if(start2!=0) {
    *newStart = start2 ;
    start2->Prev = end1 ;
    end2->Next = 0 ;
    *newEnd = end2 ;
  }
}

void printCube(Cube *cube, FILE *file) {
  int v ;
  for ( v = 0; v < g_Func.nInputs; v++ ) {
    int Value = GetVar( cube, v );
    if ( Value == VAR_NEG ) {
      fprintf( file, "0" );
    }
    else if ( Value == VAR_POS ) {
      fprintf( file, "1" );
    }
    else if ( Value == VAR_ABS ) {
      fprintf( file, "-" );
    }
    else {
      assert(0);
    }
  }
  fprintf(file, "\n") ;
}

void printCubes(Cube *start, Cube *end, FILE *file) {
  Cube *_end = 0 ;
  if(end==0) {
    _end = 0 ;
  }
  else {
    _end = end->Next ;
  }

  Cube *cube ;
  for(cube=start ; cube != _end ; cube=cube->Next) {
    printCube(cube, file) ;
  }

}

void _reorderCubes(Cube *start, Cube *end, Cube **newStart, Cube **newEnd, char *ignoreVar, int *level, int invert) {
  if(start==0) {
    *newStart = start ;
    *newEnd = end ;
    return ;
  }

  if(*level==0) {
    *newStart = start ;
    *newEnd = end ;
    return ;
  }

  //calculate best var
  int bestVar = -1 ;
  calcVarCost(start, end, ignoreVar, &bestVar) ;
  //  fprintf(stderr, "best var: %d , level: %d , invert: %d\n", bestVar, *level, invert) ;
  //split the cube list into two lists   
  Cube *start1 = 0 ;
  Cube *start2 = 0 ;
  Cube *end1 = 0 ;
  Cube *end2 = 0 ;
  Cube *newStart1 = 0 ;
  Cube *newStart2 = 0 ;
  Cube *newEnd1 = 0 ;
  Cube *newEnd2 = 0 ;
  //    fprintf(stderr, "original list\n") ;
  //    printCubes(start, end, stderr) ;
  splitCubes(start, end, bestVar, &start1, &start2, &end1, &end2) ;
  //    fprintf(stderr, "split list 1\n") ;
  //    printCubes(start1, end1, stderr) ;
  //    fprintf(stderr, "split list 2\n") ;
  //    printCubes(start2, end2, stderr) ;

  //set ignoreVar[bestVar] to true
  ignoreVar[bestVar] = 1 ;
  (*level)-- ;
  //order each sublist
  if(invert!=2) {
    _reorderCubes(start1, end1, &newStart1, &newEnd1, ignoreVar, level, 0) ;
    _reorderCubes(start2, end2, &newStart2, &newEnd2, ignoreVar, level, 1) ;

    //add not gates if invert is 1
    if(newStart2 != NULL) {
      tmpNumNots+=2 ;

      Cube *not1 = (Cube*)malloc(sizeof(Cube)) ;
      Cube *not2 = (Cube*)malloc(sizeof(Cube)) ;
      
      memset(not1, 0, sizeof(Cube)) ;
      memset(not2, 0, sizeof(Cube)) ;
      not1->fMark = 1 ;
      not2->fMark = 1 ;
      
      not1->pCubeDataIn = (word*)malloc(sizeof(int)) ;
      not2->pCubeDataIn = (word*)malloc(sizeof(int)) ;
      
      *((int*)(not1->pCubeDataIn)) = bestVar + 1;
      *((int*)(not2->pCubeDataIn)) = bestVar + 1;
      
      not1->Next = newStart2 ;
      newStart2->Prev = not1 ;
      not2->Prev = newEnd2 ;
      newEnd2->Next = not2 ;
      
      newStart2 = not1 ;
      newEnd2 = not2 ;
    }

  }
  else {
    _reorderCubes(start1, end1, &newStart1, &newEnd1, ignoreVar, level, 2) ;
    _reorderCubes(start2, end2, &newStart2, &newEnd2, ignoreVar, level, 2) ;
  }

  //unset ignoreVar[bestVar] to false
  ignoreVar[bestVar] = 0 ;
  (*level)++ ;

  //reconnect the two lists
  connectLists(newStart1, newStart2, newEnd1, newEnd2, newStart, newEnd) ;
}

void reorderCubes() {
  char *ignoreVar = (char*)malloc(sizeof(char)*g_Func.nInputs) ;
  memset(ignoreVar, 0, sizeof(char)*g_Func.nInputs) ;
  int bestVar ;
  float cost ;
  Cube *start = IterCubeSetStart( ) ;
  Cube *end = 0 ;
  Cube *newStart = 0 ;
  Cube *newEnd = 0  ;
  int level = g_Func.nInputs ;
  int invert = 0 ;

  fprintf(stderr, "reordering cubes\n") ;

  if(!hasNots) {
    invert = 2; 
  }

  long clk = clock() ;

  _reorderCubes(start, end, &newStart, &newEnd, ignoreVar, &level, invert) ;

  reorderTicks = clock() - clk ;
  
  //remove superfluous nots, note that this causes the "garbage" outputs to either be
  //  positive or negative literal...not just a bunch of positive literals

  notRemovalTicks = 0 ;

  fprintf(stderr, "tmpNumNots: %d\n", tmpNumNots) ;

  if(hasNots && removeNots) {

    clk = clock() ;

    typedef struct sNotChain {
      Cube *cube ;
      int isNot ; //else its a representaive for dependent gates
      struct sNotChain *prev ;
      struct sNotChain *next ;
    } NotChain ;

    //remove unneeded nots
    Cube *cube ;

    NotChain **notChain = (NotChain**)malloc(sizeof(NotChain*)*g_Func.nInputs) ;
    //Josh Rendon: 4/29/15
    //Fixed bug where memset was corrupting heap
    //on incorrectly specified sizeof(NotChain) instead of
    //sizeof(NotChain*)
    memset(notChain, 0, sizeof(NotChain*)*g_Func.nInputs) ;

    for(cube = newStart ; cube!=NULL ;cube=cube->Next) {

      if(cube->fMark==1) { //if cube is a "not" cube
	int line = *((int*)cube->pCubeDataIn)-1 ;
	if(notChain[line]==0) {
	  fprintf(stderr, "start notChain %d\n", line) ;
	  //NotChain *nc = (NotChain*)malloc(sizeof(NotChain)) ;
	  NotChain *nc = malloc(sizeof(NotChain) ) ;
          assert(nc != 0);
	  nc->next = 0 ;
	  nc->prev = 0 ;
	  nc->isNot = 1 ;
	  nc->cube = cube ;
	  notChain[line] = nc ;
	}
	else {
	  //if last in chain is a dependent representative, then cannot got rid of that last or current not gate
	  if(notChain[line]->isNot) {
	    //	    fprintf(stderr, "line %d: remove nots\n", line) ;
	    //can get rid of this and last not gate
	    //remove prev from the not chain
	    NotChain *prevPrev = notChain[line]->prev ;
	    NotChain *prev = notChain[line] ;
	    notChain[line] = prevPrev ;
	    //remove the not cubes from the cube list
	    
	    Cube *pcube ;
	    pcube = prev->cube ;
	    pcube->Prev->Next = pcube->Next ;
	    pcube->Next->Prev = pcube->Prev ;

	    free(pcube) ;
	    tmpNumNots-- ;

	    pcube = cube ;
	    pcube->Prev->Next = pcube->Next ;
	    pcube->Next->Prev = pcube->Prev ;

	    //set cube to be cube->Prev ;
	    cube = cube->Prev ;
	    
	    free(pcube) ;
	    tmpNumNots-- ;
	  }
	  else {
	    //add new not gate
	    //	    fprintf(stderr, "line %d: keep nots\n", line) ;
	    NotChain *nc = (NotChain*)malloc(sizeof(NotChain)) ;
	    nc->next = 0 ;
	    nc->prev = notChain[line] ;
	    nc->isNot = 1 ;
	    nc->cube = cube ;
	    notChain[line]->next = nc ;
	    notChain[line] = nc ;
	  }
	}
      }
      else {
	//check all control lines
	//mark notFound[line] = 2 if notFound[line] == 1 
	int v ;
	for ( v = 0; v < g_Func.nInputs; v++ ) {
	  int Value = GetVar( cube, v );
	  if ( Value == VAR_NEG || Value == VAR_POS) {
	    if(notChain[v]!=0) {
	      if(notChain[v]->isNot) {
		//		fprintf(stderr, "line %d: set dependent\n", v) ;
		//add new dependent representative
		NotChain *nc = (NotChain*)malloc(sizeof(NotChain)) ;
		nc->next = 0 ;
		nc->prev = notChain[v] ;
		nc->isNot = 0 ;
		nc->cube = cube ;
		notChain[v]->next = nc ;
		notChain[v] = nc ;
	      }
	    }
	  }
	}	
      }
    }

    //remove the nots that are at the end and are not needed
    int v ; 
    for ( v = 0; v < g_Func.nInputs; v++ ) {
      if(notChain[v] != 0) {
	if(notChain[v]->isNot) {
	  //remove the not
	  //	  fprintf(stderr, "line %d: remove end not\n", v) ;
	  //can get rid of end not gate
	  Cube *cube = notChain[v]->cube ;
	  cube->Prev->Next = cube->Next ;
	  if(cube->Next) {
	    cube->Next->Prev = cube->Prev ;
	  }

	  free(cube) ;
	  tmpNumNots-- ;
	}
      }
    }

    notRemovalTicks = clock() - clk ;

  }
  

  s_List = newStart ;
  s_pCubeLast = NULL ;

  free(ignoreVar) ;
}

void printToffoliGate(FILE *tfile, Cube *cube, int *pcontrol, int *target) {
  int v ;
  int w ; 
  int cOutputs;
  int nOutput;
  int WordSize;

  if(cube->fMark) {
    numNots++ ;
    fprintf(tfile, "T1 x%d\n", *((int*)cube->pCubeDataIn)-1) ;
  }
  else {
    // get the input vars
    int numPControl = 0 ;

    //zero out pcontrol
    for ( v = 0; v < g_Func.nInputs*2; v++ ) {
      pcontrol[v] = 0 ;
    }
    //zero out target
    for ( w = 0; w < g_Func.nOutputs; w++ ) {
      target[w] = 0 ;
    }

    for ( v = 0; v < g_Func.nInputs; v++ ) {
      int Value = GetVar( cube, v );
      
      if(hasNots) {
	//means to use the positive literal
	if ( Value == VAR_POS || Value == VAR_NEG) {
	  //	fprintf( stdout, "1" );
	  pcontrol[v] = 1 ;
	  pcontrol[g_Func.nInputs+v] = 0 ;
	  numPControl++ ;
	}
	else if ( Value == VAR_ABS ) {
	  //	fprintf( stdout, "-" );
	  pcontrol[v] = 0 ;
	  pcontrol[g_Func.nInputs+v] = 0 ;
	}
	else {
	  assert(0);
	}
      }
      else {
	if ( Value == VAR_NEG ) {
	  //	fprintf( stdout, "0" );
	  pcontrol[v] = 0 ;
	  pcontrol[g_Func.nInputs+v] = 1 ;
	  numPControl++ ;
	}
	else if ( Value == VAR_POS ) {
	  //	fprintf( stdout, "1" );
	  pcontrol[v] = 1 ;
	  pcontrol[g_Func.nInputs+v] = 0 ;
	  numPControl++ ;
	}
	else if ( Value == VAR_ABS ) {
	  //	fprintf( stdout, "-" );
	  pcontrol[v] = 0 ;
	  pcontrol[g_Func.nInputs+v] = 0 ;
	}
	else {
	  assert(0);
	}
      }
    }
    //fprintf( stdout, " " );
    
    // write the output variables
    cOutputs = 0;
    nOutput = g_Func.nOutputs;
    WordSize = 8*sizeof( unsigned );
    for ( w = 0; w < g_CoverInfo.nWordsOut; w++ ) {
      for ( v = 0; v < WordSize; v++ ) {
	if ( cube->pCubeDataOut[w] & (1<<v) ) {
	  //	  fprintf( stdout, "1" );
	  target[v] = 1 ;
	}
	else {
	  //	  fprintf( stdout, "0" );
	  target[v] = 0 ;
	}
	if ( ++cOutputs == nOutput ) {
	  break;
	}
      }
    }
    //    fprintf( stdout, "\n" );

    for(w=0 ; w<g_Func.nOutputs ; w++) {
      if(target[w]==1) {
	numGates++ ;
	fprintf(tfile, "T%d ", numPControl+1) ;
	if(hasNots) {
	  for(v=0 ; v<g_Func.nInputs; v++) {
	    if(pcontrol[v]==1) {
	      fprintf(tfile, "x%d,", v) ;
	    }
	  }
	}
	else {
	  for(v=0 ; v<g_Func.nInputs*2; v++) {
	    if(pcontrol[v]==1) {
	      fprintf(tfile, "x%d,", v) ;
	    }
	  }
	}
	fprintf(tfile, "f%d\n", w) ;
      }
    }
  }
}

void WriteToffoli()
// nCubesAlloc is the number of allocated cubes 
{
  int v, w;
  Cube * p;

  
  int *pcontrol = (int*)malloc(sizeof(int)*g_Func.nInputs*2) ;
  int *target = (int*)malloc(sizeof(int)*g_Func.nOutputs) ;
  int numPControl = 0 ;
  
  FILE *tfile ;
  char tfilename[1024] ;

  reorderCubes() ;

  fprintf(stderr, "tmpNumNots: %d\n", tmpNumNots) ;

  fprintf(stderr, "writing toffoli list\n") ;
  
  sprintf(tfilename, "%s.tfc", g_Func.FileInput) ;
  
  fprintf(stderr, "file: %s\n", tfilename) ;

  fprintf(stderr, "output file: %s\n", tfilename) ;

  tfile = fopen(tfilename, "w") ;

  if(!tfile) {
    fprintf(stderr, "can't open %s...quitting\n", tfilename) ;
    return ; 
  }

  //output statistics
  fprintf(tfile, "#NOTE:  two sets of stats, one at the top of the file, the other at the end of file\n") ;
  fprintf(tfile, "#first set are basic stats and esop gen time\n") ;
  fprintf(tfile, "#second set are the toffoli cascade generation stats\n") ;
  fprintf(tfile, "#\n") ;
  fprintf(tfile, "#StatSetOne format: #. name in out cubes\n") ;
  fprintf(tfile, "#StatTwoOne format: #. gates nots esopTime reorderTime notRemovalTime totalToffoliTime totalTime\n") ;
  fprintf(tfile, "#\n") ;
  fprintf(tfile, "#StatSetOne\n") ;
  fprintf(tfile, "#. %s %d %d %d\n",
	  g_Func.FileInput,
	  g_CoverInfo.nVarsIn,
	  g_CoverInfo.nVarsOut,
	  g_CoverInfo.nCubesInUse) ;


  long clk1 = clock();

  //output .v line
  fprintf(tfile, ".v ") ;
  if(hasNots) {
    for ( v = 0; v < g_Func.nInputs; v++ ) {
      fprintf(tfile, "x%d,", v) ;
    }
  }
  else {
    for ( v = 0; v < g_Func.nInputs*2; v++ ) {
      fprintf(tfile, "x%d,", v) ;
    }
  }
  for ( w = 0; w < g_Func.nOutputs; w++ ) {
    fprintf(tfile, "f%d", w) ;
    if(w != g_Func.nOutputs-1) {
      fprintf(tfile, ",") ;
    }
  }
  //output .i line
  fprintf(tfile, "\n.i ") ;
  if(hasNots) {
    for ( v = 0; v < g_Func.nInputs; v++ ) {
      fprintf(tfile, "x%d", v) ;
      if(v != g_Func.nInputs-1) {
	fprintf(tfile, ",") ;
      }
    }
  }
  else {
    for ( v = 0; v < g_Func.nInputs*2; v++ ) {
      fprintf(tfile, "x%d", v) ;
      if(v != g_Func.nInputs*2-1) {
	fprintf(tfile, ",") ;
      }
    }
  }
  //output .o line
  fprintf(tfile, "\n.o ") ;
  for ( w = 0; w < g_Func.nOutputs; w++ ) {
    fprintf(tfile, "f%d", w) ;
    if(w != g_Func.nOutputs-1) {
      fprintf(tfile, ",") ;
    }
  }
  //output .c line
  fprintf(tfile, "\n.c ") ;
  for ( w = 0; w < g_Func.nOutputs; w++ ) {
    //fprintf(tfile, "0", w) ;
    fprintf(tfile, "0%d", w) ;
    if(w != g_Func.nOutputs-1) {
      fprintf(tfile, ",") ;
    }
  }
  //output BEGIN
  fprintf(tfile, "\nBEGIN\n") ;
  
  for ( p = IterCubeSetStart( ); p; p = IterCubeSetNext() ) {
    //assert( p->fMark == 0 );  //fMark == 0 means its a toffoli not gate
    printToffoliGate(tfile, p, pcontrol, target) ;
    if(p->fMark) {
      //remove the "not" cube
      Cube *prev = p->Prev ;
      Cube *next = p->Next ;
      free((int*)(p->pCubeDataIn)) ;
      tmpNumNots-- ;
      if(prev) {
	prev->Next = next ;
	if(next) {
	  next->Prev = prev ;
	}
      }
      else if(next) {
	next->Prev = 0 ;
      }
      free(p) ;
      if(prev) {
	p = prev ;
      }
      else if(next) {
	p = next ;
      }
      else {
	s_List = NULL ;
	s_pCubeLast = NULL ;
      }
    }
  }

  fprintf(tfile, "END\n") ;

  fprintf(stderr, "tmpNumNots: %d , numNots: %ld\n", tmpNumNots, numNots) ;

  long ticks = clock() - clk1;  //note that this includes file write time

  float esopTime = TICKS_TO_SECONDS(g_CoverInfo.TimeRead) + TICKS_TO_SECONDS(g_CoverInfo.TimeStart) + TICKS_TO_SECONDS(g_CoverInfo.TimeMin);
  float reorderTime = TICKS_TO_SECONDS(reorderTicks) ;
  float notRemovalTime = TICKS_TO_SECONDS(notRemovalTicks) ;
  float toffoliTime = reorderTime+notRemovalTime ;
  float totalTime = esopTime + toffoliTime ;
  fprintf(tfile, "#\n") ;
  //  fprintf(tfile, "#StatTwoOne format: #. gates nots esopTime reorderTime notRemovalTime totalToffoliTime totalTime\n") ;
  fprintf(tfile, "#StatSetTwo\n") ;
  fprintf(tfile, "#. %ld %ld %.2f %.2f %.2f %.2f %.2f\n",
	  numGates, 
	  numNots, 
	  esopTime,
	  reorderTime,
	  notRemovalTime, 
	  toffoliTime,
	  totalTime) ;
	  
  //  fclose(tfile) ;

  free(pcontrol) ;
  free(target) ;
}

void WriteTableIntoFile( FILE * pFile )
// nCubesAlloc is the number of allocated cubes 
{
	int v, w;
	Cube * p;
	int cOutputs;
	int nOutput;
	int WordSize;

	WriteToffoli() ;

	fprintf(stderr, "WriteTableIntoFile\n") ;

	for ( p = IterCubeSetStart( ); p; p = IterCubeSetNext() )
	{
		assert( p->fMark == 0 );

		// write the input variables
		for ( v = 0; v < g_Func.nInputs; v++ )
		{
			int Value = GetVar( p, v );
			if ( Value == VAR_NEG )
				fprintf( pFile, "0" );
			else if ( Value == VAR_POS )
				fprintf( pFile, "1" );
			else if ( Value == VAR_ABS )
				fprintf( pFile, "-" );
			else
				assert(0);
		}
		fprintf( pFile, " " );

		// write the output variables
		cOutputs = 0;
		nOutput = g_Func.nOutputs;
		WordSize = 8*sizeof( unsigned );
		for ( w = 0; w < g_CoverInfo.nWordsOut; w++ )
			for ( v = 0; v < WordSize; v++ )
			{
				if ( p->pCubeDataOut[w] & (1<<v) )
					fprintf( pFile, "1" );
				else
					fprintf( pFile, "0" );
				if ( ++cOutputs == nOutput )
					break;
			}
		fprintf( pFile, "\n" );
	}

	


}

  
int WriteResultIntoFile()
// write the ESOP cover into the PLA file <NewFileName>
{

	WriteToffoli() ;

  /*
	FILE * pFile;
	time_t ltime;
	char * TimeStr;

	fprintf(stderr, "WriteResultIntoFile\n") ;

	

	pFile = fopen( g_Func.FileOutput, "w" );
	if ( pFile == NULL )
	{
	  fprintf( stderr, "\n\nCannot open the output file: %s\n", g_Func.FileOutput );
		return 1;
	}

	// get current time
	time( &ltime );
	TimeStr = asctime( localtime( &ltime ) );
	// get the number of literals
	g_CoverInfo.nLiteralsAfter = CountLiterals();
	fprintf( pFile, "# EXORCISM-4 output for command line arguments: " );
	fprintf( pFile, "\"-q%d -v%d %s\"\n", g_CoverInfo.Quality, g_CoverInfo.Verbosity, g_Func.FileInput );
	fprintf( pFile, "# Minimization performed %s", TimeStr );
	fprintf( pFile, "# Initial statistics: " );
	fprintf( pFile, "Cubes = %d  Literals = %d\n", g_CoverInfo.nCubesBefore, g_CoverInfo.nLiteralsBefore );
	fprintf( pFile, "# Final   statistics: " );
	fprintf( pFile, "Cubes = %d  Literals = %d\n", g_CoverInfo.nCubesInUse, g_CoverInfo.nLiteralsAfter );
	fprintf( pFile, "# File reading and reordering time = %.2f sec\n", TICKS_TO_SECONDS(g_CoverInfo.TimeRead) );
	fprintf( pFile, "# Starting cover generation time   = %.2f sec\n", TICKS_TO_SECONDS(g_CoverInfo.TimeStart) );
	fprintf( pFile, "# Pure ESOP minimization time      = %.2f sec\n", TICKS_TO_SECONDS(g_CoverInfo.TimeMin) );
	fprintf( pFile, ".i %d\n", g_CoverInfo.nVarsIn );
	fprintf( pFile, ".o %d\n", g_CoverInfo.nVarsOut );
	fprintf( pFile, ".p %d\n", g_CoverInfo.nCubesInUse );
	fprintf( pFile, ".type esop\n" );
	WriteTableIntoFile( pFile );
	fprintf( pFile, ".e\n" );
	fclose( pFile );
  */
	return 0;
}

///////////////////////////////////////////////////////////////////
////////////          Debugging Procedures        /////////////////
///////////////////////////////////////////////////////////////////
/*
void PrintCubeVars( ostream& DebugStream )
{
	int Width = 3;
	DebugStream << setw(Width) << " " << " |";

	register int v, w;
	for ( v = 0; v < g_CoverInfo.nVarsIn; v++ )
		DebugStream << setw(Width) << v;
	DebugStream << endl;

	for ( w = 0; w < Width; w++ )
		DebugStream << "-";

	DebugStream << "-|";
	for ( v = -1; v < g_CoverInfo.nVarsIn; v++ )
		for ( w = 0; w < Width; w++ )
			DebugStream << "-";
	DebugStream << endl;
}

void PrintCube( ostream& DebugStream, Cube* pC )
{	
//	assert( pC->fType == CUBE_BUSY );

	int Width = 3;
	DebugStream << setw(Width) << (int)pC->ID << " |  ";
	register int v, w;
	for ( v = 0; v < g_CoverInfo.nVarsIn; v++ )
	{
//		int Value = ( (pC->pCubeDataIn[(2*v)>>LOGBPI] >> ((2*v) & BPIMASK) ) & 3);
		int Value = GetVar( pC, v );
		if ( Value == VAR_NEG )
//			DebugStream << setw(Width) << "01";
			DebugStream << "0";
		else if ( Value == VAR_POS )
//			DebugStream << setw(Width) << "10";
			DebugStream << "1";
		else if ( Value == VAR_ABS )
//			DebugStream << setw(Width) << "11";
			DebugStream << "-";
		else
			assert(0);
	}
//	DebugStream << endl;

	DebugStream << " ";
	// write the output variables
	int cOutputs = 0;
	int nOutput = g_CoverInfo.nVarsOut;
	int WordSize = 8*sizeof( unsigned );
	for ( w = 0; w < g_CoverInfo.nWordsOut; w++ )
		for ( v = 0; v < WordSize; v++ )
		{
			if ( pC->pCubeDataOut[w] & (1<<v) )
				DebugStream << "1";
			else
				DebugStream << "0";
			if ( ++cOutputs == nOutput )
				break;
		}

	DebugStream << "   ";
	DebugStream << "   a = " << pC->a;
	DebugStream << "   z = " << pC->z;

	DebugStream << endl;
}

void PrintCoverDebug( ostream& DebugStream )
{
	int cCubes = 0;
	for ( Cube* q = IterCubeSetStart( ); q; q = IterCubeSetNext() )
	{
		PrintCube( cout, q );
		cCubes++;
	}
	cout << "The number of cubes = " << cCubes << endl;
}
*/

///////////////////////////////////////////////////////////////////
////////////              End of File             /////////////////
///////////////////////////////////////////////////////////////////

