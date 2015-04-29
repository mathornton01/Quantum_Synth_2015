/**CFile***********************************************************************

  FileName    [bMisc.c]

  PackageName [extra]

  Synopsis    [Experimental version of some BDD operators.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_bddEncodingBinary()
				<li> Extra_bddEncodingNonStrict()
				</ul>
			Internal procedures included in this module:
				<ul>
				</ul>
			Static procedures included in this module:
				<ul>
				<li> EvaluateEncodings_rec()
				<li> CreateTheCodes_rec()
				</ul>
	      ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$bMisc.c, v.1.2, July 8, 2001, alanmi $]

******************************************************************************/

#include "util.h"
#include "cuddInt.h"
#include "time.h"
#include "extra.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

// the table to store cofactor operations
#define _TABLESIZE_COF  51113
typedef struct 
{
	unsigned Sign;  
	DdNode * Arg1;  
} _HashEntry_cof;
_HashEntry_cof HHTable1[_TABLESIZE_COF];

// the table to store the result of computation of the number of minterms
#define _TABLESIZE_MINT  15113
typedef struct 
{
	DdNode * Arg1;  
	unsigned Arg2;  
	unsigned Res;  
} _HashEntry_mint;
_HashEntry_mint HHTable2[_TABLESIZE_MINT];

// the signature used for hashing
static unsigned s_Signature = 1;

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

// because the proposed solution to the optimal encoding problem has exponential complexity
// we limit the depth of the branch and bound procedure to 5 levels
static int s_MaxDepth = 5;      

static int s_nVarsBest;          // the number of vars in the best ordering
static int s_VarOrderBest[32];   // storing the best ordering of vars in the "simple encoding"
static int s_VarOrderCur[32];    // storing the current ordering of vars
 
// the place to store the supports of the encoded function
static DdNode * s_Field[8][256]; // the size should be K, 2^K, where K is no less than MaxDepth
static DdNode * s_Encoded;       // this is the original function
static DdNode * s_VarAll;        // the set of all column variables
static int s_MultiStart;         // the total number of encoding variables used
// the array field now stores the supports

static DdNode ** s_pbTemp;       // the temporary storage for the columns

static int s_BackTracks;
static int s_BackTrackLimit = 100;

static DdNode * s_Terminal;      // the terminal value for counting minterms


static int s_EncodingVarsLevel;


/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static DdNode * CreateTheCodes_rec( DdManager * dd, DdNode * bEncoded, int Level, DdNode ** pCVars );
static void EvaluateEncodings_rec( DdManager * dd, DdNode * bVarsCol, int nVarsCol, int nMulti, int Level );
// functions called from EvaluateEncodings_rec()
static DdNode * ComputeVarSetAndCountMinterms( DdManager * dd, DdNode * bVars, DdNode * bVarTop, unsigned * Cost );
static DdNode * ComputeVarSetAndCountMinterms2( DdManager * dd, DdNode * bVars, DdNode * bVarTop, unsigned * Cost );
unsigned Extra_CountCofactorMinterms( DdManager * dd, DdNode * bFunc, DdNode * bVarsCof, DdNode * bVarsAll );
static unsigned Extra_CountMintermsSimple( DdNode * bFunc, unsigned max );

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Performs the binary encoding of the set of function using the given vars.]

  Description [Performs a straight binary encoding of the set of functions using 
  the variable cubes formed from the given set of variables. ]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * 
Extra_bddEncodingBinary( 
  DdManager * dd, 
  DdNode ** pbFuncs,    // pbFuncs is the array of columns to be encoded
  int nFuncs,           // nFuncs is the number of columns in the array
  DdNode ** pbVars,     // pbVars is the array of variables to use for the codes
  int nVars )           // nVars is the column multiplicity, [log2(nFuncs)]
{
	int i;
	DdNode * bResult;
	DdNode * bCube, * bTemp, * bProd;

	assert( nVars >= Extra_Base2Log(nFuncs) );

	bResult = b0;   Cudd_Ref( bResult );
	for ( i = 0; i < nFuncs; i++ )
	{
		bCube   = Extra_bddBitsToCube( dd, i, nVars, pbVars );   Cudd_Ref( bCube );
		bProd   = Cudd_bddAnd( dd, bCube, pbFuncs[i] );         Cudd_Ref( bProd );
		Cudd_RecursiveDeref( dd, bCube );

		bResult = Cudd_bddOr( dd, bProd, bTemp = bResult );     Cudd_Ref( bResult );
		Cudd_RecursiveDeref( dd, bTemp );
		Cudd_RecursiveDeref( dd, bProd );
	}

	Cudd_Deref( bResult );
	return bResult;
} /* end of Extra_bddEncodingBinary */


