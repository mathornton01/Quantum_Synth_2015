/**CFile***********************************************************************

  FileName    [bNPN.c]

  PackageName [extra]

  Synopsis    [Various procedures to test the N, NP, and NPN equivalence.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_bddNCanonicalForm();
				<li> Extra_bddCubeExtractPositiveVars();
				<li> Extra_bddCubeConvertIntoPositiveVars();
				<li> Extra_bddCompareTruthVectors();
				<li> extraCubeScroll();
				</ul>
			Internal procedures included in this module:
				<ul>
				<li> extraBddNCanonicalForm();
				<li> extraBddCubeExtractPositiveVars();
				<li> extraBddCubeConvertIntoPositiveVars();
				</ul>
			Static procedures included in this module:
				<ul>
				</ul>
	      ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$bNPN.c, v.1.2, November 18, 2001, alanmi $]

******************************************************************************/

#include "util.h"
#include "extra.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

// cache tokens
DdNode * CacheOp1( DdManager * dd, DdNode * A1, DdNode * A2 ) { return (DdNode*)0; }
DdNode * CacheOp2( DdManager * dd, DdNode * A1, DdNode * A2 ) { return (DdNode*)1; }
// It was very weird when Microsoft C++ 6.0 Compiler has optimized these two 
// procedures in the release version of the library into one...
// After discovering the reason, I made them return different values (0 and 1).

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**Automaticstart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Computes the N-canonical form.]

  Description [For the given function (bF) computes the unique representative
  of the class of functions derived from bF by all possible input negations. 
  Returns both the canonical form and the polatiry vector, which can be used 
  by Extra_bddChangePolarity() to derive the canonical form from the original 
  function. The canonical form is defined as such a representative among 2^n 
  different phase assignments of bF, which has the smallest integer value of 
  the binary truth vector. The variables lower in the BDD variables order are 
  treated as less significant in creating the truth vector. 
  
  For example, for the function F = a'+b belongs to the N-equivalence class
  which has four representatives:
  F00 = a + b   TV00 = 0111.
  F01 = a + b'  TV01 = 1011.
  F10 = a'+ b   TV10 = 1101.
  F11 = a'+ b'  TV11 = 1110.
  The canonical form is F00 because the truth vector TV00 is the smallest.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_bddNCanonicalForm( 
  DdManager * dd,   /* the DD manager */
  DdNode *  bF,     /* the function to be canonicized */
  DdNode ** bP)     /* the polarity which does the job */
{
    DdNode	* bRes, * bPol, * bCube;

    do {
		dd->reordered = 0;
		bRes = extraBddNCanonicalForm( dd, bF, b1, &bPol );
		// we call this function with b1 because at the beginning
		// the polarity of all variables is not known
    } while (dd->reordered == 1);

	Cudd_Ref( bRes );
	Cudd_Ref( bPol );

	// return the polarity if the caller wants it
	if ( bP != NULL )
	{
		// convert from the polarity in [01-] encoding to the polarity in [1-] encoding
		// in [01-]
		// 0 means no need to change polarity
		// 1 means change polarity
		// - means polarity is to be determined
		// in [1-]
		// 1 means change polarity

		// (perhaps the function ChangePolarity should be modified to ignore neg pol vars!!!)

		bCube = Extra_bddCubeExtractPositiveVars( dd, bPol );  Cudd_Ref( bCube );
		Cudd_RecursiveDeref( dd, bPol );

		Cudd_Deref( bRes );
		Cudd_Deref( bCube );

		*bP = bCube;
	}
	else
	{
		Cudd_RecursiveDeref( dd, bPol );
		Cudd_Deref( bRes );
	}

    return bRes;

} /* end of Extra_bddNCanonicalForm */


/**Function********************************************************************

  Synopsis    [Extract positive polarity vars from the mixed polarity cube.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_bddCubeExtractPositiveVars( 
  DdManager * dd,       /* the DD manager */
  DdNode    * bCube)    /* the cube to be converted */
{
    DdNode * res;
    do {
		dd->reordered = 0;
		res = extraBddCubeExtractPositiveVars( dd, bCube );
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_bddCubeExtractPositiveVars */


/**Function********************************************************************

  Synopsis    [Converts a mixed polarity cube into a positive polarity one.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_bddCubeConvertIntoPositiveVars( 
  DdManager * dd,       /* the DD manager */
  DdNode    * bCube)    /* the cube to be converted */
{
    DdNode * res;
    do {
		dd->reordered = 0;
		res = extraBddCubeConvertIntoPositiveVars( dd, bCube );
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_bddCubeConvertIntoPositiveVars */


/**Function********************************************************************

  Synopsis    [Compares the binary representation of truth vectors of two functions.]

  Description [Returns 1 if (mint(bF) < mint(bG)); -1 if otherwise; 0, if equal.]

  SideEffects []

  SeeAlso     []

******************************************************************************/

int Extra_bddCompareTruthVectors( DdManager * dd, DdNode * bF, DdNode * bG )
// returns 1 if (mint(bF) < mint(bG)); -1 if otherwise; 0, if equal
{
	DdNode * bRes;

	if ( bF == bG )
		return 0;
	if ( bF == b0 || bG == b1 )
		return 1;
	if ( bF == b1 || bG == b0 )
		return -1;

    if ( bRes = cuddCacheLookup2( dd, (DdNode*(*)(DdManager*,DdNode*,DdNode*))Extra_bddCompareTruthVectors, bF, bG) )
	{	// encoding: b1 = 1, b0 = -1, z0 = 0
		if      ( bRes == b1 )   return  1;
		else if ( bRes == b0 )   return -1;
		else if ( bRes == z0 )   return  0;
		assert( 0 );
		return 0;
	}
	else
	{
		int RetValue;
		DdNode * bF0, * bF1;             
		DdNode * bG0, * bG1;            
		DdNode * bFR = Cudd_Regular(bF); 
		DdNode * bGR = Cudd_Regular(bG); 
		int LevelF   = dd->perm[bFR->index];
		int LevelG   = dd->perm[bGR->index];
		int LevelTop;

		if ( LevelF < LevelG )
			LevelTop = LevelF;
		else
			LevelTop = LevelG;


		// cofactor the functions
		if ( LevelTop == LevelF )
		{
			if ( bFR != bF ) // bF is complemented 
			{
				bF0 = Cudd_Not( cuddE(bFR) );
				bF1 = Cudd_Not( cuddT(bFR) );
			}
			else
			{
				bF0 = cuddE(bFR);
				bF1 = cuddT(bFR);
			}
		}
		else 
			bF0 = bF1 = bF;

		if ( LevelTop == LevelG )
		{
			if ( bGR != bG ) // bG is complemented 
			{
				bG0 = Cudd_Not( cuddE(bGR) );
				bG1 = Cudd_Not( cuddT(bGR) );
			}
			else
			{
				bG0 = cuddE(bGR);
				bG1 = cuddT(bGR);
			}
		}
		else 
			bG0 = bG1 = bG;

		RetValue = Extra_bddCompareTruthVectors( dd, bF0, bG0 );
		if ( RetValue == 0 ) // the negative cofactors are equal
			RetValue = Extra_bddCompareTruthVectors( dd, bF1, bG1 );
		
		// insert the result into cache
		// encoding: b1 = 1, b0 = -1, z0 = 0
		if      ( RetValue ==  1 )    bRes = b1;
		else if ( RetValue == -1 )    bRes = b0;
		else if ( RetValue ==  0 )    bRes = z0;
		else { assert( 0 ); }

		cuddCacheInsert2(dd, (DdNode*(*)(DdManager*,DdNode*,DdNode*))Extra_bddCompareTruthVectors, bF, bG, bRes);
		return RetValue;
	}
} /* end of Extra_bddCompareTruthVectors */


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Performs the recursive step of Extra_bddNCanonicalForm().]

  Description [The semantics of bVars is the following: A var in negative polarity
  means that it should be left in the function without changing polarity. A var in
  positive polarity means that this var should be complemented. A var missing in 
  the cube means that this var's polarity should be determined.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * extraBddNCanonicalForm( 
  DdManager * dd,    /* the DD manager */
  DdNode *  bF,      /* the function to be canonicized */
  DdNode *  bVars,   /* the variables whose polarity is to be determined */
  DdNode ** bP)      /* the polarity which does the job */
{
	DdNode * bRes;
	DdNode * bPol;
	DdNode * bFR = Cudd_Regular(bF); 

	// if the function is a constant, the canonical form is the function itself
	if ( cuddIsConstant(bFR) )
	{
		*bP = bVars;
		return bF;
	}
	// it is possible to add one more terminal case:
	// if the function does not have any support variables that are not in bVars
	// it is possible to perform the specified polarity shift right away
	// by the call to a modified Extra_bddChangePolarity()

    if ( bRes = cuddCacheLookup2(dd, CacheOp1, bF, bVars) )
	{
		cuddRef( bRes );
		if ( bPol = cuddCacheLookup2(dd, CacheOp2, bF, bVars) )
		{
			*bP = bPol;
			cuddDeref( bRes );
			return bRes;
		}
		else
		{   // if the second cache lookup was not successful
			// we should derefence the first reclaimed result
			Cudd_RecursiveDeref( dd, bRes );
		}
	}

	{
		DdNode * bF0, * bF1;             // cofactors of the function
		DdNode * bRes0, * bRes1;         // the resulting canonical forms (CF) for cofactors
		DdNode * bPol0, * bPol1;         // the polarity saying which vars should be complemented to get CF
		DdNode * bVarsCut, * bVarsCutR;  // bVars that is cut on the level LevelF
		DdNode * bVarsNew;               // bVars that should be given to the recursive calls
		DdNode * bTemp;
		int LevelF, LevelV;
		int TopVarPol;
		int Compare;

		// cofactor the function
		if ( bFR != bF ) // bF is complemented 
		{
			bF0 = Cudd_Not( cuddE(bFR) );
			bF1 = Cudd_Not( cuddT(bFR) );
		}
		else
		{
			bF0 = cuddE(bFR);
			bF1 = cuddT(bFR);
		}

		// get the level of the function
		LevelF = dd->perm[bFR->index];

		// scroll through bVars as long as they are above LevelF
		bVarsCut  = extraCubeScroll( dd, bVars, LevelF );

		// get the level of the scrolled vars
		bVarsCutR = Cudd_Regular( bVarsCut );
		LevelV    = cuddI(dd, bVarsCutR->index);

		// using the relative position of bVarsCut w.r.t. bF
		// determine bVarsNew and the top var polarity in bVarsCut
		assert( LevelF <= LevelV );
		if ( LevelF == LevelV )
		{
			DdNode * bV0, * bV1;
			if ( bVarsCutR != bVarsCut ) // bVarsCut is complemented 
			{
				bV0 = Cudd_Not( cuddE(bVarsCutR) );
				bV1 = Cudd_Not( cuddT(bVarsCutR) );
			}
			else
			{
				bV0 = cuddE(bVarsCutR);
				bV1 = cuddT(bVarsCutR);
			}

			// assign bVarsNew and the top var polarity in bVarsCut
			if ( bV1 == b0 )
			{
				 TopVarPol = 0;
				 bVarsNew  = bV0;
			}
			else if ( bV0 == b0 )
			{
				 TopVarPol = 1;
				 bVarsNew  = bV1;
			}
			else
			{  // this is not a cube
				assert(0);
			}
		}
		else if ( LevelF < LevelV )
		{
		    TopVarPol = 2;
			bVarsNew  = bVarsCut;
		}

		// two cases are possible
		// (1) the topmost var in bF is not in bVars
		//     in this case, the polarity should be determined
		// (2) the topmost var in bF is in bVars
		//     in this case, the polarity is already known
		//     it should be set properly (using complementation if necessary)

		// (1) the topmost var in bF is not in bVars
		if ( TopVarPol == 2 )
		{
			// solve the problem for cofactors
			bRes0 = extraBddNCanonicalForm( dd, bF0, bVarsNew, &bPol0 );
			if ( bRes0 == NULL )
			{
				*bP = NULL;
				return NULL;
			}
			cuddRef( bRes0 );
			cuddRef( bPol0 );

			bRes1 = extraBddNCanonicalForm( dd, bF1, bVarsNew, &bPol1 );
			if ( bRes1 == NULL )
			{
				Cudd_RecursiveDeref( dd, bRes0 );
				Cudd_RecursiveDeref( dd, bPol0 );
				*bP = NULL;
				return NULL;
			}
			cuddRef( bRes1 );
			cuddRef( bPol1 );

			// compare the solution to decide which polarity should be adopted
			Compare = Extra_bddCompareTruthVectors( dd, bRes0, bRes1 );
			if ( Compare == 0 || Compare == 1 )
			{ // Mint(bRes0) <= Mint(bRes1)
				// take bRes0 as the left cofactor and tune bRes1 to have the same polarity
				Cudd_RecursiveDeref( dd, bRes1 );
				Cudd_RecursiveDeref( dd, bPol1 );

				bRes1 = extraBddNCanonicalForm( dd, bF1, bPol0, &bPol1 );
				if ( bRes1 == NULL )
				{
					Cudd_RecursiveDeref( dd, bRes0 );
					Cudd_RecursiveDeref( dd, bPol0 );
					*bP = NULL;
					return NULL;
				}
				cuddRef( bRes1 );
				cuddRef( bPol1 );
				Cudd_RecursiveDeref( dd, bPol0 );

				bPol = bPol1; // takes reference
			}
			else if ( Compare == -1 )
			{ // Mint(bRes0) > Mint(bRes1)
				// take bRes1 as the left cofactor and tune bRes0 to the same polarity
				Cudd_RecursiveDeref( dd, bRes0 );
				Cudd_RecursiveDeref( dd, bPol0 );

				bRes0 = extraBddNCanonicalForm( dd, bF0, bPol1, &bPol0 );
				if ( bRes0 == NULL )
				{
					Cudd_RecursiveDeref( dd, bRes1 );
					Cudd_RecursiveDeref( dd, bPol1 );
					*bP = NULL;
					return NULL;
				}
				cuddRef( bRes0 );
				cuddRef( bPol0 );
				Cudd_RecursiveDeref( dd, bPol1 );

				bPol = bPol0; // takes reference

				// swap the cofactors
				bTemp = bRes0;
				bRes0 = bRes1;
				bRes1 = bTemp;
			}
			else
			{
				assert( 0 );
			}
			// only bRes0, bRes1 and bPol are referenced as this point

			// update the resulting polarity
			// add the given var in the positive(negative) polarity
			bPol = cuddBddAndRecur( dd, bTemp = bPol, Cudd_NotCond( dd->vars[bFR->index], Compare!=-1 ) );
			if ( bPol == NULL )
			{
				Cudd_RecursiveDeref( dd, bRes0 );
				Cudd_RecursiveDeref( dd, bRes1 );
				Cudd_RecursiveDeref( dd, bTemp );
				*bP = NULL;
				return NULL;
			}
			cuddRef( bPol );
			Cudd_RecursiveDeref( dd, bTemp );
		}
		// (2) the topmost var in bF is in bVars
		else // if ( TopVarPol == 0 || TopVarPol == 1 )
		{
			if ( TopVarPol == 0 )
			{ // take bRes0 as the left cofactor and tune bRes1 to the same polarity
				bRes0 = extraBddNCanonicalForm( dd, bF0, bVarsNew, &bPol0 );
				if ( bRes0 == NULL )
				{
					*bP = NULL;
					return NULL;
				}
				cuddRef( bRes0 );
				cuddRef( bPol0 );

				bRes1 = extraBddNCanonicalForm( dd, bF1, bPol0, &bPol1 );
				if ( bRes1 == NULL )
				{
					Cudd_RecursiveDeref( dd, bRes0 );
					Cudd_RecursiveDeref( dd, bPol0 );
					*bP = NULL;
					return NULL;
				}
				cuddRef( bRes1 );
				cuddRef( bPol1 );
				Cudd_RecursiveDeref( dd, bPol0 );

				bPol = bPol1; // takes reference
			}
			else if ( TopVarPol == 1 )
			{ // take bRes1 as the left cofactor and tune bRes0 to the same polarity
				bRes1 = extraBddNCanonicalForm( dd, bF1, bVarsNew, &bPol1 );
				if ( bRes1 == NULL )
				{
					*bP = NULL;
					return NULL;
				}
				cuddRef( bRes1 );
				cuddRef( bPol1 );

				bRes0 = extraBddNCanonicalForm( dd, bF0, bPol1, &bPol0 );
				if ( bRes0 == NULL )
				{
					Cudd_RecursiveDeref( dd, bRes1 );
					Cudd_RecursiveDeref( dd, bPol1 );
					*bP = NULL;
					return NULL;
				}
				cuddRef( bRes0 );
				cuddRef( bPol0 );
				Cudd_RecursiveDeref( dd, bPol1 );

				bPol = bPol0; // takes reference

				// swap the cofactors
				bTemp = bRes0;
				bRes0 = bRes1;
				bRes1 = bTemp;
			}
			else
			{
				assert( 0 );
			}

			// update the resulting polarity
			// add the given var in the positive(negative) polarity
			bPol = cuddBddAndRecur( dd, bTemp = bPol, Cudd_NotCond( dd->vars[bFR->index], TopVarPol!=1 ) );
			if ( bPol == NULL )
			{
				Cudd_RecursiveDeref( dd, bRes0 );
				Cudd_RecursiveDeref( dd, bRes1 );
				Cudd_RecursiveDeref( dd, bTemp );
				*bP = NULL;
				return NULL;
			}
			cuddRef( bPol );
			Cudd_RecursiveDeref( dd, bTemp );
		}

		// compose the result
		assert( bRes0 != bRes1 );
		if ( Cudd_IsComplement(bRes1) ) 
		{
			bRes = cuddUniqueInter(dd, bFR->index, Cudd_Not(bRes1), Cudd_Not(bRes0));
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes0);
				Cudd_RecursiveDeref(dd,bRes1);
				Cudd_RecursiveDeref( dd, bPol );
				*bP = NULL;
				return NULL;
			}
			bRes = Cudd_Not(bRes);
		} 
		else 
		{
			bRes = cuddUniqueInter( dd, bFR->index, bRes1, bRes0 );
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes0);
				Cudd_RecursiveDeref(dd,bRes1);
				Cudd_RecursiveDeref( dd, bPol );
				*bP = NULL;
				return NULL;
			}
		}
		cuddRef( bRes );
		cuddDeref( bRes0 );
		cuddDeref( bRes1 );			

		// only bRes and bPol are referenced as this point

		if ( bVars != bVarsCut )
		{ // add the variables above LevelF to the resulting Polarity
			DdNode * bVarsCutPos;
			DdNode * bVarsToBeAdded;

			bVarsCutPos = extraBddCubeConvertIntoPositiveVars( dd, bVarsCut );
			if ( bVarsCutPos == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes);
				Cudd_RecursiveDeref(dd,bPol);
				*bP = NULL;
				return NULL;
			}
			cuddRef( bVarsCutPos );

			// abstract these variables from the original set
			bVarsToBeAdded = cuddBddExistAbstractRecur( dd, bVars, bVarsCutPos );
			if ( bVarsToBeAdded == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes);
				Cudd_RecursiveDeref(dd,bPol);
				Cudd_RecursiveDeref(dd,bVarsCutPos);
				*bP = NULL;
				return NULL;
			}
			cuddRef( bVarsToBeAdded );
			Cudd_RecursiveDeref(dd,bVarsCutPos);

			// add these vars to the polarity
			bPol = cuddBddAndRecur( dd, bTemp = bPol, bVarsToBeAdded );
			if ( bPol == NULL )
			{
				Cudd_RecursiveDeref(dd,bRes);
				Cudd_RecursiveDeref(dd,bTemp);
				Cudd_RecursiveDeref(dd,bVarsToBeAdded);
				*bP = NULL;
				return NULL;
			}
			cuddRef( bPol );
			Cudd_RecursiveDeref( dd, bTemp );
			Cudd_RecursiveDeref( dd, bVarsToBeAdded );
		}

		cuddDeref( bPol );
		cuddDeref( bRes );
	
		// insert the result into cache 
		cuddCacheInsert2(dd, CacheOp1, bF, bVars, bRes);
		cuddCacheInsert2(dd, CacheOp2, bF, bVars, bPol);

		*bP = bPol;
		return bRes;
	}
} /* end of extraBddNCanonicalForm */


