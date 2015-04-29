/**CFile***********************************************************************

  FileName    [bCache.c]

  PackageName [extra]

  Synopsis    [Multi-argument cache and specialized traversal routines.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_CacheClear();
				<li> Extra_CacheAllocate();
				<li> Extra_CacheDelocate();
				<li> Extra_CheckColumnCompatibility();
				<li> Extra_CheckColumnCompatibilityGroup();
				<li> Extra_CheckRootFunctionIdentity();
				<li> Extra_CheckOrBiDecomposability();
				</ul>
			Internal procedures included in this module:
				<ul>
				<li> Extra_CheckRootFunctionIdentity_rec();
				<li> Extra_CheckOrBiDecomposability_rec();
				</ul>
			Static procedures included in this module:
				<ul>
				</ul>
	      ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$bCache.c, v.1.2, September 15, 2001, alanmi $]

******************************************************************************/

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

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

// the signature counter
unsigned g_CacheSignature;

// the table size
unsigned g_CacheTableSize;

// the table size is set to the smallest integer larger than nEntries 
// it is used in the hashing macros

typedef struct 
{
	DdNode * bX[5];  // if all five arguments are used, the result is stored in the second bit of bX[4]
} _HashEntry;
static _HashEntry * _HTable;
static int _HashSuccess = 0;
static int _HashFailure = 0;


/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

// complementation and testing for pointers for hash entries
#define Hash_IsComplement(p)  ((int)((long) (p) & 02))
#define Hash_Regular(p)       ((DdNode*)((unsigned)(p) & ~02)) 
#define Hash_Not(p)           ((DdNode*)((long)(p) ^ 02)) 
#define Hash_NotCond(p,c)     ((DdNode*)((long)(p) ^ (02*c)))

