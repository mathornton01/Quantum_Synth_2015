/**CFile***********************************************************************

  FileName    [zCC.c]

  PackageName [extra]

  Synopsis    [Fast implicit heuristic CC solver.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_zddSolveCC();
				</ul>
			Internal procedures included in this module:
				<ul>
				</ul>
			static procedures included in this module:
				<ul>
				<li> zddopCCEnumeratePaths();
				<li> zddopCCRowRemapFn();
				<li> zddopCCColRemapFn();
				<li> zddopCCSolSelectFn();
				<li> extraZddSubSetLabeled();
				<li> extraZddSupSetLabeled();
				<li> extraZddColumnMax();
				</ul>
	      ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  ReviSon    [$zCC.c, v.1.2, December 3,17, 2000, alanmi $]

******************************************************************************/

#include "util.h"
#include "time.h"
#include "cuddInt.h"
#include "st.h"
#include "extra.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                     */
/*---------------------------------------------------------------------------*/

typedef struct
{
	int Col;    // the column number
	float Cost; // the cost of this column
} colinfo;

colinfo ** s_Queque;   // the array of colinfo structures forming a priority queque
colinfo *  s_Memory;   // the pointer to the memory used

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

static DdManager * s_ddold;      /* the original manager */
static DdManager * s_ddnew;      /* the temporary DD manager */

static int         s_nVars;      /* the number of vars in CC */
static int         s_nRowLabels; /* the number of vars to encode rows */
static int         s_nColLabels; /* the number of vars to encode columns */
static int         s_nLabels;    /* the number of total labeling vars */
static int         s_nVarsNew;   /* the number of total vars in the new manager (s_nLabels+s_nVars) */

static int         s_nUpperLabels; /* the number of upper labels (columns or rows) */
static int         s_nLowerLabels; /* the number of lower labels (columns or rows) */

static DdNode    * s_zUpperLabeledSet; /* accumulator of labeled rows */
static DdNode    * s_zLowerLabeledSet; /* accumulator of labeled columns */
static DdNode    * s_zSolution;    /* accumulator of original columns in the solution */

static DdNode    * s_zRow2ColMap;  /* externalized pointer used by static functions */

static int         s_nPaths;       /* the counter of paths enumerated so far */
static int         s_nColNums;     /* the counter of columns in the solution */

static int       * s_pVarValues;   /* array to store variable values on the path */
static int       * s_pColNums;     /* array to store numbers of columns in the solution */

static st_table  * s_RowHash;      /* the local cache for rows */ 
static st_table  * s_ColHash;      /* the local cache for columns */ 

static float       s_ColumnCost;   /* accumulator of the column cost */
static float       s_RowCost;      /* the cost of row to subtract from column costs */

static int         s_cTableEntries;  /* used for debugging */
static int         s_cTableEntriesInit;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/* local cache for zddCCCountPaths()          */
/* TODO: add dynamic allocation               */
#define TABLESIZE 11111
static struct Entry_tag
{
	DdNode * S;
	int      Res;
} HTable[TABLESIZE];


/**AutomaTcStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* static Function prototypes                                                */
/*---------------------------------------------------------------------------*/

/* path enumerating functions */
/* traversing the origina ZDD of columns and rows */
static int zddopCCEnumeratePaths ARGS((DdManager *dd, DdNode *S, int levPrev, int(*Fn)(int*,int) ));
/* traversing the map of columns into rows and vise-versa */
static int zddopCCEnumerateMapPaths ARGS((DdNode *S, int levPrev, int CutLevel, unsigned Path, int(*Fn)(unsigned,DdNode*) ));
/* traversing the set of rows/columns extracted from the map */
static int zddopCCEnumerateSetPaths ARGS((DdNode *S, int levPrev, unsigned Path, int(*Fn)(unsigned) ));

/* two remapping functions */
static int zddopCCRemapToUpper ARGS((int* VarValues, int Row));
static int zddopCCRemapToLower ARGS((int* VarValues, int Row));

/* solution selection function */
static int zddopCCSolSelectFn ARGS((int* VarValues, int Row));

/* labeled sub(p)setting functions */
static DdNode * extraZddSubSetLabeled ARGS((DdManager *dd, DdNode *X, DdNode *Y));
static DdNode * extraZddSupSetLabeled ARGS((DdManager *dd, DdNode *X, DdNode *Y));
static DdNode * extraZddSupSetLabeled2 ARGS((DdManager *dd, DdNode *X, DdNode *Y));

/* functions to set up the hash-table of rows/columns */
static int zddopCCRowCollectFn ARGS((unsigned Path, DdNode * S ));
static int zddopCCColCollectFn ARGS((unsigned Path, DdNode * S ));

/* function called internally during row hash-table setting to compute the cost of a column */
static int zddopCCRowAddCostFn ARGS((unsigned Path));

/* functions to subtract the cost of columns removed from the table */
static int zddopCCRowSubtractFn ARGS((unsigned Path));
static int zddopCCColSubtractFn ARGS((unsigned Path));

static int zddCCCountPaths ARGS((DdNode *S ));

static int zddCCCompIntFn ARGS((const void* a, const void* b ));
static int zddCCCompQuequeFn ARGS((const void* a, const void* b ));