/**Function********************************************************************

  Synopsis    [Scrolls through the cube.]

  Description [Takes a cube with variables in mixed polarities and a BDD level 
  above which the cube should be scrolled. Returns that part of the cube that 
  starts on the given level, or lower. In a particular case, if the cube's 
  topmost variable is already not above the given level, returns the cube 
  as it is. Does not build new BDD nodes.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * extraCubeScroll( 
  DdManager * dd,     /* the DD manager */
  DdNode *  bCube,    /* the cube to scrolled */
  int Level)          /* level above which to scroll */
{
	DdNode * bC0, * bC1;
	DdNode * bCubeR;
	int LevelC;

	while ( 1 )
	{
		// get the regular cube
		bCubeR = Cudd_Regular(bCube);
		// get the cube's level
		LevelC = cuddI(dd,bCubeR->index);

		if ( Level <= LevelC )
			return bCube;
		// otherwise ( Level > LevelC )
		// the cube should be scrolled

		if ( bCubeR != bCube ) // bCube is complemented 
		{
			bC0 = Cudd_Not( cuddE(bCubeR) );
			bC1 = Cudd_Not( cuddT(bCubeR) );
		}
		else
		{
			bC0 = cuddE(bCubeR);
			bC1 = cuddT(bCubeR);
		}

		// determine the polarity of the topmost var
		if ( bC1 == b0 )
			 bCube = bC0;
		else if ( bC0 == b0 )
			 bCube = bC1;
		else
		{  // this is not a cube
			assert(0);
		}
	}
	// never happens
	return NULL;
}