/**Function********************************************************************

  Synopsis    [Solves the column encoding problem using a sophisticated method.]

  Description [The encoding is based on the idea of deriving functions which 
  depend on only one variable, which corresponds to the case of non-disjoint 
  decompostion. It is assumed that the variables pCVars are ordered below the variables
  representing the solumns, and the first variable pCVars[0] is the topmost one.]

  SideEffects []

  SeeAlso     [Extra_bddEncodingBinary]

******************************************************************************/

DdNode * 
Extra_bddEncodingNonStrict( 
  DdManager * dd, 
  DdNode ** pbColumns,   // pbColumns is the array of columns to be encoded;
  int nColumns,          // nColumns is the number of columns in the array
  DdNode * bVarsCol,     // bVarsCol is the cube of variables on which the columns depend
  DdNode ** pCVars,      // pCVars is the array of variables to use for the codes
  int nMulti,            // nMulti is the column multiplicity, [log2(nColumns)]
  int * pSimple )        // pSimple gets the number of code variables taken from the input varibles without change
{
	DdNode * bEncoded, * bResult;
	int nVarsCol = Cudd_SupportSize(dd,bVarsCol);
	long clk;

	// cannot work with more that 32-bit codes
	assert( nMulti < 32 );

	// perform the preliminary encoding using the straight binary code
	bEncoded = Extra_bddEncodingBinary( dd, pbColumns, nColumns, pCVars, nMulti );   Cudd_Ref( bEncoded );
	//printf( "Node count = %d", Cudd_DagSize(bEncoded) );

	// set the backgroup value for counting minterms
	s_Terminal = b0;
	// set the level of the encoding variables
	s_EncodingVarsLevel = dd->invperm[pCVars[0]->index];

	// the current number of backtracks
	s_BackTracks = 0;
	// the variables that are cofactored on the topmost level where everything starts (no vars)
	s_Field[0][0] = b1;   
	// the size of the best set of "simple" encoding variables found so far
	s_nVarsBest   = 0;

    // set the relation to be accessible to traversal procedures
	s_Encoded = bEncoded;
	// the set of all vars to be accessible to traversal procedures
	s_VarAll  = bVarsCol;
	// the column multiplicity
	s_MultiStart  = nMulti;


	clk = clock();
	// find the simplest encoding
	if ( nColumns > 2 )
	EvaluateEncodings_rec( dd, bVarsCol, nVarsCol, nMulti, 1 );
//	printf( "The number of backtracks = %d\n", s_BackTracks );
//	s_EncSearchTime += clock() - clk;

	// allocate the temporary storage for the columns
	s_pbTemp = (DdNode **) malloc( nColumns * sizeof(DdNode *) );

//	clk = clock();
	bResult = CreateTheCodes_rec( dd, bEncoded, 0, pCVars );   Cudd_Ref( bResult );
//	s_EncComputeTime += clock() - clk;
	
	// delocate the preliminarily encoded set
	Cudd_RecursiveDeref( dd, bEncoded );
//	Cudd_RecursiveDeref( dd, aEncoded );

	free( s_pbTemp );

	*pSimple = s_nVarsBest;
	Cudd_Deref( bResult );
	return bResult;
}
 
/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Computes the non-strict codes when evaluation is finished.]

  Description [The information about the best code is stored in s_VarOrderBest,
  which has s_nVarsBest entries.]

  SideEffects [None]