/**AutomaTcEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of exported Functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Solves the cyclic core heuristically.]

  Description [Solves the cyclic core specified by the pair of ZDDs 
  (zRows, zCols) using a fast greedy heuristic method. The flag denotes 
  the CC type: 1 means that "columns cover rows" (CC of this kind arises 
  in the set covering problem, in which each element is encoded using 
  a separate ZDD variable). 0 means "rows cover columns" (two-level SOP 
  minimization with products and primes encoded using ZDDs yields this 
  kind of CC). Returns the set of columns representing the solution.]

  SideEffects [Defines a temporary DD manager to store labeled ZDDs.]

  SeeAlso     [Cudd_zddSolveUCP1]

******************************************************************************/
DdNode *
Extra_zddSolveCC( 
  DdManager * dd,     /* manager which owns the cyclic core */
  DdNode * zRows,     /* the set of CC rows */
  DdNode * zCols,     /* the set of CC columns */
  int fColsCoverRows, /* parameters defining the type of CC */
  int fVerbose) 
{
	int nRows, nCols, Temp;
	int i, lev, TempSolCounter;
	DdNode *zFirstMap, *zSecondMap, *zCol2RowMap, *zRow2ColMap, *zCube, *zTemp;
	long clk1, clk2 = clock();

	/* set the static variables */
	s_ddold = dd;
	s_nVars = dd->sizeZ;  /* the number of elements in the covering problem */
	nRows = Cudd_zddCount( dd, zRows ); /* the number of rows in the CC */
	nCols = Cudd_zddCount( dd, zCols ); /* the number of columns in the CC */
	for ( s_nRowLabels = 0, Temp = nRows; Temp; Temp>>=1, s_nRowLabels++ );
	for ( s_nColLabels = 0, Temp = nCols; Temp; Temp>>=1, s_nColLabels++ );
	assert( s_nRowLabels < 32 );
	assert( s_nColLabels < 32 );
	s_nLabels  = s_nRowLabels + s_nColLabels;
	s_nVarsNew = s_nVars + s_nRowLabels + s_nColLabels;

	if ( s_nVars < 2 || nRows < 2 || nCols < 2 )
		return NULL;

	/* start a new manager */
	s_ddnew = Cudd_Init( 0, s_nVarsNew, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 16777216);
	if ( s_ddnew == NULL )
		return NULL;
	/* disable dynamic reordering */
	Cudd_AutodynDisableZdd( s_ddnew );

	/* start the array for var values */
	s_pVarValues = ALLOC( int, s_nVarsNew );
	if ( s_pVarValues == NULL )
		return NULL;

	/* find the labeled mapping of the set of columns and rows */
	clk1 = clock();
	if ( fColsCoverRows )
	{ /* standart set covering problem requires SubSet(rows,cols) operation
	     therefore row labels go on top */
		s_nUpperLabels = s_nRowLabels;
		s_nLowerLabels = s_nColLabels;

		/* get the labeled set of ROWS */
		/* set the column labels to zeros - they are the same during the traversal */
		for ( lev = 0; lev < s_nLowerLabels; lev++ )
			s_pVarValues[ s_nVarsNew - s_nLowerLabels + lev ] = 0;
		s_zUpperLabeledSet = s_ddnew->zero;
		Cudd_Ref( s_zUpperLabeledSet );
		s_nPaths = 0;
		zddopCCEnumeratePaths( s_ddold, zRows, -1, zddopCCRemapToUpper );
		assert( s_nPaths == nRows );

		/* get the labeled set of COLUMNS */
		/* set the row labels to zeros - they are the same during the traversal */
		for ( lev = 0; lev < s_nUpperLabels; lev++ )
			s_pVarValues[ lev ] = 0;
		s_zLowerLabeledSet = s_ddnew->zero;
		Cudd_Ref( s_zLowerLabeledSet );
		s_nPaths = 0;
		zddopCCEnumeratePaths( s_ddold, zCols, -1, zddopCCRemapToLower );
		assert( s_nPaths == nCols );

		/* perform sub(p)setting of the labeled sets */
		zFirstMap = extraZddSubSetLabeled( s_ddnew, s_zUpperLabeledSet, s_zLowerLabeledSet );
	}
	else
	{ /* two-level minimization UCP requires SupSet(rows,cols) operation
	     therefore column labels go on top */
		s_nUpperLabels = s_nColLabels;
		s_nLowerLabels = s_nRowLabels;

		/* get the labeled set of ROWS */
		/* set the column labels to zeros - they are the same during the traversal */
		for ( lev = 0; lev < s_nUpperLabels; lev++ )
			s_pVarValues[ lev ] = 0;
		s_zLowerLabeledSet = s_ddnew->zero;
		Cudd_Ref( s_zLowerLabeledSet );
		s_nPaths = 0;
		zddopCCEnumeratePaths( s_ddold, zRows, -1, zddopCCRemapToLower );
		assert( s_nPaths == nRows );

		/* get the labeled set of COLUMNS */
		/* set the row labels to zeros - they are the same during the traversal */
		for ( lev = 0; lev < s_nLowerLabels; lev++ )
			s_pVarValues[ s_nVarsNew - s_nLowerLabels + lev ] = 0;
		s_zUpperLabeledSet = s_ddnew->zero;
		Cudd_Ref( s_zUpperLabeledSet );
		s_nPaths = 0;
		zddopCCEnumeratePaths( s_ddold, zCols, -1, zddopCCRemapToUpper );
		assert( s_nPaths == nCols );

		/* perform sub(p)setting of the labeled sets */
//		zFirstMap = extraZddSupSetLabeled( s_ddnew, s_zLowerLabeledSet, s_zUpperLabeledSet );
		zFirstMap = extraZddSupSetLabeled2( s_ddnew, s_zLowerLabeledSet, s_zUpperLabeledSet );
	}
	Cudd_Ref( zFirstMap );
	Cudd_RecursiveDerefZdd( s_ddnew, s_zUpperLabeledSet );
	Cudd_RecursiveDerefZdd( s_ddnew, s_zLowerLabeledSet );

	if ( fVerbose )
	{
//		Cudd_zddPrintMinterm( s_ddnew, zFirstMap );
		printf( "\nThe number of nodes in the complete labeled map = %d", Cudd_DagSize(zFirstMap) );
//		printf( "\nThe number of paths in the complete labeled map = %d", zddCCCountPaths(zFirstMap) );
		printf( "\nThe complete labeled map computation time = %.3f sec\n", (float)(clock() - clk1)/(float)(CLOCKS_PER_SEC) );
	}


	/* find the mapping of labels */

	// create the cube of ordinary variables 
	// set the upper and lower lables to zero, other to one 
	clk1 = clock();
	for ( lev = 0; lev < s_nUpperLabels; lev++ )
		s_pVarValues[ lev ] = 0;
	for (        ; lev < s_nVarsNew - s_nLowerLabels; lev++ )
		s_pVarValues[ lev ] = 1;
	for (        ; lev < s_nVarsNew; lev++ )
		s_pVarValues[ lev ] = 0;
	// create the cube 
	zCube = Extra_zddCombination( s_ddnew, s_pVarValues, s_nVarsNew );
	Cudd_Ref( zCube );

	// remove the extra variables (leaving only labels) 
	zFirstMap = Extra_zddExistAbstract( s_ddnew, zTemp = zFirstMap, zCube );
	Cudd_Ref( zFirstMap );
	Cudd_RecursiveDerefZdd( s_ddnew, zTemp );
	Cudd_RecursiveDerefZdd( s_ddnew, zCube );

	if ( fVerbose )
	{
//		Cudd_zddPrintMinterm( s_ddnew, zFirstMap );
		printf( "\nThe number of nodes in the abstracted labeled map = %d", Cudd_DagSize(zFirstMap) );
//		printf( "\nThe number of paths in the abstracted labeled map = %d", zddCCCountPaths(zFirstMap) );
		printf( "\nThe abstracted labeled map computation time = %.3f sec\n", (float)(clock() - clk1)/(float)(CLOCKS_PER_SEC) );
	}

	/* move the lower labeling variables up */
	clk1 = clock();
	for ( lev = 0; lev < s_nUpperLabels; lev++ )
		s_pVarValues[ lev ] = lev;
	for (        ; lev < s_nVarsNew - s_nLowerLabels; lev++ )
		s_pVarValues[ lev ] = lev + s_nLowerLabels;
	for (        ; lev < s_nVarsNew; lev++ )
		s_pVarValues[ lev ] = lev - (s_nVarsNew - s_nLowerLabels) + s_nUpperLabels;

	zFirstMap = Extra_zddPermute( s_ddnew, zTemp = zFirstMap, s_pVarValues );
	Cudd_Ref( zFirstMap );
	Cudd_RecursiveDerefZdd( s_ddnew, zTemp );

	if ( fVerbose )
	{
//		Cudd_zddPrintMinterm( s_ddnew, zFirstMap );
		printf( "\nThe number of nodes in the permuted labeled map = %d", Cudd_DagSize(zFirstMap) );
//		printf( "\nThe number of paths in the permuted labeled map = %d", zddCCCountPaths(zFirstMap) );
		printf( "\nThe permuted labeled map computation time = %.3f sec\n", (float)(clock() - clk1)/(float)(CLOCKS_PER_SEC) );
	}


	/* build the inverse map */

	/* change the spaces of lower and upper labeling variables */
	clk1 = clock();
	for ( lev = 0; lev < s_nUpperLabels; lev++ )
		s_pVarValues[ lev ] = s_nLowerLabels + lev;
	for (        ; lev < s_nUpperLabels + s_nLowerLabels; lev++ )
		s_pVarValues[ lev ] = lev - s_nUpperLabels;
	for (        ; lev < s_nVarsNew; lev++ )
		s_pVarValues[ lev ] = lev;

	zSecondMap = Extra_zddPermute( s_ddnew, zFirstMap, s_pVarValues );
	Cudd_Ref( zSecondMap );

	if ( fVerbose )
	{
	//	Cudd_zddPrintMinterm( s_ddnew, zFirstMap );
		printf( "\nThe number of nodes in the inverted labeled map = %d", Cudd_DagSize(zSecondMap) );
	//	printf( "\nThe number of paths in the inverted labeled map = %d", zddCCCountPaths(zSecondMap) );
		printf( "\nThe permuted inverted map computation time = %.3f sec\n", (float)(clock() - clk1)/(float)(CLOCKS_PER_SEC) );
	}


	/* assign the map depending on the type of the cyclic core */
	if ( fColsCoverRows )
	{
		zRow2ColMap = zFirstMap;
		zCol2RowMap = zSecondMap;
	}
	else
	{
		zCol2RowMap = zFirstMap;
		zRow2ColMap = zSecondMap;
	}
	s_zRow2ColMap = zRow2ColMap;


	/* allocate memory for the priority queque */
	/* the array of pointers to colinfo structures forming a priority queque */
	s_Queque = (colinfo**) malloc( nCols * sizeof( colinfo* ) );   
	/* the array of colinfo structures itself */
	s_Memory = (colinfo*) malloc( nCols * sizeof( colinfo ) );   
	/* assign the pointers */
	for ( i = 0; i < nCols; i++ )
		s_Queque[i] = s_Memory + i;


	clk1 = clock();
	/* create two hash tables:
	   for Rows: storing the cost of each row by its number
	   for Cols: storing the pointer to the priority que entry for this column
	*/
	s_RowHash = st_init_table_with_params(st_numcmp,st_numhash,nRows,4,2,0);
	s_ColHash = st_init_table_with_params(st_numcmp,st_numhash,nCols,4,2,0);

	/* enumerate the rows and count their cost */
	s_nPaths = 0;
	zddopCCEnumerateMapPaths( zRow2ColMap, -1, s_nRowLabels, 0, zddopCCRowCollectFn );
	assert( s_nPaths == nRows );

	/* enumerate the columns and count their cost */
	s_nPaths = 0;
	zddopCCEnumerateMapPaths( zCol2RowMap, -1, s_nColLabels, 0, zddopCCColCollectFn );
	assert( s_nPaths == nCols );
	if ( fVerbose )
	{
//		printf( "\nThe number of entries in RowHash = %d", st_count( s_RowHash ) );
//		printf( "\nThe number of entries in ColHash = %d\n", st_count( s_ColHash ) );
	}

	/* sort the array of columns */
	qsort( s_Queque, nCols, sizeof(colinfo*), zddCCCompQuequeFn );


	/* start the column numbers in the solution */
	s_pColNums = ALLOC( int, nCols+1 );
	if ( s_pColNums == NULL )
		return NULL;
	s_nColNums = 0;
	s_cTableEntries = 0;
	clk1 = clock();
	while ( s_Queque[ s_nColNums ]->Cost > 0.001 )
	{
		DdNode * zRowsAssoc = zCol2RowMap;

		/* add the first column (with the largest cost) to the solution */
		int nColumn = s_Queque[ s_nColNums ]->Col;
		s_pColNums[ s_nColNums++ ] = nColumn;
		assert( s_nColNums <= nCols );

		/* get the set of rows assiciated with this column */
		while ( zRowsAssoc->index < s_nColLabels )
			if ( nColumn & (1<<(s_nColLabels-1-zRowsAssoc->index)) )
				zRowsAssoc = cuddT( zRowsAssoc );
			else
				zRowsAssoc = cuddE( zRowsAssoc );

		/* go through all the associated rows; for each row, find the covering columns
		   and subtract the cost of this row from the costs of these columns */
		s_nPaths = 0;
		zddopCCEnumerateSetPaths( zRowsAssoc, s_nColLabels-1, 0, zddopCCRowSubtractFn );
		assert( s_nPaths > 0 );

		/* sort the array of columns (that part which does not belong to the solution */
		qsort( s_Queque + s_nColNums, nCols-s_nColNums, sizeof(colinfo*), zddCCCompQuequeFn );
	}
	Cudd_RecursiveDerefZdd( s_ddnew, zRow2ColMap );
	Cudd_RecursiveDerefZdd( s_ddnew, zCol2RowMap );

	s_cTableEntriesInit = zddCCCountPaths(zRow2ColMap);
	assert( s_cTableEntriesInit == s_cTableEntries );

	if ( fVerbose )
	{
		printf( "\nHeuristic CC solution size = %d", s_nColNums );
		printf( "\nIterative column selection time = %.3f sec\n", (float)(clock() - clk1)/(float)(CLOCKS_PER_SEC) );
	}

	/* remember the number of columns in the solution */
	TempSolCounter = s_nColNums;
	assert( s_nColNums <= nCols );
	/* set the delimiter */
	s_pColNums[ s_nColNums++ ] = 1000000; 

	/* sort the array of solutions */
	qsort( s_pColNums, TempSolCounter, sizeof(int), zddCCCompIntFn );


	/* build the solutions in terms of original columns 
	 * to do this, traverse the original column ZDD and compute
	 * the union of columns corresponding to given numbers */
	clk1 = clock();
	s_zSolution = s_ddold->zero;
	Cudd_Ref( s_zSolution );
	s_nPaths = 0;
	s_nColNums = 0;
	zddopCCEnumeratePaths( s_ddold, zCols, -1, zddopCCSolSelectFn );
	/* the number of enumerated paths is the same as the number of all columns */
	assert( s_nPaths == nCols );
	/* the number of paths in the solution is the same as the number original solution columns */
	assert( s_nColNums = TempSolCounter );
	/* now, the solution is ready in s_zSolution */

	if ( fVerbose )
	{
	//	Cudd_zddPrintMinterm( s_ddold, s_zSolution );
		printf( "\nThe number of nodes in the solution = %d", Cudd_DagSize(s_zSolution) );
	//	printf( "\nThe number of paths in the solution = %d", zddCCCountPaths(s_zSolution) );
		printf( "\nThe solution computation time = %.3f sec\n", (float)(clock() - clk1)/(float)(CLOCKS_PER_SEC) );
		printf("\nTotal heuristic CC solution time = %.3f sec\n", (float)(clock() - clk2)/(float)(CLOCKS_PER_SEC) );
	}


	/* delocate storage */
	FREE( s_pVarValues );
	FREE( s_pColNums );
	st_free_table( s_RowHash );
	st_free_table( s_ColHash );

	// free the queque
	free( s_Memory );
	free( s_Queque );

	/* check that there are no referenced DDs in the package */
	if ( fVerbose )
	printf( "\nThe number of referenced nodes in the temporary manager = %d\n", Cudd_CheckZeroRef( s_ddnew ) );

	/* shut down the package */
	Cudd_Quit( s_ddnew );

	cuddDeref( s_zSolution );
	return s_zSolution;

} /* end of Extra_zddSolveCC */