/**Automaticstart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static int Extra_CheckRootFunctionIdentity_rec( DdManager * dd, DdNode * bF1, DdNode * bF2, DdNode * bC1, DdNode * bC2 );
static int Extra_CheckOrBiDecomposability_rec( DdManager * dd, DdNode * bF1, DdNode * bF2, DdNode * bF3, DdNode * bC1, DdNode * bC2 );

/**Automaticend***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Cleas the local cache.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Extra_CacheClear()
{
	int i;
	for ( i = 0; i < (int)g_CacheTableSize; i++ )
		_HTable[i].bX[0] = 0;
	g_CacheSignature = 1;
}

/**Function********************************************************************

  Synopsis    [(Re)allocates the local cache.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Extra_CacheAllocate( int nEntries )
{
	unsigned Requested = Cudd_Prime( nEntries );

	// check what is the size of the current cache
	if ( g_CacheTableSize != Requested )
	{ // the current size is different
		
		// delocate the old, allocate the new
		if ( g_CacheTableSize )
			Extra_CacheDelocate();

		// allocate memory for the hash table
		g_CacheTableSize = Requested;
		_HTable = (_HashEntry*) malloc( g_CacheTableSize * sizeof(_HashEntry) );
	}
	
	// otherwise, there is no need to allocate, just clean
	Extra_CacheClear();

//	printf( "\nThe number of allocated cache entries = %d.\n\n", g_CacheTableSize );
}

/**Function********************************************************************

  Synopsis    [Delocates the local cache.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Extra_CacheDelocate()
{
	free(_HTable);
	g_CacheTableSize = 0;
}



/**Function********************************************************************

  Synopsis    [Checks compatiblity of two columns for incompletely specified functions.]

  Description [Columns are compatible if the overlap of on-set (bF1) for the first column (bC1) 
  and off-set (bF2) for the second column (bC2) is compatible is empty. Another condition is 
  that the vice versa overlap should also be empty. Therefore this function performs only one
  half of the total check: (bF1(bC1=1) & bF2(bC2=1) == 0). Important: This functions assumes 
  that bC1 and bC2 are mixed-polarity cubes developing on the same set of vars!]

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Extra_CheckColumnCompatibility( DdManager * dd, DdNode * bF1, DdNode * bF2, DdNode * bC1, DdNode * bC2 )
{
	unsigned HKey;
	// if either bC1 or bC2 is zero, the test is true
//	if ( bC1 == b0 || bC2 == b0 )  return 1;
	assert( bC1 != b0 );
	assert( bC2 != b0 );

	if ( bF1 == b0 || bF2 == b0 )  return 1;

//	if ( bF1 == b1 )
//		return GetPathValue( dd, bF2, bC2 );
//	if ( bF2 == b1 )
//		return GetPathValue( dd, bF1, bC1 );

	// if both bC1 and bC2 are one - perform comparison
	if ( bC1 == b1 && bC2 == b1 )  return (int)( Cudd_bddIteConstant(dd,bF1,bF2,b0) == b0 );
	// otherwise, keep expanding

	// check cache
	HKey = hashKey4( bF1, bF2, bC1, bC2, g_CacheTableSize );
	if ( _HTable[HKey].bX[0] == bF1 && 
		 _HTable[HKey].bX[1] == bF2 && 
		 _HTable[HKey].bX[2] == bC1 && 
		 Hash_Regular(_HTable[HKey].bX[3]) == bC2 )
	{
		_HashSuccess++;
		return (int)( bC2 != _HTable[HKey].bX[3] ); // the last bit records the result (yes/no)
	}

//PRB(bC1);
//PRB(bC2);
//fflush(stdout);

	{
		// determine the top variables
		int RetValue;
		DdNode * bA[4]  = { bF1, bF2, bC1, bC2 };                             // arguments
		DdNode * bAR[4] = { Cudd_Regular(bF1), Cudd_Regular(bF2), Cudd_Regular(bC1), Cudd_Regular(bC2) }; // regular arguments
		int CurLevel[4] = { cuddI(dd,bAR[0]->index), cuddI(dd,bAR[1]->index), dd->perm[bAR[2]->index], dd->perm[bAR[3]->index] };
		int TopLevel = CUDD_CONST_INDEX;
		int i;
		DdNode * bE[4], * bT[4];


		_HashFailure++;

		// make sure that the top level of the columns is the same
		assert( CurLevel[2] == CurLevel[3] );

		// determine the top level
		for ( i = 0; i < 4; i++ )
			if ( TopLevel > CurLevel[i] )
				 TopLevel = CurLevel[i];

		// compute the cofactors
		for ( i = 0; i < 4; i++ )
		if ( TopLevel == CurLevel[i] )
		{
			if ( bA[i] != bAR[i] ) // complemented
			{
				bE[i] = Cudd_Not(cuddE(bAR[i]));
				bT[i] = Cudd_Not(cuddT(bAR[i]));
			}
			else
			{
				bE[i] = cuddE(bAR[i]);
				bT[i] = cuddT(bAR[i]);
			}
		}
		else
			bE[i] = bT[i] = bA[i];

		// solve subproblems
		// two cases are possible
		// (1) bC1 and bC2 are on the top level (in this case, follow the cube)
		// (2) bC1 and bC2 are NOT on the top level (in this case, keep cofactoring)

		// (1) bC1 and bC2 are on the top level (in this case, follow the cube)
		if ( TopLevel == CurLevel[2] ) 
		{
			DdNode * bF1next, * bF2next, * bC1next, * bC2next;

			if ( bE[2] == b0 ) // var is positive in the cube bC1
			{
				bF1next = bT[0];
				bC1next = bT[2];
			}
			else // if ( bT[2] == b0 )
			{
				bF1next = bE[0];
				bC1next = bE[2];
			}

			if ( bE[3] == b0 ) // var is positive in the cube bC2
			{
				bF2next = bT[1];
				bC2next = bT[3];
			}
			else // if ( bT[3] == b0 )
			{
				bF2next = bE[1];
				bC2next = bE[3];
			}

			RetValue = Extra_CheckColumnCompatibility( dd, bF1next, bF2next, bC1next, bC2next );
		}
		// (2) bC1 and bC2 are NOT on the top level (in this case, keep cofactoring)
		else
		{
			// split around this variable
			RetValue = Extra_CheckColumnCompatibility( dd, bE[0], bE[1], bC1, bC2 );
			if ( RetValue == 1 ) // test another branch; otherwise, there is no need to test
				RetValue = Extra_CheckColumnCompatibility( dd, bT[0], bT[1], bC1, bC2 );
		}

		// set cache
		_HTable[HKey].bX[0] = bF1;
		_HTable[HKey].bX[1] = bF2;
		_HTable[HKey].bX[2] = bC1;
		_HTable[HKey].bX[3] = Hash_NotCond( bC2, RetValue );

		return RetValue;
	}
}


/**Function********************************************************************

  Synopsis    [Checks compatiblity of two groups of columns for incompletely specified functions.]

  Description [The groups are compatible if any column in bC1 is compatible with any column 
  in bC2 (two columns are compatible if the overlap of on-set and off-set is empty)
  In other words, this function tests whether it is true that (bF1(bC1=1) & bF2(bC2=1) == 0)
  Assumes that bC1 and bC2 are column sets dependent on variables bVars.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Extra_CheckColumnCompatibilityGroup( DdManager * dd, DdNode * bF1, DdNode * bF2, DdNode * bC1, DdNode * bC2, DdNode * bVars )
{
	unsigned HKey;

	// if either of the functions is zero, the test is true (there is no overlap)
	if ( bF1 == b0 || bF2 == b0 )  return 1;
	// if there are no columns to test, the test is true
	if ( bC1 == b0 || bC2 == b0 )  return 1;

	// if there are no more column variables left - test intersection
	if ( bVars == b1 )  return (int)( Cudd_bddIteConstant(dd,bF1,bF2,b0) == b0 );

	// if one of the components is constant 1, check the equality ( bF(bC=1) == 0 )
	// in other words: bC => Not(bF), in other words ITE( bC, Not(bF), 1 ) == 1
	if ( bF1 == b1 )
		return (int)( Cudd_bddIteConstant(dd,bC2,Cudd_Not(bF2),b0) == b1 );
	if ( bF2 == b1 )
		return (int)( Cudd_bddIteConstant(dd,bC1,Cudd_Not(bF1),b0) == b1 );

	// normalize the arguments
	if ( (unsigned)bF1 < (unsigned)bF2 )
		return Extra_CheckColumnCompatibilityGroup( dd, bF2, bF1, bC2, bC1, bVars );

	// check cache
	HKey = hashKey5( bF1, bF2, bC1, bC2, bVars, g_CacheTableSize );
	if ( _HTable[HKey].bX[0] == bF1 && 
		 _HTable[HKey].bX[1] == bF2 && 
		 _HTable[HKey].bX[2] == bC1 && 
		 _HTable[HKey].bX[3] == bC2 && 
		 Hash_Regular(_HTable[HKey].bX[4]) == bVars )
	{
		_HashSuccess++;
		return (int)( bVars != _HTable[HKey].bX[4] ); // the last bit records the result (yes/no)
	}
	// otherwise, keep expanding
	else
	{
		// determine the top variables
		int RetValue;
		DdNode * bA[4]  = { bF1, bF2, bC1, bC2 };            // arguments
		DdNode * bAR[4] = { Cudd_Regular(bF1), Cudd_Regular(bF2), Cudd_Regular(bC1), Cudd_Regular(bC2) }; // regular arguments
		int CurLevel[5] = { cuddI(dd,bAR[0]->index), cuddI(dd,bAR[1]->index), cuddI(dd,bAR[2]->index), cuddI(dd,bAR[3]->index), dd->perm[bVars->index] };
		int TopLevel = CUDD_CONST_INDEX;
		int i;
		DdNode * bE[4], * bT[4];

		_HashFailure++;

		// make sure that the top level of the columns is the same
//		assert( CurLevel[2] == CurLevel[3] );

		// determine the top level
		for ( i = 0; i < 5; i++ )
			if ( TopLevel > CurLevel[i] )
				 TopLevel = CurLevel[i];

		// the topmost variable in column encoding functions should be present in bVars
		if ( TopLevel == CurLevel[2] || TopLevel == CurLevel[3] )
		{
			assert( TopLevel == CurLevel[4] );
		}

		// compute the cofactors
		for ( i = 0; i < 4; i++ )
		if ( TopLevel == CurLevel[i] )
		{
			if ( bA[i] != bAR[i] ) // complemented
			{
				bE[i] = Cudd_Not(cuddE(bAR[i]));
				bT[i] = Cudd_Not(cuddT(bAR[i]));
			}
			else
			{
				bE[i] = cuddE(bAR[i]);
				bT[i] = cuddT(bAR[i]);
			}
		}
		else
			bE[i] = bT[i] = bA[i];


		// two cases are possible
		// (1) the topmost variable belongs to bVars (in this case, require the compatibility in all branches)
		// (2) the topmost variable does NOT belong to bVars (in this case, keep cofactoring)

		// (1) the topmost variable belongs to bVars
		if ( TopLevel == CurLevel[4] ) 
		{
			// each column in bC1 should be compatible with each column in bC2
			// make sure that this is true for 
			RetValue = Extra_CheckColumnCompatibilityGroup( dd, bE[0], bE[1], bE[2], bE[3], cuddT(bVars) );
			if ( RetValue ) 
			{
				RetValue = Extra_CheckColumnCompatibilityGroup( dd, bE[0], bT[1], bE[2], bT[3], cuddT(bVars) );
				if ( RetValue ) 
				{
					RetValue = Extra_CheckColumnCompatibilityGroup( dd, bT[0], bE[1], bT[2], bE[3], cuddT(bVars) );
					if ( RetValue ) 
						RetValue = Extra_CheckColumnCompatibilityGroup( dd, bT[0], bT[1], bT[2], bT[3], cuddT(bVars) );
				}
			}
		}
		// (2) the topmost variable does NOT belong to bVars
		else
		{
			// split around this variable
			RetValue = Extra_CheckColumnCompatibilityGroup( dd, bE[0], bE[1], bC1, bC2, bVars );
			if ( RetValue == 1 ) // test another branch; otherwise, there is no need to test
				RetValue = Extra_CheckColumnCompatibilityGroup( dd, bT[0], bT[1], bC1, bC2, bVars );
		}

		// set cache
		_HTable[HKey].bX[0] = bF1;
		_HTable[HKey].bX[1] = bF2;
		_HTable[HKey].bX[2] = bC1;
		_HTable[HKey].bX[3] = bC2;
		_HTable[HKey].bX[4] = Hash_NotCond( bVars, RetValue );

		return RetValue;
	}
}


/**Function********************************************************************

  Synopsis    [Checks whether it is true that bF1(bC1=0) == bF2(bC2=0).]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Extra_CheckRootFunctionIdentity( DdManager * dd, DdNode * bF1, DdNode * bF2, DdNode * bC1, DdNode * bC2 )
{
	int RetValue;

	_HashSuccess = 0;
	_HashFailure = 0;
	RetValue = Extra_CheckRootFunctionIdentity_rec(dd, bF1, bF2, bC1, bC2);
//	printf( "Cache success = %d. Cache failure = %d.\n", _HashSuccess, _HashFailure );
//	printf( ".");

	return RetValue;
}


/**Function********************************************************************

  Synopsis    [Returns 1 if it is true that ( bF1 & Exist(bF2,bC1) & Exist(bF3,bC2) == 0 ).]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Extra_CheckOrBiDecomposability( DdManager * dd, DdNode * bF1, DdNode * bF2, DdNode * bC1, DdNode * bC2 )
{
	int RetValue;

	_HashSuccess = 0;
	_HashFailure = 0;
	RetValue = Extra_CheckOrBiDecomposability_rec(dd, bF1, bF2, bF2, bC1, bC2);
//	printf( "Cache success = %d. Cache failure = %d.\n", _HashSuccess, _HashFailure );
//	printf( ".");

	return RetValue;
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Performs the recursive step of Extra_CheckRootFunctionIdentity().]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Extra_CheckRootFunctionIdentity_rec( DdManager * dd, DdNode * bF1, DdNode * bF2, DdNode * bC1, DdNode * bC2 )
{
	unsigned HKey;

	// if either bC1 or bC2 is zero, the test is true
//	if ( bC1 == b0 || bC2 == b0 )  return 1;
	assert( bC1 != b0 );
	assert( bC2 != b0 );

	// if both bC1 and bC2 are one - perform comparison
	if ( bC1 == b1 && bC2 == b1 )  return (int)( bF1 == bF2 );

	if ( bF1 == b0 )
		return Cudd_bddLeq( dd, bC2, Cudd_Not(bF2) );

	if ( bF1 == b1 )
		return Cudd_bddLeq( dd, bC2, bF2 );

	if ( bF2 == b0 )
		return Cudd_bddLeq( dd, bC1, Cudd_Not(bF1) );

	if ( bF2 == b1 )
		return Cudd_bddLeq( dd, bC1, bF1 );

	// otherwise, keep expanding

	// check cache
//	HKey = _Hash( ((unsigned)bF1), ((unsigned)bF2), ((unsigned)bC1), ((unsigned)bC2) );
	HKey = hashKey4( bF1, bF2, bC1, bC2, g_CacheTableSize );

	if ( _HTable[HKey].bX[0] == bF1 && 
		 _HTable[HKey].bX[1] == bF2 && 
		 _HTable[HKey].bX[2] == bC1 && 
		 _HTable[HKey].bX[3] == bC2 )
	{
		_HashSuccess++;
		return (int)_HTable[HKey].bX[4]; // the last bit records the result (yes/no)
	}
	else
	{

		// determine the top variables
		int RetValue;
		DdNode * bA[4]  = { bF1, bF2, bC1, bC2 }; // arguments
		DdNode * bAR[4] = { Cudd_Regular(bF1), Cudd_Regular(bF2), Cudd_Regular(bC1), Cudd_Regular(bC2) }; // regular arguments
		int CurLevel[4] = { cuddI(dd,bAR[0]->index), cuddI(dd,bAR[1]->index), cuddI(dd,bAR[2]->index), cuddI(dd,bAR[3]->index) };
		int TopLevel = CUDD_CONST_INDEX;
		int i;
		DdNode * bE[4], * bT[4];
		DdNode * bF1next, * bF2next, * bC1next, * bC2next;

		_HashFailure++;

		// determine the top level
		for ( i = 0; i < 4; i++ )
			if ( TopLevel > CurLevel[i] )
				 TopLevel = CurLevel[i];

		// compute the cofactors
		for ( i = 0; i < 4; i++ )
		if ( TopLevel == CurLevel[i] )
		{
			if ( bA[i] != bAR[i] ) // complemented
			{
				bE[i] = Cudd_Not(cuddE(bAR[i]));
				bT[i] = Cudd_Not(cuddT(bAR[i]));
			}
			else
			{
				bE[i] = cuddE(bAR[i]);
				bT[i] = cuddT(bAR[i]);
			}
		}
		else
			bE[i] = bT[i] = bA[i];

		// solve subproblems
		// three cases are possible

		// (1) the top var belongs to both C1 and C2
		//     in this case, any cofactor of F1 and F2 will do, 
		//     as long as the corresponding cofactor of C1 and C2 is not equal to 0
		if ( TopLevel == CurLevel[2] && TopLevel == CurLevel[3] ) 
		{
			if ( bE[2] != b0 ) // C1
			{
				bF1next = bE[0];
				bC1next = bE[2];
			}
			else
			{
				bF1next = bT[0];
				bC1next = bT[2];
			}
			if ( bE[3] != b0 ) // C2
			{
				bF2next = bE[1];
				bC2next = bE[3];
			}
			else
			{
				bF2next = bT[1];
				bC2next = bT[3];
			}
			RetValue = Extra_CheckRootFunctionIdentity_rec( dd, bF1next, bF2next, bC1next, bC2next );
		}
		// (2) the top var belongs to either C1 or C2
		//     in this case normal splitting of cofactors
		else if ( TopLevel == CurLevel[2] && TopLevel != CurLevel[3] ) 
		{
			if ( bE[2] != b0 ) // C1
			{
				bF1next = bE[0];
				bC1next = bE[2];
			}
			else
			{
				bF1next = bT[0];
				bC1next = bT[2];
			}
			// split around this variable
			RetValue = Extra_CheckRootFunctionIdentity_rec( dd, bF1next, bE[1], bC1next, bE[3] );
			if ( RetValue == 1 ) // test another branch; otherwise, there is no need to test
				RetValue = Extra_CheckRootFunctionIdentity_rec( dd, bF1next, bT[1], bC1next, bT[3] );
		}
		else if ( TopLevel != CurLevel[2] && TopLevel == CurLevel[3] ) 
		{
			if ( bE[3] != b0 ) // C2
			{
				bF2next = bE[1];
				bC2next = bE[3];
			}
			else
			{
				bF2next = bT[1];
				bC2next = bT[3];
			}
			// split around this variable
			RetValue = Extra_CheckRootFunctionIdentity_rec( dd, bE[0], bF2next, bE[2], bC2next );
			if ( RetValue == 1 ) // test another branch; otherwise, there is no need to test
				RetValue = Extra_CheckRootFunctionIdentity_rec( dd, bT[0], bF2next, bT[2], bC2next );
		}
		// (3) the top var does not belong to C1 and C2
		//     in this case normal splitting of cofactors
		else // if ( TopLevel != CurLevel[2] && TopLevel != CurLevel[3] )
		{
			// split around this variable
			RetValue = Extra_CheckRootFunctionIdentity_rec( dd, bE[0], bE[1], bE[2], bE[3] );
			if ( RetValue == 1 ) // test another branch; otherwise, there is no need to test
				RetValue = Extra_CheckRootFunctionIdentity_rec( dd, bT[0], bT[1], bT[2], bT[3] );
		}

		// set cache
		for ( i = 0; i < 4; i++ )
			_HTable[HKey].bX[i] = bA[i];
		_HTable[HKey].bX[4] = (DdNode*)RetValue;

		return RetValue;
	}
}


/**Function********************************************************************

  Synopsis    [Performs the recursive step of Extra_CheckOrBiDecomposability().]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Extra_CheckOrBiDecomposability_rec( DdManager * dd, DdNode * bF1, DdNode * bF2, DdNode * bF3, DdNode * bC1, DdNode * bC2 )
{
	unsigned HKey;

	// bC1 and bC2 originally are given as BDD cubes
	// we expand them "cautiously", by always taking the positive cofactor
	// in this case, bC1 and bC2 are either non-zero (pointing to the remaining part of the cube)
	// or a constant 1 (meaning that this is the end of the cube)
	assert( bC1 != b0 );
	assert( bC2 != b0 );

	if ( bF1 == b0 )
		return 1;

	if ( bF2 == b0 )
		return 1;

	if ( bF3 == b0 )
		return 1;


/*
	// now, both bF1 and bF2 are not a constant zero BDD
	// if bF1 == 1 and bF2 != b0, then always (Exist(bF2,bC1) & Exist(bF2,bC2) != 0)
	if ( bF1 == b1 )
		return 0;

	if ( bF2 == b1 )
		return 0;
*/
/*
	// if both bC1 and bC2 are one,there are no variables left in the set
	// return the condition (bF1 * bF2 == 0), or (bF1 => bF2' == 1)
	// if either bC1 and bC2 are one, the same is true...
	if ( bC1 == b1 || bC2 == b1 )  
		return Cudd_bddLeq( dd, bF1, Cudd_Not(bF2) );
*/
	// now, after introducing bF3, these terminal cases do not work :(



	if ( bF2 == b1 && bF3 == b1 )
		return 0;

	if ( bC1 == b1 && bF3 == b1 )
		return Cudd_bddLeq( dd, bF1, Cudd_Not(bF2) );

	if ( bC2 == b1 && bF2 == b1 )
		return Cudd_bddLeq( dd, bF1, Cudd_Not(bF3) );