******************************************************************************/
DdNode * CreateTheCodes_rec( DdManager * dd, DdNode * bEncoded, int Level, DdNode ** pCVars )
// bEncoded is the preliminarily encoded set of columns
// Level is the current level in the recursion
// pCVars are the variables to be used for encoding
{
	DdNode * bRes;
	if ( Level == s_nVarsBest )
	{ // the terminal case, when we need to remap the encoded function 
		// from the preliminary encoded variables to the new ones
		st_table * CutNodes;
		int nCols;
//		double nMints;
/*
#ifdef _DEBUG

		{
		DdNode * bTemp;
		// make sure that the given number of variables is enough
		bTemp  = Cudd_bddExistAbstract( dd, bEncoded, s_VarAll );           Cudd_Ref( bTemp );
//		nMints = Cudd_CountMinterm( dd, bTemp, s_MultiStart );
		nMints = Extra_CountMintermsSimple( bTemp, (1<<s_MultiStart) );
		if ( nMints > Extra_Power2( s_MultiStart-Level ) ) 
		{  // the number of minterms is too large to encode the columns 
		   // using the given minimum number of encoding variables
			assert( 0 );
		}
		Cudd_RecursiveDeref( dd, bTemp );
		}
#endif
*/
		// get the columns to be re-encoded
		CutNodes = Extra_bddNodePathsUnderCut( dd, bEncoded, s_EncodingVarsLevel );
		// LUT size is the cut level because because the temporary encoding variables 
		// are above the functional variables - this is not true!!!
		// the temporary variables are below!

		// put the entries from the table into the temporary array
		{ 
			st_generator * gen;
			DdNode * bColumn, * bCode;
			nCols = 0;
			st_foreach_item( CutNodes, gen, (char**)&bCode, (char**)&bColumn ) 
			{
				if ( bCode == b0 )
				{ // the unused part of the columns
					Cudd_RecursiveDeref( dd, bColumn );
					Cudd_RecursiveDeref( dd, bCode );
					continue;
				}
				else
				{
					s_pbTemp[ nCols ] = bColumn; // takes ref
					Cudd_RecursiveDeref( dd, bCode );
					nCols++;
				}
			}
			st_free_table( CutNodes );
//			assert( nCols == (int)nMints );
		}

		// encode the columns
		if ( s_MultiStart-Level == 0 ) // we reached the bottom level of recursion
		{
			assert( nCols       == 1 );
//			assert( (int)nMints == 1 );
			bRes = s_pbTemp[0];     Cudd_Ref( bRes );
		}
		else
		{
			bRes = Extra_bddEncodingBinary( dd, s_pbTemp, nCols, pCVars+Level, s_MultiStart-Level ); Cudd_Ref( bRes );
		}

		// deref the columns
		{
			int i;
			for ( i = 0; i < nCols; i++ )
				Cudd_RecursiveDeref( dd, s_pbTemp[i] );
		}
	}
	else
	{
		// cofactor the problem as specified in the best solution
		DdNode * bCof0,  * bCof1;
		DdNode * bRes0,  * bRes1;
		DdNode * bProd0, * bProd1;
		DdNode * bTemp;
		DdNode * bVarNext = dd->vars[ s_VarOrderBest[Level] ];

		bCof0  = Cudd_Cofactor( dd, bEncoded,  Cudd_Not( bVarNext ) );   Cudd_Ref( bCof0 );
		bCof1  = Cudd_Cofactor( dd, bEncoded,            bVarNext   );   Cudd_Ref( bCof1 );

		// call recursively
		bRes0 = CreateTheCodes_rec( dd, bCof0, Level+1, pCVars );  Cudd_Ref( bRes0 );
		bRes1 = CreateTheCodes_rec( dd, bCof1, Level+1, pCVars );  Cudd_Ref( bRes1 );

		Cudd_RecursiveDeref( dd, bCof0 );
		Cudd_RecursiveDeref( dd, bCof1 );

		// compose the result using the identity (bVarNext <=> pCVars[Level])  - this is wrong!
		// compose the result as follows: x'y'F0 + xyF1
		bProd0 = Cudd_bddAnd( dd, Cudd_Not(bVarNext), Cudd_Not(pCVars[Level]) );   Cudd_Ref( bProd0 );
		bProd1 = Cudd_bddAnd( dd,          bVarNext ,          pCVars[Level]  );   Cudd_Ref( bProd1 );

		bProd0 = Cudd_bddAnd( dd, bTemp = bProd0, bRes0 );   Cudd_Ref( bProd0 );
		Cudd_RecursiveDeref( dd, bTemp );
		Cudd_RecursiveDeref( dd, bRes0 );

		bProd1 = Cudd_bddAnd( dd, bTemp = bProd1, bRes1 );   Cudd_Ref( bProd1 );
		Cudd_RecursiveDeref( dd, bTemp );
		Cudd_RecursiveDeref( dd, bRes1 );

		bRes = Cudd_bddOr( dd, bProd0, bProd1 );             Cudd_Ref( bRes );

		Cudd_RecursiveDeref( dd, bProd0 );
		Cudd_RecursiveDeref( dd, bProd1 );
	}
	Cudd_Deref( bRes );
	return bRes;
}