/*---------------------------------------------------------------------------*/
/* Definition of internal Functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static Functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Enumeration of paths in a ZDD.]
  
  Description [Recursive procedure to enumerate all paths(cubes) in ZDD S; 
  levPrev is the level from which the function is called (initially -1). 
  During the traversal the function writes the variables on the path into 
  global static array s_pVarValues. Upon reaching the bottom level on a 
  1-path, this procedure calls function Fn by passing to it two parameters: 
  the variable assignment (s_pVarValues) and the number of the given path 
  (s_nPaths). To achieve better performance the function writes the
  variable values in s_pVarValues with a certain offset to be able to build
  a ZDD for this combination in function Fn without remapping var values. 
  Function Fn returns 1 to continue traversal, or 0 to abort. Reordering 
  should be disabled!]

  SideEffects [Increments the global counter of paths.]

  SeeAlso     []

******************************************************************************/
static int 
zddopCCEnumeratePaths( 
  DdManager *dd,       /* the manager whose ZDD is being enumerated */
  DdNode *S,           /* the ZDD to enumerate paths */ 
  int levPrev,         /* the level from which this function is called */ 
  int(*Fn)(int*,int) ) /* the function to call upon hitting the bottom */
{
	int lev;
	if ( S == dd->zero )
		return 1;
	if ( S == dd->one )
	{
		/* fill in the remaining variables */
		for ( lev = levPrev + 1; lev < dd->sizeZ; lev++ )
			s_pVarValues[ s_nUpperLabels + dd->invpermZ[ lev ] ] = 0;
		/* call the function */
		return ((*Fn)(s_pVarValues, s_nPaths++));
	}
	else
	{
		/* find the level of the top variable */
		int TopLevel = dd->permZ[ S->index ];

		/* fill in the remaining variables */
		for ( lev = levPrev + 1; lev < TopLevel; lev++ )
			s_pVarValues[ s_nUpperLabels + dd->invpermZ[ lev ] ] = 0;

		/* fill in this variable */
		/* the given var has negative polarity */
		s_pVarValues[ s_nUpperLabels + S->index ] = 0;
		// iterate through the else branch
		if ( zddopCCEnumeratePaths( dd, cuddE(S), TopLevel, Fn ) == 0 )
			return 0;
		
		// the given var has positive polarity
		s_pVarValues[ s_nUpperLabels + S->index ] = 1;
		// iterate through the then branch
		return ( zddopCCEnumeratePaths( dd, cuddT(S), TopLevel, Fn ) );
	}
} /* end of zddopCCEnumeratePaths */