/**Function********************************************************************

  Synopsis    [Performs the recursive step of Extra_bddCubeExtractPositiveVars().]

  Description [Takes a cube with variables in negative and positive polarity.
  Returns a cube containing only those variables that were in the positive 
  polarity.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * extraBddCubeExtractPositiveVars( 
  DdManager * dd,       /* the DD manager */
  DdNode    * bCube)    /* the cube to be converted */
{
	DdNode * bRes;

	if ( bCube == b1 )
		return b1;

    if ( bRes = cuddCacheLookup1(dd, extraBddCubeExtractPositiveVars, bCube) )
    	return bRes;
	else
	{
		DdNode * bCubeR = Cudd_Regular(bCube);
		DdNode * bC0, * bC1;
		DdNode * bTemp;

		// get the cofactors
		if ( bCubeR != bCube ) // bCube is complemented 
		{
			bC0 = Cudd_Not( cuddE(bCubeR) );
			bC1 = Cudd_Not( cuddT(bCubeR) );
		}
		else
		{
			bC0 = cuddE(bCubeR);
			bC1 = cuddT(bCubeR);
		}

		// determine the polarity of the topmost var
		if ( bC1 == b0 )
		{ // the topmost var is in the negative polarity -> skip it
			bRes = extraBddCubeExtractPositiveVars( dd, bC0 );
			if ( bRes == NULL ) 
				return NULL;
		}
		else if ( bC0 == b0 )
		{  // the topmost var is in the positive polarity -> add it
			bRes = extraBddCubeExtractPositiveVars( dd, bC1 );
			if ( bRes == NULL ) 
				return NULL;
			cuddRef( bRes );

			assert( bRes != b0 );
			assert( !Cudd_IsComplement( bRes ) );
			bRes = cuddUniqueInter( dd, bCubeR->index, bTemp = bRes, b0 );
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref( dd, bTemp );
				return NULL;
			}
			cuddDeref( bTemp );
		}
		else
		{  // this is not a cube
			assert(0);
		}

		cuddCacheInsert1( dd, extraBddCubeExtractPositiveVars, bCube, bRes );
		return bRes;
	}
}	/* end of extraBddCubeExtractPositiveVars */