/*
	// the problem is symmetric w.r.t. bC1 and bC2, 
	// it is possible to improve the cache hit-rate by ordering bC1 and bC2
	if ( bC1->index < bC2->index )
	{
		DdNode * bTemp;
		bTemp = bC1;
		bC1 = bC2;
		bC2 = bTemp;
	}
*/
	// now, after introducing bF3, the problem is not longer symmteric w.r.t. bC1 and bC2


	// otherwise, keep expanding

	// check cache
	HKey = hashKey5( bF1, bF2, bF3, bC1, bC2, g_CacheTableSize );

	if ( _HTable[HKey].bX[0] == bF1 && 
		 _HTable[HKey].bX[1] == bF2 && 
		 _HTable[HKey].bX[2] == bF3 && 
		 _HTable[HKey].bX[3] == bC1 && 
		 Hash_Regular(_HTable[HKey].bX[4]) == bC2 )
	{
		_HashSuccess++;
		return (int)( _HTable[HKey].bX[4] != bC2 ); // the last bit records the result (yes/no)
	}
	else
	{

		// determine the top variables
		// it is helpful to remember that 
		// (1) all variables (bF1, bF2, bC1, bC2) are non-constant BDDs
		// (2) the cubes (bC1 and bC2) are non-complemented BDDs

		// the above is no longer true :(

		int i, RetValue;
		DdNode * bA[5]  = { bF1, bF2, bF3, bC1, bC2 }; // arguments
		DdNode * bAR[5] = { Cudd_Regular(bF1), Cudd_Regular(bF2), Cudd_Regular(bF3), bC1, bC2 }; // regular arguments
		int CurLevel[5] = { cuddI(dd,bAR[0]->index), cuddI(dd,bAR[1]->index), cuddI(dd,bAR[2]->index), cuddI(dd,bAR[3]->index), cuddI(dd,bAR[4]->index) };
		int TopLevel = CUDD_CONST_INDEX;
		DdNode * bE[3], * bT[3];

		_HashFailure++;

		// make sure that cubes never depend on the same variable
		assert( bC1->index == CUDD_CONST_INDEX || bC1->index != bC2->index );

		// determine the top level
		for ( i = 0; i < 5; i++ )
			if ( TopLevel > CurLevel[i] )
				TopLevel = CurLevel[i];
		
		// compute the cofactors of the three functions
		for ( i = 0; i < 3; i++ )
		if ( TopLevel == CurLevel[i] )
		{
			if ( bA[i] != bAR[i] ) // complemented
			{
				bE[i] = Cudd_Not(cuddE(bAR[i]));
				bT[i] = Cudd_Not(cuddT(bAR[i]));
			}
			else
			{
				bE[i] = cuddE(bAR[i]);
				bT[i] = cuddT(bAR[i]);
			}
		}
		else
			bE[i] = bT[i] = bA[i];

		// solve subproblems
		// three cases are possible

		// (1) the top var belongs to neither C1 nor C2
		//     in this case, we just expand the problem recursively
		//     the Shannon expansion of Z = A * Ex(B) * Ey(C) w.r.t. w is
		//     w'*Zw' + w*Zw = (w'*Aw' + w*Aw) * Ex(w'*Bw'   + w*Bw) * Ey(w'*Cw' + w*Cw) =
		//                   = (w'*Aw' + w*Aw) * (w'*Ex(Bw') + w*Ex(Bw)) * (w'*Ey(Cw') + w*Ey(Cw)) =
		//                   =  w'*[Aw' * Ex(Bw') * Ey(Cw')] + w*[Aw * Ex(Bw) * Ey(Cw)]
		if ( TopLevel != CurLevel[3] && TopLevel != CurLevel[4] ) 
		{
			// split around this variable
			RetValue = Extra_CheckOrBiDecomposability_rec( dd, bE[0], bE[1], bE[2], bC1, bC2 );
			if ( RetValue == 1 ) // test another branch; otherwise, there is no need to test
				RetValue = Extra_CheckOrBiDecomposability_rec( dd, bT[0], bT[1], bT[2], bC1, bC2 );
		}
		// (2) the top var belongs to C1 and does not belong to C2
		//     in this case, four checks are necessary because
		//     the Shannon expansion of Z = A * Ex(B) * Ey(C) w.r.t. x is
		//     x'*Zx' + x*Zx = (x'*Ax' + x*Ax) * (Er(Bx') + Er(Bx)) * Ey(x'*Cx' + x*Cx) =
		//                   = (x'*Ax' + x*Ax) * (Er(Bx') + Er(Bx)) * [x'*Ey(Cx') + x*Ey(Cx)] =  
		//                   =  x'*[Ax' * Er(Bx') * Ey(Cx')]  +  x'*[Ax' * Er(Bx ) * Ey(Cx')] + 
		//                   +  x *[Ax  * Er(Bx') * Ey(Cx )]  +  x *[Ax  * Er(Bx ) * Ey(Cx) ]
		//     where r is the set of remaining variables after subtracting x
		else if ( TopLevel == CurLevel[3] && TopLevel != CurLevel[4] ) 
		{
			RetValue = Extra_CheckOrBiDecomposability_rec( dd, bE[0], bE[1], bE[2], cuddT(bC1), bC2 );
			if ( RetValue == 1 ) // test another branch; otherwise, there is no need to test
			{
				RetValue = Extra_CheckOrBiDecomposability_rec( dd, bE[0], bT[1], bE[2], cuddT(bC1), bC2 );
				if ( RetValue == 1 ) // test another branch; otherwise, there is no need to test
				{
					RetValue = Extra_CheckOrBiDecomposability_rec( dd, bT[0], bE[1], bT[2], cuddT(bC1), bC2 );
					if ( RetValue == 1 ) // test another branch; otherwise, there is no need to test
						RetValue = Extra_CheckOrBiDecomposability_rec( dd, bT[0], bT[1], bT[2], cuddT(bC1), bC2 );
				}
			}
		}
		// (3) the top var belongs to C2 and does not belong to C1
		//     in this case, four checks are necessary, similarly to the above case
		else if ( TopLevel != CurLevel[3] && TopLevel == CurLevel[4] ) 
		{
			RetValue = Extra_CheckOrBiDecomposability_rec( dd, bE[0], bE[1], bE[2], bC1, cuddT(bC2) );
			if ( RetValue == 1 ) // test another branch; otherwise, there is no need to test
			{
				RetValue = Extra_CheckOrBiDecomposability_rec( dd, bE[0], bE[1], bT[2], bC1, cuddT(bC2) );
				if ( RetValue == 1 ) // test another branch; otherwise, there is no need to test
				{
					RetValue = Extra_CheckOrBiDecomposability_rec( dd, bT[0], bT[1], bE[2], bC1, cuddT(bC2) );
					if ( RetValue == 1 ) // test another branch; otherwise, there is no need to test
						RetValue = Extra_CheckOrBiDecomposability_rec( dd, bT[0], bT[1], bT[2], bC1, cuddT(bC2) );
				}
			}
		}
		else
		{
			assert( 0 );
		}

		// set cache
		for ( i = 0; i < 4; i++ )
			_HTable[HKey].bX[i] = bA[i];
		_HTable[HKey].bX[4] = Hash_NotCond( bC2, RetValue );

		return RetValue;
	}
}

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/