/**Function********************************************************************

  Synopsis    [Function which adds a relabeled assignment to the new set.]

  Description [Uses the upper variable range to express the assignment.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
static int
zddopCCRemapToUpper( 
  int* VarValues,  /* row/column variable values */
  int  Number )    /* the number of this row/column (used as its label) */
{
	int lev;
	DdNode *NewComb, *Temp;

	/* set the row labels to the code of this row/column */
	for ( lev = 0; lev < s_nUpperLabels; lev++ )
		if ( Number & (1<<(s_nUpperLabels-1-lev)) )
			VarValues[ lev ] = 1;
		else
			VarValues[ lev ] = 0;
	/* column labels are already set to zeros */
	/* regular variables are already set to values they have on the path */

	/* create the combination */
	NewComb = Extra_zddCombination( s_ddnew, VarValues, s_nVarsNew );
	Cudd_Ref( NewComb );

	/* add this combination to the set */
	s_zUpperLabeledSet = Cudd_zddUnion( s_ddnew, Temp = s_zUpperLabeledSet, NewComb );
	Cudd_Ref( s_zUpperLabeledSet );
	Cudd_RecursiveDerefZdd( s_ddnew, Temp );
	Cudd_RecursiveDerefZdd( s_ddnew, NewComb );

	return 1;
} /* end of zddopCCRemapToUpper */



/**Function********************************************************************

  Synopsis    [Function which adds a relabeled assignment to the new set.]

  Description [Uses the lower variable range to express the assignment]

  SideEffects []

  SeeAlso     []

******************************************************************************/
static int
zddopCCRemapToLower( 
  int* VarValues,  /* row/column variable values */
  int  Number )    /* the number of this row/column (used as its label) */
{
	int lev;
	DdNode *NewComb, *Temp;

	/* set the row labels to the code of this row/column */
	for ( lev = 0; lev < s_nLowerLabels; lev++ )
		if ( Number & (1<<(s_nLowerLabels-1-lev)) )
			VarValues[ s_nVarsNew - s_nLowerLabels + lev ] = 1;
		else
			VarValues[ s_nVarsNew - s_nLowerLabels + lev ] = 0;
	/* column labels are already set to zeros */
	/* regular variables are already set to values they have on the path */

	/* create the combination */
	NewComb = Extra_zddCombination( s_ddnew, VarValues, s_nVarsNew );
	Cudd_Ref( NewComb );

	/* add this combination to the set */
	s_zLowerLabeledSet = Cudd_zddUnion( s_ddnew, Temp = s_zLowerLabeledSet, NewComb );
	Cudd_Ref( s_zLowerLabeledSet );
	Cudd_RecursiveDerefZdd( s_ddnew, Temp );
	Cudd_RecursiveDerefZdd( s_ddnew, NewComb );

	return 1;
} /* end of zddopCCRemapToLower */


/**Function********************************************************************

  Synopsis    [Function which adds columns to the solution.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
static int
zddopCCSolSelectFn( 
  int* VarValues,  /* row variable values */
  int  Col )       /* the number of this row (used as its label) */
{
	DdNode *NewComb, *Temp;

	/* skip columns that do not belong to the solution */
	if ( Col < s_pColNums[ s_nColNums ] )
		return 1;

	assert( Col == s_pColNums[ s_nColNums ] );

	/* create the combination - offset value values to skip the labels */
	NewComb = Extra_zddCombination( s_ddold, VarValues + s_nUpperLabels, s_nVars );
	Cudd_Ref( NewComb );

	/* add this combination to the set */
	s_zSolution = Cudd_zddUnion( s_ddold, Temp = s_zSolution, NewComb );
	Cudd_Ref( s_zSolution );
	Cudd_RecursiveDerefZdd( s_ddold, Temp );
	Cudd_RecursiveDerefZdd( s_ddold, NewComb );

	/* increment the counter of columns in the solution */
	s_nColNums++;

	return 1;
} /* end of zddopCCSolSelectFn */



/**Function********************************************************************

  Synopsis    [Enumerates paths in the labeled row/column map.]
  
  Description [This function is similar to zddopCCEnumeratePaths(). 
  Works with the temporary manager.]

  SideEffects [Calls functions which increment the global counter of paths.]

  SeeAlso     []

******************************************************************************/
static int 
zddopCCEnumerateMapPaths( 
  DdNode *S,           /* the ZDD to enumerate paths */ 
  int levPrev,         /* the level from which this function is called */ 
  int CutLevel,        /* the level below which enumeration stops */
  unsigned Path,       /* the unsigned integer representing the path of length <= 32 */
  int(*Fn)(unsigned,DdNode*) ) /* the function to call upon hitting the bottom */
{
	if ( S->index >= CutLevel )
	{
		if ( S == s_ddnew->zero )
			return 1;
		else /* call the function */			
			return ((*Fn)(Path, S));
	}
	else
	{
		/* find the level of the top variable */
		unsigned Mask = ( ~0 << (CutLevel - (levPrev + 1)) );

		/* fill in the remaining variables */
		Path &= Mask;
		/* recur through the else branch */
		if ( zddopCCEnumerateMapPaths( cuddE(S), S->index, CutLevel, Path, Fn ) == 0 )
			return 0;
		
		/* the given var has positive polarity */
		Path &= Mask;
		Path |= ( 1 << (CutLevel - 1 - S->index) );
		/* recur through the then branch */
		return ( zddopCCEnumerateMapPaths( cuddT(S), S->index, CutLevel, Path, Fn ) );
	}
} /* end of zddopCCEnumerateMapPaths */