/**Function********************************************************************

  Synopsis    [Performs the recursive step of Extra_bddCubeConvertIntoPositiveVars().]

  Description [Takes a cube with variables in negative and positive polarity.
  Returns a cube containing all the variables in the positive polarity.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * extraBddCubeConvertIntoPositiveVars( 
  DdManager * dd,       /* the DD manager */
  DdNode    * bCube)    /* the cube to be converted */
{
	DdNode * bRes;

	if ( bCube == b1 )
		return b1;

    if ( bRes = cuddCacheLookup1(dd, extraBddCubeConvertIntoPositiveVars, bCube) )
    	return bRes;
	else
	{
		DdNode * bCubeR = Cudd_Regular(bCube);
		DdNode * bC0, * bC1;
		DdNode * bTemp;

		// get the cofactors
		if ( bCubeR != bCube ) // bCube is complemented 
		{
			bC0 = Cudd_Not( cuddE(bCubeR) );
			bC1 = Cudd_Not( cuddT(bCubeR) );
		}
		else
		{
			bC0 = cuddE(bCubeR);
			bC1 = cuddT(bCubeR);
		}

		// determine the polarity of the topmost var
		if ( bC1 == b0 )
		{ // the topmost var is in the negative polarity -> skip it
			bRes = extraBddCubeConvertIntoPositiveVars( dd, bC0 );
			if ( bRes == NULL ) 
				return NULL;
			cuddRef( bRes );
		}
		else if ( bC0 == b0 )
		{  // the topmost var is in the positive polarity -> add it
			bRes = extraBddCubeConvertIntoPositiveVars( dd, bC1 );
			if ( bRes == NULL ) 
				return NULL;
			cuddRef( bRes );
		}
		else
		{  // this is not a cube
			assert(0);
		}

		assert( bRes != b0 );
		assert( !Cudd_IsComplement( bRes ) );
		bRes = cuddUniqueInter( dd, bCubeR->index, bTemp = bRes, b0 );
		if ( bRes == NULL ) 
		{
			Cudd_RecursiveDeref( dd, bTemp );
			return NULL;
		}
		cuddDeref( bTemp );

		cuddCacheInsert1( dd, extraBddCubeConvertIntoPositiveVars, bCube, bRes );
		return bRes;
	}
}	/* end of extraBddCubeConvertIntoPositiveVars */



/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/
