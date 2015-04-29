/**CFile***********************************************************************

  FileName    [hmLast.c]

  PackageName [Rondo Heuristic]

  Synopsis    [Heuristic SOP minimizer implementing Espresso strategy using ZDDs.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_zddLastGasp();
				</ul>
			Internal procedures included in this module:
				<ul>
				<li> extraZddLastGasp();
				</ul>
			Static procedures included in this module:
				<ul>
				</ul>
	      ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$hmLast.c, v.1.2, January 16, 2001, alanmi $]

******************************************************************************/

#include "extra.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
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


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    []

  Description [Returns a pointer to the BDD if successful; NULL otherwise.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode	*
Extra_zddLastGasp(
  DdManager * dd,
  DdNode * zC,
  DdNode * bFuncOn,
  DdNode * bFuncOnDc)
{
    DdNode	*bSingleCovered;
	DdNode  *zWrappingCubes, *zWrappingPrimes, *zContainingPrimes;
	DdNode  *zCoverPlus, *zResult;

	/* compute the maximal reductions of all the cubes in the cover */
	bSingleCovered = Extra_zddSingleCoveredArea( dd, zC );
	Cudd_Ref( bSingleCovered );
	zWrappingCubes = Extra_zddWrapAreaIntoCubes( dd, zC, bSingleCovered );
	Cudd_Ref( zWrappingCubes );

	{ /* verification */
		DdNode * bSingleCoveredNew = Extra_zddSingleCoveredArea( dd, zWrappingCubes );
		Cudd_Ref( bSingleCoveredNew );
		assert( bSingleCoveredNew == bSingleCovered );
		Cudd_RecursiveDeref( dd, bSingleCoveredNew );
	}

	Cudd_RecursiveDeref( dd, bSingleCovered );

	/* compute the primes containing maximal redution cubes */
	zWrappingPrimes = Extra_zddExpand( dd, zWrappingCubes, bFuncOnDc, 0 );
	Cudd_Ref( zWrappingPrimes );

	/* find those primes that contain at least two cubes */
	zContainingPrimes = Extra_zddCubesCoveringTwoAndMore( dd, zWrappingCubes, zWrappingPrimes );
	Cudd_Ref( zContainingPrimes );
	Cudd_RecursiveDerefZdd( dd, zWrappingCubes );

	/* add these primes to the cover */
	zCoverPlus = Cudd_zddUnion( dd, zC, zContainingPrimes );
	Cudd_RecursiveDerefZdd( dd, zContainingPrimes );

	/* call the irredundant cover */
	zResult = Extra_zddIrredundant( dd, zCoverPlus, bFuncOn );
  	Cudd_Ref( zResult );
	Cudd_RecursiveDerefZdd( dd, zCoverPlus );
  
	Cudd_Deref( zResult );
	return zResult;
} /* end of Extra_zddLastGasp */


/**Function********************************************************************

  Synopsis    []

  Description [Returns a pointer to the BDD if successful; NULL otherwise.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode	*
Extra_zddWrapAreaIntoCubes(
  DdManager * dd,
  DdNode * zC,
  DdNode * bA)
{
    DdNode	*res;

    do {
	dd->reordered = 0;
	res = extraZddWrapAreaIntoCubes(dd, zC, bA);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddWrapAreaIntoCubes */


/**Function********************************************************************

  Synopsis    []

  Description [Returns a pointer to the BDD if successful; NULL otherwise.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode	*
Extra_zddCubesCoveringTwoAndMore(
  DdManager * dd,
  DdNode * zC,
  DdNode * zD)
{
    DdNode	*res;

    do {
	dd->reordered = 0;
	res = extraZddCubesCoveringTwoAndMore(dd, zC, zD);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddCubesCoveringTwoAndMore */

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis [Performs the recursive step of Extra_zddWrapAreaIntoCubes.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
extraZddWrapAreaIntoCubes(
  DdManager * dd,
  DdNode * zC,
  DdNode * bA)
{
    DdNode	*zRes;
    statLine(dd); 

	/* if there is nothing in the cover, there should be nothing to wrap */
	if ( zC == z0 )  { assert( bA == b0 ); return z0; }
	/* if the cover is the universe, the area should be not zero, return the minimal cube */
	if ( zC == z1 )  
	{
		DdNode * bCube, * zCube;
		assert( bA != b0 );

		bCube = Cudd_FindEssential( dd, bA );
		Cudd_Ref( bCube );

		zCube = Extra_zddConvertBddCubeIntoZddCube( dd, bCube );
		Cudd_Ref( zCube );
		Cudd_RecursiveDeref( dd, bCube );

		Cudd_Deref( zCube );
		return zCube;
	}
	/* if there is no area, there are shold not be no cubes */
	if ( bA == b0 )  { assert(0); return z0; }
	/* if the area is the total boolean space, there are shold not be no cubes, or the universe */
	if ( bA == b1 )  { assert(0); return z0; }

    /* check cache */
    zRes = cuddCacheLookup2Zdd(dd, extraZddWrapAreaIntoCubes, zC, bA);
    if (zRes)
		return zRes;
	else
	{
		DdNode *bA0, *bA1, *bA2;

		/* find the level in terms of the original variable levels, not ZDD variable levels */
		int levCover = dd->permZ[zC->index] >> 1;
		int levArea  = dd->perm[Cudd_Regular(bA)->index];
		/* determine whether the area is a complemented BDD */
		int fIsComp  = Cudd_IsComplement( bA );

		if ( levCover > levArea )
		{
			/* find the parts(cofactors) of the area */
			bA0 = Cudd_NotCond( Cudd_E( bA ), fIsComp );
			bA1 = Cudd_NotCond( Cudd_T( bA ), fIsComp );

			/* find the union of cofactors */
			bA2 = cuddBddAndRecur( dd, Cudd_Not(bA0), Cudd_Not(bA0) );
			if ( bA2 == NULL ) return NULL;
			cuddRef( bA2 );
			bA2 = Cudd_Not(bA2);

			/* only those cubes are contained which are contained in the intersection */
			zRes = extraZddWrapAreaIntoCubes( dd, zC, bA2 );
			if ( zRes == NULL ) return NULL;
			cuddRef( zRes );
			Cudd_RecursiveDeref( dd, bA2 );
			cuddDeref( zRes );
		}
		else /* if ( levCover <= levArea ) */
		{
			DdNode *bA0out, *bA1out;
			DdNode *zRes0, *zRes1, *zRes2, *zTemp;
			int TopZddVar;

			/* decompose the cover */
			DdNode *zC0, *zC1, *zC2;
			extraDecomposeCover(dd, zC, &zC0, &zC1, &zC2);

			if ( levCover == levArea )
			{
				/* find the parts(cofactors) of the area */
				bA0 = Cudd_NotCond( Cudd_E( bA ), fIsComp );
				bA1 = Cudd_NotCond( Cudd_T( bA ), fIsComp );
			}
			else /* if ( levCover < levArea ) */
			{
				/* assign the cofactors and their intersection */
				bA0 = bA1 = bA;
			}

			/* find the area not covered by C0 */
			bA0out = extraZddConvertToBddAndAdd( dd, zC0, Cudd_Not(bA0) );
			if ( bA0out == NULL )
				return NULL;
			cuddRef( bA0out );
			bA0out = Cudd_Not(bA0out);

			/* find the area not covered by C1 */
			bA1out = extraZddConvertToBddAndAdd( dd, zC1, Cudd_Not(bA1) );
			if ( bA1out == NULL )
			{
				Cudd_RecursiveDeref( dd, bA0out );
				return NULL;
			}
			cuddRef( bA1out );
			bA1out = Cudd_Not(bA1out);

			/* find the area to be covered by C2 */

			/* find the area not covered by C0 */
			bA0 = cuddBddAndRecur( dd, bA0, Cudd_Not(bA0out) );
			if ( bA0 == NULL )
			{
				Cudd_RecursiveDeref( dd, bA0out );
				Cudd_RecursiveDeref( dd, bA1out );
				return NULL;
			}
			cuddRef( bA0 );

			/* find the area not covered by C0 */
			bA1 = cuddBddAndRecur( dd, bA1, Cudd_Not(bA1out) );
			if ( bA1 == NULL )
			{
				Cudd_RecursiveDeref( dd, bA0 );
				Cudd_RecursiveDeref( dd, bA0out );
				Cudd_RecursiveDeref( dd, bA1out );
				return NULL;
			}
			cuddRef( bA1 );

			/* find the area not covered by C1 */
			bA2 = cuddBddAndRecur( dd, Cudd_Not(bA0out), Cudd_Not(bA1out) );
			if ( bA2 == NULL )
			{
				Cudd_RecursiveDeref( dd, bA0 );
				Cudd_RecursiveDeref( dd, bA1 );
				Cudd_RecursiveDeref( dd, bA0out );
				Cudd_RecursiveDeref( dd, bA1out );
				return NULL;
			}
			cuddRef( bA2 );
			bA2 = Cudd_Not(bA2);
			Cudd_RecursiveDeref( dd, bA0out );
			Cudd_RecursiveDeref( dd, bA1out );


			/* solve subproblems */

			/* cover without literal can only be contained in the intersection */
			zRes0 = extraZddWrapAreaIntoCubes( dd, zC0, bA0 );
			if ( zRes0 == NULL )
			{
				Cudd_RecursiveDeref( dd, bA0 );
				Cudd_RecursiveDeref( dd, bA1 );
				Cudd_RecursiveDeref( dd, bA2 );
				return NULL;
			}
			cuddRef( zRes0 );
			Cudd_RecursiveDeref( dd, bA0 );

			/* cover without literal can only be contained in the intersection */
			zRes1 = extraZddWrapAreaIntoCubes( dd, zC1, bA1 );
			if ( zRes1 == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDeref( dd, bA1 );
				Cudd_RecursiveDeref( dd, bA2 );
				return NULL;
			}
			cuddRef( zRes1 );
			Cudd_RecursiveDeref( dd, bA1 );

			/* cover without literal can only be contained in the intersection */
			zRes2 = extraZddWrapAreaIntoCubes( dd, zC2, bA2 );
			if ( zRes2 == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				Cudd_RecursiveDeref( dd, bA2 );
				return NULL;
			}
			cuddRef( zRes2 );
			Cudd_RecursiveDeref( dd, bA2 );

			/* --------------- compose the result ------------------ */
			/* the index of the positive ZDD variable in zC */
			TopZddVar = (zC->index >> 1) << 1;

			/* compose with-neg-var and without-var using the neg ZDD var */
			zTemp = cuddZddGetNode(dd, TopZddVar + 1, zRes0, zRes2 );
			if ( zTemp == NULL ) 
			{
				Cudd_RecursiveDerefZdd(dd, zRes0);
				Cudd_RecursiveDerefZdd(dd, zRes1);
				Cudd_RecursiveDerefZdd(dd, zRes2);
				return NULL;
			}
			cuddRef( zTemp );
			cuddDeref( zRes0 );
			cuddDeref( zRes2 );

			/* compose with-pos-var and previous result using the pos ZDD var */
			zRes = cuddZddGetNode(dd, TopZddVar, zRes1, zTemp );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd(dd, zRes1);
				Cudd_RecursiveDerefZdd(dd, zTemp);
				return NULL;
			}
			cuddDeref( zRes1 );
			cuddDeref( zTemp );
		}

		/* insert the result into cache and return */
		cuddCacheInsert2(dd, extraZddWrapAreaIntoCubes, zC, bA, zRes);
		return zRes;
	}
} /* end of extraZddWrapAreaIntoCubes */


/**Function********************************************************************

  Synopsis [Performs the recursive step of Cudd_zddCubesCoveringTwoAndMore.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
extraZddCubesCoveringTwoAndMore(
  DdManager * dd,
  DdNode * zC,
  DdNode * zD)
{
    DdNode	*zRes;
    statLine(dd); 

	/* if there is nothing in the cover, there should be nothing to cover */
	if ( zC == z0 )  { assert( zD == z0 ); return z0; }
	/* if the cover is the universe, count the number of cubes in the second cover */
	if ( zC == z1 )  
	{
		while ( zD != z1 )
		{
			if ( cuddE(zD) == z0 ) /* one cube so far */
				zD = cuddT(zD);
			else if ( cuddT(zD) == z0 ) /* one cube so far */
				zD = cuddE(zD);
			else /* there are at least two cubes!!!  - return the cover */
				return zC;
		}
		/* there is only one cube in zD - return empty */
		return z0;
	}
	/* if the second cover is empty and the first cover is not - impossible */
	if ( zD == z0 )  { assert(0); return z0; }
	/* if the second cover is the universe and the second cover is not - impossible */
	if ( zD == z1 )  { assert(0); return z0; }

    /* check cache */
    zRes = cuddCacheLookup2Zdd(dd, extraZddCubesCoveringTwoAndMore, zC, zD);
    if (zRes)
		return zRes;
	else
	{
		/* find the level in terms of the original variable levels, not ZDD variable levels */
		int levC = dd->permZ[zC->index] >> 1;
		int levD = dd->permZ[zD->index] >> 1;

		if ( levC < levD )
		{ /* the literal is present in the second cover and not in the first - impossible */
			assert(0);
			return z0;
		}
		else /* if ( levC >= levD ) */
		{
			DdNode *zRes0, *zRes1, *zRes2;
			DdNode *zC0, *zC1, *zC2;
			DdNode *zD0, *zD1, *zD2, *zTemp;
			int TopZddVar;

			/* decompose covers */
			if ( levC == levD )
				extraDecomposeCover(dd, zC, &zC0, &zC1, &zC2);
			else /* if ( levC > levD ) */
				zC0 = zC1 = z0, zC2 = zD;
			extraDecomposeCover(dd, zD, &zD0, &zD1, &zD2);

			/* solve subproblems */

			zRes0 = extraZddCubesCoveringTwoAndMore( dd, zC0, zD0 );
			if ( zRes0 == NULL )
				return NULL;
			cuddRef( zRes0 );

			zRes1 = extraZddCubesCoveringTwoAndMore( dd, zC1, zD1 );
			if ( zRes1 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zRes0);
				return NULL;
			}
			cuddRef( zRes1 );

			zRes2 = extraZddCubesCoveringTwoAndMore( dd, zC2, zD2 );
			if ( zRes2 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zRes0);
				Cudd_RecursiveDerefZdd(dd, zRes1);
				return NULL;
			}
			cuddRef( zRes2 );

			/* --------------- compose the result ------------------ */
			/* the index of the positive ZDD variable in zC */
			TopZddVar = (zD->index >> 1) << 1;

			/* compose with-neg-var and without-var using the neg ZDD var */
			zTemp = cuddZddGetNode(dd, TopZddVar + 1, zRes0, zRes2 );
			if ( zTemp == NULL ) 
			{
				Cudd_RecursiveDerefZdd(dd, zRes0);
				Cudd_RecursiveDerefZdd(dd, zRes1);
				Cudd_RecursiveDerefZdd(dd, zRes2);
				return NULL;
			}
			cuddRef( zTemp );
			cuddDeref( zRes0 );
			cuddDeref( zRes2 );

			/* compose with-pos-var and previous result using the pos ZDD var */
			zRes = cuddZddGetNode(dd, TopZddVar, zRes1, zTemp );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd(dd, zRes1);
				Cudd_RecursiveDerefZdd(dd, zTemp);
				return NULL;
			}
			cuddDeref( zRes1 );
			cuddDeref( zTemp );
		}

		/* insert the result into cache and return */
		cuddCacheInsert2(dd, extraZddCubesCoveringTwoAndMore, zC, zD, zRes);
		return zRes;
	}
} /* end of extraZddCubesCoveringTwoAndMore */


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/