/**Function********************************************************************

  Synopsis    [Collects rows into the hash table.]
  
  Description [Collects rows into the hash table. The table maps integer
  numbers of rows into the number of covering colunms. This number is used
  to compute the cost of a row. This number does not change during the CC 
  solution CC. Works with the temporary manager.]

  SideEffects [Calls functions which increment the global counter of paths.]

  SeeAlso     []

******************************************************************************/
static int
zddopCCRowCollectFn( 
  unsigned Path,  /* the row number */
  DdNode * S )    /* the associated columns */
{
	int AssocColumns = zddCCCountPaths(S );
	assert( AssocColumns > 1 );
	if (st_insert(s_RowHash, (char *)Path, (char *)AssocColumns) == ST_OUT_OF_MEM) 
	{
		assert(0);
	}
	s_nPaths++;
	return 1;
} /* end of zddopCCRowCollectFn */

/**Function********************************************************************

  Synopsis    [Collects columns into the hash table.]
  
  Description [Collects columns into the hash table. The table maps integer
  numbers of rows into the pointer to the entry in the priority queque
  storing this column. This pointer is used to access the column, in particular,
  to modify its cost. Works with the temporary manager.]

  SideEffects [Calls functions which increment the global counter of paths.]

  SeeAlso     []

******************************************************************************/
static int
zddopCCColCollectFn( 
  unsigned Path,  /* the column number */
  DdNode * S )    /* the associated rows */
{
	/* go through all the associated rows, 
	   for each row, add its cost to s_ColumnCost */
	s_ColumnCost = 0.0;
	zddopCCEnumerateSetPaths( S, s_nColLabels-1, 0, zddopCCRowAddCostFn );

	s_Queque[ s_nPaths ]->Col = Path;
	s_Queque[ s_nPaths ]->Cost = (float)s_ColumnCost;

	if (st_insert(s_ColHash, (char *)Path, (char *)s_Queque[ s_nPaths ]) == ST_OUT_OF_MEM) 
	{
		assert(0);
	}

	s_nPaths++;
	return 1;
} /* end of zddopCCColCollectFn */

/**Function********************************************************************

  Synopsis    [Add the cost of rows to the cost of the given column.]
  
  Description [Collects columns into the hash table. The table maps integer
  numbers of rows into the pointer to the entry in the priority queque
  storing this column. This pointer is used to access the column, in particular,
  to modify its cost. Works with the temporary manager.]

  SideEffects [Calls functions which increment the global counter of paths.]

  SeeAlso     []

******************************************************************************/
static int
zddopCCRowAddCostFn( 
  unsigned Path )  /* the row number */
{
	/* find the number of columns associated with this row */
	int Value;
	if (st_lookup_int(s_RowHash, (char *)Path, &Value) == 0) 
	{
		assert(0);
	}
	assert( Value > 1 );

	/* use this pointer to subtract the cost - assigned previously */
//  s_ColumnCost += (float)1.0/(Value-1);
	s_ColumnCost += (float)1.0;
	return 1;
} /* end of zddopCCRowAddCostFn */



/**Function********************************************************************

  Synopsis    [Enumerates paths in the set of rows/columns.]
  
  Description [This function is similar to zddopCCEnumeratePaths(). 
  Works with the temporary manager.]

  SideEffects [Calls functions which increment the global counter of paths.]

  SeeAlso     []

******************************************************************************/
static int 
zddopCCEnumerateSetPaths( 
  DdNode *S,           /* the ZDD to enumerate paths */ 
  int levPrev,         /* the level from which this function is called */ 
  unsigned Path,
  int(*Fn)(unsigned) ) /* the function to call upon hitting the bottom */
{
	if ( S == s_ddnew->zero )
		return 1;
	else if ( S == s_ddnew->one )
		/* call the function */
		return ((*Fn)(Path));
	else
	{
		/* find the level of the top variable */
		unsigned Mask = ( ~0 << (s_nLabels - (levPrev + 1)) );

		/* fill in the remaining variables */
		Path &= Mask;
		/* recur through the else branch */
		if ( zddopCCEnumerateSetPaths( cuddE(S), S->index, Path, Fn ) == 0 )
			return 0;
		
		/* the given var has positive polarity */
		Path &= Mask;
		Path |= ( 1 << (s_nLabels - 1 - S->index) );
		/* recur through the then branch */
		return ( zddopCCEnumerateSetPaths( cuddT(S), S->index, Path, Fn ) );
	}
} /* end of zddopCCEnumerateSetPaths */


/**Function********************************************************************

  Synopsis    [Subtracts the cost of the rows from the covering columns.]
  
  Description [Works with the temporary manager.]

  SideEffects [Calls functions which increment the global counter of paths.]

  SeeAlso     []

******************************************************************************/
static int
zddopCCRowSubtractFn( 
  unsigned Path )  /* the row number */
{
	DdNode * zColsAssoc;
	/* find the cost of this row */
	char** Slot;
	int nColumn, Value;
	if (st_find_or_add(s_RowHash, (char *)Path, &Slot) == 0) 
	{
		assert(0);
	}

	/* no need to process this row for the second time */
	if ( (*(int*)Slot) == -1 )
		return 1;
	else
	{
		Value = (*(int*)Slot);
		(*(int*)Slot) = -1;
	}
	assert( Value > 1 );

//  s_RowCost = (float)1.0/(Value-1);
	s_RowCost = (float)1.0;

	/* get the set of rows assiciated with this column */
	zColsAssoc = s_zRow2ColMap;
	while ( zColsAssoc->index < s_nRowLabels )
		if ( Path & (1<<(s_nRowLabels-1-zColsAssoc->index)) )
			zColsAssoc = cuddT( zColsAssoc );
		else
			zColsAssoc = cuddE( zColsAssoc );

	nColumn = zddCCCountPaths(zColsAssoc );
    assert( Value == nColumn );

	/* go through all the associated columns, 
	   for each column, subtract the cost of this row */
	zddopCCEnumerateSetPaths( zColsAssoc, s_nRowLabels-1, 0, zddopCCColSubtractFn );

	s_nPaths++;
	return 1;
} /* end of zddopCCRowSubtractFn */


/**Function********************************************************************

  Synopsis    [Subtracts the cost of the rows from the given column.]
  
  Description [Works with the temporary manager.]

  SideEffects [Calls functions which increment the global counter of paths.]

  SeeAlso     []

******************************************************************************/
static int
zddopCCColSubtractFn( 
  unsigned Path )  /* the row number */
{
	/* find the pointer to this column's data structure */
	colinfo * p;
	if (st_lookup(s_ColHash, (char *)Path, (char**)&p) == 0) 
	{
		assert(0);
	}

	/* use this pointer to subtract the cost - assigned previously */
	p->Cost -= s_RowCost;

	s_cTableEntries++;
	return 1;
} /* end of zddopCCColSubtractFn */