/**Function********************************************************************

  Synopsis    [Computes the current set of variables and counts the number of minterms.]

  Description [Old implementation.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void EvaluateEncodings_rec( DdManager * dd, DdNode * bVarsCol, int nVarsCol, int nMulti, int Level )
// bVarsCol is the set of remaining variables
// nVarsCol is the number of remaining variables
// nMulti is the number of encoding variables to be used
// Level is the level of recursion, from which this function is called
// if we successfully finish this procedure, Level also stands for how many encoding variabled we saved
{
	int i, k;
	int nEntries = (1<<(Level-1)); // the number of entries in the field of the previous level
	DdNode * bVars0, * bVars1; // the cofactors
	unsigned nMint0, nMint1;   // the number of minterms
	DdNode * bTempV;
	DdNode * bVarTop;
	int fBreak;


	// there is no need to search above this level
	if ( Level > s_MaxDepth )
		return;

	// if there are no variables left, quit the research
	if ( bVarsCol == b1 )
		return;

	if ( s_BackTracks > s_BackTrackLimit )
		return;

	s_BackTracks++;

	// otherwise, go through the remaining variables
	for ( bTempV = bVarsCol; bTempV != b1; bTempV = cuddT(bTempV) )
	{
		// the currently tested variable
		bVarTop = dd->vars[bTempV->index];

		// put it into the array
		s_VarOrderCur[Level-1] = bTempV->index;

		// go through the entries and fill them out by cofactoring
		fBreak = 0;
		for ( i = 0; i < nEntries; i++ )
		{
			bVars0 = ComputeVarSetAndCountMinterms( dd, s_Field[Level-1][i], Cudd_Not(bVarTop), &nMint0 );
			Cudd_Ref( bVars0 );

			if ( nMint0 > Extra_Power2( nMulti-1 ) ) 
			{
				// there is no way to encode - dereference and return
				Cudd_RecursiveDeref( dd, bVars0 );
				fBreak = 1;
				break;
			}

			bVars1 = ComputeVarSetAndCountMinterms( dd, s_Field[Level-1][i], bVarTop, &nMint1 );
			Cudd_Ref( bVars1 );

			if ( nMint1 > Extra_Power2( nMulti-1 ) ) 
			{
				// there is no way to encode - dereference and return
				Cudd_RecursiveDeref( dd, bVars0 );
				Cudd_RecursiveDeref( dd, bVars1 );
				fBreak = 1;
				break;
			}

			// otherwise, add these two cofactors
			s_Field[Level][2*i + 0] = bVars0; // takes ref
			s_Field[Level][2*i + 1] = bVars1; // takes ref
		} 

		if ( !fBreak )
		{
			DdNode * bVarsRem;
			// if we ended up here, it means that the cofactors w.r.t. variable bVarTop satisfy the condition
			// save this situation
			if ( s_nVarsBest < Level )
			{
				s_nVarsBest = Level;
				// copy the variable assignment
				for ( k = 0; k < Level; k++ )
					s_VarOrderBest[k] = s_VarOrderCur[k];
			}

			// call recursively
			// get the new variable set
			if ( nMulti-1 > 0 )
			{
				bVarsRem = Cudd_bddExistAbstract( dd, bVarsCol, bVarTop );   Cudd_Ref( bVarsRem );
				EvaluateEncodings_rec( dd, bVarsRem, nVarsCol-1, nMulti-1, Level+1 );
				Cudd_RecursiveDeref( dd, bVarsRem );
			}
		}

		// deref the contents of the array
		for ( k = 0; k < i; k++ )
		{
			Cudd_RecursiveDeref( dd, s_Field[Level][2*k + 0] );
			Cudd_RecursiveDeref( dd, s_Field[Level][2*k + 1] );
		}

		// if the solution is found, there is no need to continue
		if ( s_nVarsBest == s_MaxDepth )
			return;

		// if the solution is found, there is no need to continue
		if ( s_nVarsBest == s_MultiStart )
			return;
	}
	// at this point, we have tried all possible directions in the space of variables
}

/**Function********************************************************************

  Synopsis    [Computes the current set of variables and counts the number of minterms.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * ComputeVarSetAndCountMinterms( DdManager * dd, DdNode * bVars, DdNode * bVarTop, unsigned * Cost )
// takes bVars - the variables cofactored so far (some of them may be in negative polarity)
// bVarTop - the topmost variable w.r.t. which to cofactor (may be in negative polarity)
// returns the cost and the new set of variables (bVars & bVarTop)
{
	DdNode * bVarsRes;

	// get the resulting set of variables
	bVarsRes = Cudd_bddAnd( dd, bVars, bVarTop );  Cudd_Ref( bVarsRes );

	// increment signature before calling Cudd_CountCofactorMinterms()
	s_Signature++;
	*Cost = Extra_CountCofactorMinterms( dd, s_Encoded, bVarsRes, s_VarAll );

	Cudd_Deref( bVarsRes );
//	s_CountCalls++;
	return bVarsRes;
}

/**Function********************************************************************

  Synopsis    [Computes the current set of variables and counts the number of minterms.]

  Description [The old implementation, which is approximately 4 times slower.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * ComputeVarSetAndCountMinterms2( DdManager * dd, DdNode * bVars, DdNode * bVarTop, unsigned * Cost )
{
	DdNode * bVarsRes;
	DdNode * bCof, * bFun;

	bVarsRes = Cudd_bddAnd( dd, bVars, bVarTop );             Cudd_Ref( bVarsRes );

	bCof  = Cudd_Cofactor( dd, s_Encoded,  bVarsRes );        Cudd_Ref( bCof );
	bFun  = Cudd_bddExistAbstract( dd, bCof, s_VarAll );      Cudd_Ref( bFun );
	*Cost = (unsigned)Cudd_CountMinterm( dd, bFun, s_MultiStart );
	Cudd_RecursiveDeref( dd, bFun );
	Cudd_RecursiveDeref( dd, bCof );

	Cudd_Deref( bVarsRes );
//	s_CountCalls++;
	return bVarsRes;
}


/**Function********************************************************************

  Synopsis    [Counts the number of encoding minterms pointed to by the cofactor of the function.]

  Description []

  SideEffects [None]

******************************************************************************/
unsigned Extra_CountCofactorMinterms( DdManager * dd, DdNode * bFunc, DdNode * bVarsCof, DdNode * bVarsAll )
// this function computes how many minterms depending on the encoding variables
// are there in the cofactor of bFunc w.r.t. variables bVarsCof
// bFunc is assumed to depend on variables s_VarsAll
// the variables s_VarsAll should be ordered above the encoding variables
{
	unsigned HKey;
	DdNode * bFuncR;

	// if the function is zero, there are no minterms
//	if ( bFunc == b0 )
//		return 0;

//	if ( st_lookup(Visited, (char*)bFunc, NULL) )
//		return 0;

//	HKey = hashKey2c( s_Signature, bFuncR );
//	if ( HHTable1[HKey].Sign == s_Signature && HHTable1[HKey].Arg1 == bFuncR ) // this node is visited
//		return 0;


	// check the hash-table 
	bFuncR = Cudd_Regular(bFunc);
//	HKey = hashKey2( s_Signature, bFuncR, _TABLESIZE_COF );
	HKey = hashKey2( s_Signature, bFunc, _TABLESIZE_COF );
	for ( ;  HHTable1[HKey].Sign == s_Signature; HKey = (HKey+1) % _TABLESIZE_COF )
//		if ( HHTable1[HKey].Arg1 == bFuncR ) // this node is visited
		if ( HHTable1[HKey].Arg1 == bFunc ) // this node is visited
			return 0;


	// if the function is already the code
	if ( dd->perm[bFuncR->index] >= s_EncodingVarsLevel )
	{
//		st_insert(Visited, (char*)bFunc, NULL);

//		HHTable1[HKey].Sign = s_Signature;
//		HHTable1[HKey].Arg1 = bFuncR;

		assert( HHTable1[HKey].Sign != s_Signature );
		HHTable1[HKey].Sign = s_Signature;
//		HHTable1[HKey].Arg1 = bFuncR;
		HHTable1[HKey].Arg1 = bFunc;

		return Extra_CountMintermsSimple( bFunc, (1<<s_MultiStart) );
	}
	else
	{
		DdNode * bFunc0,    * bFunc1;
		DdNode * bVarsCof0, * bVarsCof1;
		DdNode * bVarsCofR = Cudd_Regular(bVarsCof);
		unsigned Res;

		// get the levels
		int LevelF = dd->perm[bFuncR->index];
		int LevelC = cuddI(dd,bVarsCofR->index);
		int LevelA = dd->perm[bVarsAll->index];

		int LevelTop = LevelF;

		if ( LevelTop > LevelC )
			 LevelTop = LevelC;

		if ( LevelTop > LevelA )
			 LevelTop = LevelA;

		// the top var in the function or in cofactoring vars always belongs to the set of all vars
		assert( !( LevelTop == LevelF || LevelTop == LevelC ) || LevelTop == LevelA );

		// cofactor the function
		if ( LevelTop == LevelF )
		{
			if ( bFuncR != bFunc ) // bFunc is complemented 
			{
				bFunc0 = Cudd_Not( cuddE(bFuncR) );
				bFunc1 = Cudd_Not( cuddT(bFuncR) );
			}
			else
			{
				bFunc0 = cuddE(bFuncR);
				bFunc1 = cuddT(bFuncR);
			}
		}
		else // bVars is higher in the variable order 
			bFunc0 = bFunc1 = bFunc;

		// cofactor the cube
		if ( LevelTop == LevelC )
		{
			if ( bVarsCofR != bVarsCof ) // bFunc is complemented 
			{
				bVarsCof0 = Cudd_Not( cuddE(bVarsCofR) );
				bVarsCof1 = Cudd_Not( cuddT(bVarsCofR) );
			}
			else
			{
				bVarsCof0 = cuddE(bVarsCofR);
				bVarsCof1 = cuddT(bVarsCofR);
			}
		}
		else // bVars is higher in the variable order 
			bVarsCof0 = bVarsCof1 = bVarsCof;

		// there are two cases: 
		// (1) the top variable belongs to the cofactoring variables
		// (2) the top variable does not belong to the cofactoring variables

		// (1) the top variable belongs to the cofactoring variables
		Res = 0;
		if ( LevelTop == LevelC )
		{
			if ( bVarsCof1 == b0 ) // this is a negative cofactor
			{
				if ( bFunc0 != b0 )
				Res = Extra_CountCofactorMinterms( dd, bFunc0, bVarsCof0, cuddT(bVarsAll) );
			}
			else                        // this is a positive cofactor
			{
				if ( bFunc1 != b0 )
				Res = Extra_CountCofactorMinterms( dd, bFunc1, bVarsCof1, cuddT(bVarsAll) );
			}
		}
		else
		{
			if ( bFunc0 != b0 )
			Res += Extra_CountCofactorMinterms( dd, bFunc0, bVarsCof0, cuddT(bVarsAll) );
			
			if ( bFunc1 != b0 )
			Res += Extra_CountCofactorMinterms( dd, bFunc1, bVarsCof1, cuddT(bVarsAll) );
		}

//		st_insert(Visited, (char*)bFunc, NULL);

//		HHTable1[HKey].Sign = s_Signature;
//		HHTable1[HKey].Arg1 = bFuncR;

		// skip through the entries with the same signatures 
		// (these might have been created at the time of recursive calls)
		for ( ; HHTable1[HKey].Sign == s_Signature; HKey = (HKey+1) % _TABLESIZE_COF );
		assert( HHTable1[HKey].Sign != s_Signature );
		HHTable1[HKey].Sign = s_Signature;
//		HHTable1[HKey].Arg1 = bFuncR;
		HHTable1[HKey].Arg1 = bFunc;

		return Res;
	}
}