/**Function********************************************************************

  Synopsis    [Comparison function for qsort().]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
static int zddCCCompQuequeFn( const void* a, const void* b )
{
	float Value = ((*(colinfo**)b))->Cost - ((*(colinfo**)a))->Cost;
	if ( Value < -0.0001 )
		return -1;
	else if ( Value > 0.0001 )
		return 1;
	else
		return 0;
}

/**Function********************************************************************

  Synopsis    [Comparison function for qsort().]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
static int zddCCCompIntFn( const void* a, const void* b )
{
	return (int)(*((int*)a) - *((int*)b));
}



/*////////////////////////////////////////////////////////////////////////*/
/*///                    OTHER TRAVERSAL FUNCTIONS                     ///*/
/*////////////////////////////////////////////////////////////////////////*/

/**Function********************************************************************

  Synopsis    [Generalization of Cudd_zddSupSet to labeled sets.]

  Description [The function takes two labeled sets of subsets (X and Y), 
  assuming that X's and Y's labels are on top of the normal variables in X and Y,
  and that Y's labels are above X's labels (the column labels should be this way 
  to solve the covering problem); the cut level is the number of labeling variables 
  in X and Y. This function computes SupSet(X,Y) just like Cudd_zddSupSet() below 
  the cut level. Above the cut level, it composes the ZDD using labeling variables. 
  The result of this function depends on two sets of labeling variables 
  and the variables representing sets of elements. 
  Variable reordering should be disabled!]

  SideEffects []

  SeeAlso     []

******************************************************************************/

static DdNode * 
extraZddSupSetLabeled( 
  DdManager *dd,    /* the manager */
  DdNode *X,        /* the first set (rows) */
  DdNode *Y )    /* the number of column labels */
{	
	DdNode *zRes;

	/* any comb is a superset of itself */
	if ( X == Y ) 
		return X;
	/* no comb in X is superset of non-existing combs */
	if ( Y == dd->zero ) 
		return dd->zero;	
	/* any comb in X is the superset of the empty comb */
	if ( Extra_zddEmptyBelongs( dd, Y ) ) 
		return X;
	/* if X is empty, the result is empty */
	if ( X == dd->zero ) 
		return dd->zero;
	/* if X is the empty comb (and Y does not contain it!), return empty */
	if ( X == dd->one ) 
		return dd->zero;

    /* check cache */
    zRes = cuddCacheLookup2Zdd(dd, extraZddSupSetLabeled, X, Y);
    if (zRes)
    	return(zRes);
	else
	{
		DdNode *zRes0, *zRes1, *zTemp;
		int TopLevelX = dd->permZ[ X->index ];
		int TopLevelY = dd->permZ[ Y->index ];

		if ( TopLevelX < TopLevelY )
		{
			/* combinations of X without label that are supersets of combinations with Y */
			zRes0 = extraZddSupSetLabeled( dd, cuddE( X ), Y );
			if ( zRes0 == NULL )  return NULL;
			cuddRef( zRes0 );

			/* combinations of X with label that are supersets of combinations with Y */
			zRes1 = extraZddSupSetLabeled( dd, cuddT( X ), Y );
			if ( zRes1 == NULL )  
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				return NULL;
			}
			cuddRef( zRes1 );

			/* compose Res0 and Res1 with the given ZDD variable */
			zRes = cuddZddGetNode( dd, X->index, zRes1, zRes0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				return NULL;
			}
			cuddDeref( zRes0 );
			cuddDeref( zRes1 );
		}
		else if ( TopLevelX == TopLevelY )
		{
			/* combs of X without var that are supersets of combs of Y without var */
			zRes0 = extraZddSupSetLabeled( dd, cuddE( X ), cuddE( Y ) );
			if ( zRes0 == NULL )  return NULL;
			cuddRef( zRes0 );

			/* merge combs of Y with and without var */
			zTemp = cuddZddUnion( dd, cuddE( Y ), cuddT( Y ) );
			if ( zTemp == NULL )  
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				return NULL;
			}
			cuddRef( zTemp );

			/* combs of X with label that are supersets of combs in Temp */
			zRes1 = extraZddSupSetLabeled( dd, cuddT( X ), zTemp );
			if ( zRes1 == NULL )  
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zTemp );
				return NULL;
			}
			cuddRef( zRes1 );
			Cudd_RecursiveDerefZdd( dd, zTemp );

			/* compose Res0 and Res1 with the given ZDD variable */
			zRes = cuddZddGetNode( dd, X->index, zRes1, zRes0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				return NULL;
			}
			cuddDeref( zRes0 );
			cuddDeref( zRes1 );
		}
//		else /* if ( TopLevelX > TopLevelY ) */
		/////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////
		else if ( TopLevelX > TopLevelY && TopLevelY < s_nUpperLabels ) 
		{	/* fold the zdd using labeling variables */

			/* solve subproblems - the else branch */
			zRes0 = extraZddSupSetLabeled( dd, X, cuddE( Y ) );
			if ( zRes0 == NULL )  return NULL;
			cuddRef( zRes0 );

			/* solve subproblems - the then branch */
			zRes1 = extraZddSupSetLabeled( dd, X, cuddT( Y ) );
			if ( zRes1 == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				return NULL;
			}
			cuddRef( zRes1 );

			/* compose Res0 and Res1 with the given ZDD variable (labeling variable) */
			zRes = cuddZddGetNode( dd, Y->index, zRes1, zRes0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				return NULL;
			}
			cuddDeref( zRes0 );
			cuddDeref( zRes1 );
		}
		else /* if ( TopLevelX > TopLevelY && TopLevelY >= s_nUpperLabels ) */
		/////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////
		{
			/* combs of X that are supersets of combs of Y without label */
			zRes = extraZddSupSetLabeled( dd, X, cuddE( Y ) );
			if ( zRes == NULL )  return NULL;
		}

		/* insert the result into cache */
	    cuddCacheInsert2(dd, extraZddSupSetLabeled, X, Y, zRes);
		return zRes;
	}
} /* end of extraZddSupSetLabeled */



/**Function********************************************************************

  Synopsis    [Generalization of Cudd_zddSupSet to labeled sets.]

  Description [The function takes two labeled sets of subsets (X and Y), 
  assuming that X's and Y's labels are on top of the normal variables in X and Y,
  and that Y's labels are above X's labels (the column labels should be this way 
  to solve the covering problem); the cut level is the number of labeling variables 
  in X and Y. This function computes SupSet(X,Y) just like Cudd_zddSupSet() below 
  the cut level. Above the cut level, it composes the ZDD using labeling variables. 
  The result of this function depends on two sets of labeling variables 
  and the variables representing sets of elements. 
  Variable reordering should be disabled!]

  SideEffects []

  SeeAlso     []

******************************************************************************/

static DdNode * 
extraZddSupSetLabeled2( 
  DdManager *dd,    /* the manager */
  DdNode *X,        /* the first set (rows) */
  DdNode *Y )    /* the number of column labels */
{	
	DdNode *zRes;

	/* any comb is a superset of itself */
	if ( X == Y ) 
		return X;
	/* no comb in X is superset of non-existing combs */
	if ( Y == dd->zero ) 
		return dd->zero;	
	/* any comb in X is the superset of the empty comb */
	if ( Extra_zddEmptyBelongs( dd, Y ) ) 
		return X;
	/* if X is empty, the result is empty */
	if ( X == dd->zero ) 
		return dd->zero;
	/* if X is the empty comb (and Y does not contain it!), return empty */
	if ( X == dd->one ) 
		return dd->zero;

    /* check cache */
    zRes = cuddCacheLookup2Zdd(dd, extraZddSupSetLabeled2, X, Y);
    if (zRes)
    	return(zRes);
	else
	{
		DdNode *zRes0, *zRes1, *zTemp;

		int TopLevelX = dd->permZ[ X->index ];
		int TopLevelY = dd->permZ[ Y->index ];
		int Threshold = dd->sizeZ - s_nLowerLabels;

		if ( TopLevelX < TopLevelY )
		{
			/* combinations of X without label that are supersets of combinations with Y */
			zRes0 = extraZddSupSetLabeled2( dd, cuddE( X ), Y );
			if ( zRes0 == NULL )  return NULL;
			cuddRef( zRes0 );

			/* combinations of X with label that are supersets of combinations with Y */
			zRes1 = extraZddSupSetLabeled2( dd, cuddT( X ), Y );
			if ( zRes1 == NULL )  
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				return NULL;
			}
			cuddRef( zRes1 );

			/* compose Res0 and Res1 with the given ZDD variable */
//			zRes = cuddZddGetNode( dd, X->index, zRes1, zRes0 );
//			if ( zRes == NULL ) 
//			{
//				Cudd_RecursiveDerefZdd( dd, zRes0 );
//				Cudd_RecursiveDerefZdd( dd, zRes1 );
//				return NULL;
//			}
//			cuddDeref( zRes0 );
//			cuddDeref( zRes1 );

			/* abstract variables above the threshold */
			while ( zRes0->index < Threshold )
			{
				zTemp = zRes0;
				zRes0 = cuddZddUnion( dd, cuddE(zRes0), cuddT(zRes0) );
				if ( zRes0 == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zTemp );
					Cudd_RecursiveDerefZdd( dd, zRes1 );
					return NULL;
				}
				cuddRef( zRes0 );
				Cudd_RecursiveDerefZdd( dd, zTemp );
			}
			while ( zRes1->index < Threshold )
			{
				zTemp = zRes1;
				zRes1 = cuddZddUnion( dd, cuddE(zRes1), cuddT(zRes1) );
				if ( zRes1 == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zTemp );
					Cudd_RecursiveDerefZdd( dd, zRes0 );
					return NULL;
				}
				cuddRef( zRes1 );
				Cudd_RecursiveDerefZdd( dd, zTemp );
			}

			/* compute the union of Res0 and Res1 */
			zRes = cuddZddUnion( dd, zRes0, zRes1 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				return NULL;
			}
			cuddRef( zRes );
			Cudd_RecursiveDerefZdd( dd, zRes0 );
			Cudd_RecursiveDerefZdd( dd, zRes1 );
			cuddDeref( zRes );
		}
		else if ( TopLevelX == TopLevelY )
		{
			/* combs of X without var that are supersets of combs of Y without var */
			zRes0 = extraZddSupSetLabeled2( dd, cuddE( X ), cuddE( Y ) );
			if ( zRes0 == NULL )  return NULL;
			cuddRef( zRes0 );

			/* merge combs of Y with and without var */
			zTemp = cuddZddUnion( dd, cuddE( Y ), cuddT( Y ) );
			if ( zTemp == NULL )  
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				return NULL;
			}
			cuddRef( zTemp );

			/* combs of X with label that are supersets of combs in Temp */
			zRes1 = extraZddSupSetLabeled2( dd, cuddT( X ), zTemp );
			if ( zRes1 == NULL )  
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zTemp );
				return NULL;
			}
			cuddRef( zRes1 );
			Cudd_RecursiveDerefZdd( dd, zTemp );

			/* compose Res0 and Res1 with the given ZDD variable */
//			zRes = cuddZddGetNode( dd, X->index, zRes1, zRes0 );
//			if ( zRes == NULL ) 
//			{
//				Cudd_RecursiveDerefZdd( dd, zRes0 );
//				Cudd_RecursiveDerefZdd( dd, zRes1 );
//				return NULL;
//			}
//			cuddDeref( zRes0 );
//			cuddDeref( zRes1 );

			/* abstract variables above the threshold */
			while ( zRes0->index < Threshold )
			{
				zTemp = zRes0;
				zRes0 = cuddZddUnion( dd, cuddE(zRes0), cuddT(zRes0) );
				if ( zRes0 == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zTemp );
					Cudd_RecursiveDerefZdd( dd, zRes1 );
					return NULL;
				}
				cuddRef( zRes0 );
				Cudd_RecursiveDerefZdd( dd, zTemp );
			}
			while ( zRes1->index < Threshold )
			{
				zTemp = zRes1;
				zRes1 = cuddZddUnion( dd, cuddE(zRes1), cuddT(zRes1) );
				if ( zRes1 == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zTemp );
					Cudd_RecursiveDerefZdd( dd, zRes0 );
					return NULL;
				}
				cuddRef( zRes1 );
				Cudd_RecursiveDerefZdd( dd, zTemp );
			}

			/* compute the union of Res0 and Res1 */
			zRes = cuddZddUnion( dd, zRes0, zRes1 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				return NULL;
			}
			cuddRef( zRes );
			Cudd_RecursiveDerefZdd( dd, zRes0 );
			Cudd_RecursiveDerefZdd( dd, zRes1 );
			cuddDeref( zRes );
		}
//		else /* if ( TopLevelX > TopLevelY ) */
		/////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////
		else if ( TopLevelX > TopLevelY && TopLevelY < s_nUpperLabels ) 
		{	/* fold the zdd using labeling variables */

			/* solve subproblems - the else branch */
			zRes0 = extraZddSupSetLabeled2( dd, X, cuddE( Y ) );
			if ( zRes0 == NULL )  return NULL;
			cuddRef( zRes0 );

			/* solve subproblems - the then branch */
			zRes1 = extraZddSupSetLabeled2( dd, X, cuddT( Y ) );
			if ( zRes1 == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				return NULL;
			}
			cuddRef( zRes1 );

			/* compose Res0 and Res1 with the given ZDD variable (labeling variable) */
			zRes = cuddZddGetNode( dd, Y->index, zRes1, zRes0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				return NULL;
			}
			cuddDeref( zRes0 );
			cuddDeref( zRes1 );
		}
		else /* if ( TopLevelX > TopLevelY && TopLevelY >= s_nUpperLabels ) */
		/////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////
		{
			/* combs of X that are supersets of combs of Y without label */
			zRes = extraZddSupSetLabeled2( dd, X, cuddE( Y ) );
			if ( zRes == NULL )  return NULL;
		}

		/* insert the result into cache */
	    cuddCacheInsert2(dd, extraZddSupSetLabeled2, X, Y, zRes);
		return zRes;
	}
} /* end of extraZddSupSetLabeled2 */