/**Function********************************************************************

  Synopsis    [Counts the number of minterms.]

  Description [This function counts minterms for functions up to 32 variables
  using a local cache. The terminal value (s_Termina) should be adjusted for
  BDDs and ADDs.]

  SideEffects [None]

******************************************************************************/
unsigned Extra_CountMintermsSimple( DdNode * bFunc, unsigned max )
{
	unsigned HKey;

	// normalize
	if ( Cudd_IsComplement(bFunc) )
		return max - Extra_CountMintermsSimple( Cudd_Not(bFunc), max );

	// now it is known that the function is not complemented
	if ( cuddIsConstant(bFunc) )
		return ((bFunc==s_Terminal)? 0: max);

	// check cache
	HKey = hashKey2( bFunc, max, _TABLESIZE_MINT );
	if ( HHTable2[HKey].Arg1 == bFunc && HHTable2[HKey].Arg2 == max )
		return HHTable2[HKey].Res;
	else
	{
		// min = min0/2 + min1/2;
		unsigned min = (Extra_CountMintermsSimple( cuddE(bFunc), max ) >> 1) +
			           (Extra_CountMintermsSimple( cuddT(bFunc), max ) >> 1);

		HHTable2[HKey].Arg1 = bFunc;
		HHTable2[HKey].Arg2 = max;
		HHTable2[HKey].Res  = min;

		return min;
	}
}	/* end of Extra_CountMintermsSimple */