/**Function********************************************************************

  Synopsis    [Generalization of Cudd_zddSubSet to labeled sets.]

  Description [The function takes two labeled sets of subsets (X and Y), 
  assuming that X's and Y's labels are on top of the normal variables in X and Y,
  and that Y's labels are above X's labels (the column labels should be this way 
  to solve the covering problem); the cut level is the number of labeling variables 
  in X and Y. This function computes SubSet(X,Y) just like Cudd_zddSubSet() below 
  the cut level. Above the cut level, it composes the ZDD using labeling variables. 
  The final result of applying this function depends on the two sets of labeling 
  variables and the variables representing sets of elements. No wrappers are provided
  because the variable reordering should be disabled!]

  SideEffects []

  SeeAlso     []

******************************************************************************/
static DdNode * 
extraZddSubSetLabeled( 
  DdManager *dd,    /* the manager */
  DdNode *X,        /* the first set (rows) */
  DdNode *Y )    /* the number of column labels */
{
	DdNode *zRes;

	/* any comb is a subset of itself */
	if ( X == Y ) 
		return X;
	/* if X is empty, the result is empty */
	if ( X == dd->zero ) 
		return dd->zero;
	/* combs in X are notsubsets of non-existant combs in Y */
	if ( Y == dd->zero ) 
		return dd->zero;
	/* the empty comb is contained in all combs of Y */
	if ( X == dd->one ) 
		return dd->one;
	/* only {()} is the subset of {()} */
	if ( Y == dd->one ) /* check whether the empty combination is present in X */ 
		return ( Extra_zddEmptyBelongs( dd, X )? dd->one: dd->zero );

    /* check cache */
    zRes = cuddCacheLookup2Zdd(dd, extraZddSubSetLabeled, X, Y);
    if (zRes)
    	return(zRes);
	else
	{
		DdNode *zRes0, *zRes1, *zTemp;
		int TopLevelX = dd->permZ[ X->index ];
		int TopLevelY = dd->permZ[ Y->index ];

		/////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////
		if ( TopLevelX < TopLevelY && TopLevelX < s_nUpperLabels ) 
		{	/* fold the zdd using labeling variables */

			/* solve subproblems - the else branch */
			zRes0 = extraZddSubSetLabeled( dd, cuddE( X ), Y );
			if ( zRes0 == NULL )  return NULL;
			cuddRef( zRes0 );

			/* solve subproblems - the then branch */
			zRes1 = extraZddSubSetLabeled( dd, cuddT( X ), Y );
			if ( zRes1 == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				return NULL;
			}
			cuddRef( zRes1 );

			/* compose Res0 and Res1 with the given ZDD variable (labeling variable) */
			zRes = cuddZddGetNode( dd, X->index, zRes1, zRes0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				return NULL;
			}
			cuddDeref( zRes0 );
			cuddDeref( zRes1 );
		}
		else /* if ( TopLevelX < TopLevelY && TopLevelX >= s_nUpperLabels ) */
		/////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////
		if ( TopLevelX < TopLevelY )
		{
			/* compute combs of X without var that are notsubsets of combs with Y */
			zRes = extraZddSubSetLabeled( dd, cuddE( X ), Y );
			if ( zRes == NULL )  return NULL;
		}
		else if ( TopLevelX == TopLevelY )
		{
			/* merge combs of Y with and without var */
			zTemp = cuddZddUnion( dd, cuddE( Y ), cuddT( Y ) );
			if ( zTemp == NULL )
				return NULL;
			cuddRef( zTemp );

			/* compute combs of X without var that are notsubsets of combs is Temp */
			zRes0 = extraZddSubSetLabeled( dd, cuddE( X ), zTemp );
			if ( zRes0 == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zTemp );
				return NULL;
			}
			cuddRef( zRes0 );
			Cudd_RecursiveDerefZdd( dd, zTemp );

			/* combs of X with var that are notsubsets of combs in Y with var */
			zRes1 = extraZddSubSetLabeled( dd, cuddT( X ), cuddT( Y ) );
			if ( zRes1 == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				return NULL;
			}
			cuddRef( zRes1 );

			/* compose Res0 and Res1 with the given ZDD variable */
			zRes = cuddZddGetNode( dd, X->index, zRes1, zRes0 );
			if ( zRes == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				return NULL;
			}
			cuddDeref( zRes0 );
			cuddDeref( zRes1 );
		}
		else /* if ( TopLevelX > TopLevelY ) */
		{
			/* merge combs of Y with and without var */
			zTemp = cuddZddUnion( dd, cuddE( Y ), cuddT( Y ) );
			if ( zTemp == NULL )  return NULL;
			cuddRef( zTemp );

			/* compute combs that are notsubsets of Temp */
			zRes = extraZddSubSetLabeled( dd, X, zTemp );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zTemp );
				return NULL;
			}
			cuddRef( zRes );
			Cudd_RecursiveDerefZdd( dd, zTemp );
			cuddDeref( zRes );
		}

		/* insert the result into cache */
	    cuddCacheInsert2(dd, extraZddSubSetLabeled, X, Y, zRes);
		return zRes;
	}

} /* end of extraZddSubSetLabeled */


/**Function********************************************************************

  Synopsis    [Finds the number of the column associated with max number of rows.]

  Description [Finds the number of the column associated with max number of rows.
  In the returned integer, the most-significant 16 bits store the column number, 
  the least-significant 16 bits store the number of rows associated with this column.]

  SideEffects [Fills out the local cache implemented using st_table.]

  SeeAlso     []

******************************************************************************/
static unsigned 
extraZddColumnMax( 
  DdManager *dd,    /* the manager */
  DdNode *Map )     /* the mapping of columns into rows */
{
    int HashValue;

	if ( Map == dd->zero )
		return 0;
	if ( Map == dd->one )
		return 1;

    /* check cache */
    HashValue = (unsigned)Map % TABLESIZE;
    if ( HTable[HashValue].S == Map )
        return HTable[HashValue].Res;
	else
	{
		unsigned Result, Result0, Result1;
		int TopLevel = dd->permZ[ Map->index ];
		assert( TopLevel == Map->index );

		/* solve subproblems */
		Result1 = extraZddColumnMax( dd, cuddT( Map ) );
		Result0 = extraZddColumnMax( dd, cuddE( Map ) );

		/* create solution depending on the top level */
		if ( TopLevel < s_nColLabels )
		{ 
			int cRows0 = (Result0 & 0xffff);
			int cRows1 = (Result1 & 0xffff);

			if ( cRows0 >= cRows1 ) 
			{ /* prefer the else branch */
				/* no need to set the labeling bit */
				Result = Result0;
			}
			else /* if ( cRows0 < cRows1 ) */
			{ /* prefer the then branch */
				/* set the labeling bit */
				Result = (Result1 | ( 1 << (s_nColLabels-1-TopLevel + 16) ));
			}			
		}
		else /* if ( TopLevel >= CutLevel ) */
		{ 
			/* find the number of paths from this node to terminal one */
			Result = Result0 + Result1;
		}

        HTable[HashValue].S = Map;
        HTable[HashValue].Res = Result;
		return Result;
	}
} /* end of extraZddColumnMax */



/**Function********************************************************************

  Synopsis    [Counts the number of paths (true assignments) in a ZDD.]

  Description []

  SideEffects [Fills out a local cache.]

  SeeAlso     []

******************************************************************************/
static int 
zddCCCountPaths(
  DdNode *S )     /* the set of assignments (rows or columns) */
{
	if ( S == s_ddnew->zero )
		return 0;
	else if ( S == s_ddnew->one )
		return 1;
	else
	{
		/* check cache */
		int HashValue = (unsigned)S % TABLESIZE;
		if ( HTable[HashValue].S == S )
			return HTable[HashValue].Res;
		else
		{
			int Result = zddCCCountPaths(cuddE( S ) ) +
						 zddCCCountPaths(cuddT( S ) );

			HTable[HashValue].S = S;
			HTable[HashValue].Res = Result;
			return Result;
		}
	}
} /* end of zddCCCountPaths */

