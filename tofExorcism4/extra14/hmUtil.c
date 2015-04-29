/**CFile***********************************************************************

  FileName    [hmUtil.c]

  PackageName [Rondo Heuristic]

  Synopsis    [Heuristic SOP minimizer implementing Espresso strategy using ZDDs.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_zddCoveredByArea();
				<li> Extra_zddNotCoveredByCover();
				<li> Extra_zddOverlappingWithArea();
				<li> Extra_zddConvertToBdd();
				<li> Extra_zddConvertEsopToBdd();
				<li> Extra_zddConvertToBddAndAdd();
				<li> Extra_zddSingleCoveredArea();
				<li> Extra_zddVerifyCover();
				</ul>
			Internal procedures included in this module:
				<ul>
				<li> extraZddCoveredByArea();
				<li> extraZddNotCoveredByCover();
				<li> extraZddOverlappingWithArea();
				<li> extraZddConvertToBdd();
				<li> extraZddConvertEsopToBdd();
				<li> extraZddConvertToBddAndAdd();
				<li> extraZddSingleCoveredArea();
				<li> extraDecomposeCover();
				</ul>
			Static procedures included in this module:
				<ul>
				</ul>
	      ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$hmUtil.c, v.1.2, January 16, 2001, alanmi $]

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

  Synopsis    [Selects cubes from the cover that are completely contained in the area.]

  Description [This function is similar to Extra_zddSubSet(X,Y) which selects 
  those subsets of X that are completely contained in at least one subset of Y. 
  Extra_zddCoveredByArea() filters the cover of cubes and returns only those cubes 
  that are completely contained in the area. The cover is given as a ZDD, the 
  area is a BDD. Returns the reduced cube set on success; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [Extra_zddSubSet]

******************************************************************************/
DdNode	*
Extra_zddCoveredByArea(
  DdManager * dd,
  DdNode * zC,
  DdNode * bA)
{
    DdNode	*res;

    do {
	dd->reordered = 0;
	res = extraZddCoveredByArea(dd, zC, bA);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddCoveredByArea */


/**Function********************************************************************

  Synopsis    [Selects cubes from the cover that are not completely covered by other cover.]

  Description [This function is equivalent to first converting the second cover 
  into BDD, calling Extra_zddCoveredByArea(), and then subtracting from the
  first cover the result of the latter operation. However, it is more efficient.
  Returns the reduced cube set on success; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [Extra_zddSubSet]

******************************************************************************/
DdNode	*
Extra_zddNotCoveredByCover(
  DdManager * dd,
  DdNode * zC,
  DdNode * zD)
{
    DdNode	*res;

    do {
	dd->reordered = 0;
	res = extraZddNotCoveredByCover(dd, zC, zD);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddNotCoveredByCover */


/**Function********************************************************************

  Synopsis    [Selects cubes from the cover that are overlapping with the area.]

  Description [This function is similar to Extra_zddNotSubSet(X,Y) which selects 
  those subsets of X that are completely contained in at least one subset of Y. 
  Extra_zddOverlappingWithArea() filters the cover of cubes and returns only those 
  cubes that overlap with the area. The completely contained cubes are counted as
  overlapping. The cover is given as a ZDD, the area is a BDD. Returns the reduced 
  cube set on success; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [Extra_zddSubSet]

******************************************************************************/
DdNode	*
Extra_zddOverlappingWithArea(
  DdManager * dd,
  DdNode * zC,
  DdNode * bA)
{
    DdNode	*res;

    do {
	dd->reordered = 0;
	res = extraZddOverlappingWithArea(dd, zC, bA);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddOverlappingWithArea */


/**Function********************************************************************

  Synopsis    [Converts the ZDD into the BDD.]

  Description [This function is a reimplementation of Cudd_MakeBddFromZddCover 
  for the case when BDD variable and ZDD variable orders are synchronized. 
  It is the user's responsibility to ensure the synchronization. This function is
  more efficient than Cudd_MakeBddFromZddCover because it does not require
  referencing cofactors of the cover and because it used cuddUniqueInterO() 
  instead of cuddUniqueInterIVO(). Returns the bdd on success; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [Cudd_MakeBddFromZddCover]

******************************************************************************/
DdNode	*
Extra_zddConvertToBdd(
  DdManager * dd,
  DdNode * zC)
{
    DdNode	*res;

    do {
	dd->reordered = 0;
	res = extraZddConvertToBdd(dd, zC);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddConvertToBdd */

/**Function********************************************************************

  Synopsis    [Converts the ZDD for ESOP into the BDD.]

  Description [Returns the bdd on success; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [Cudd_MakeBddFromZddCover Extra_zddConvertToBdd]

******************************************************************************/
DdNode	*
Extra_zddConvertEsopToBdd(
  DdManager * dd,
  DdNode * zC)
{
    DdNode	*res;

    do {
	dd->reordered = 0;
	res = extraZddConvertEsopToBdd(dd, zC);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddConvertEsopToBdd */


/**Function********************************************************************

  Synopsis    [Converts ZDD into BDD while at the same time adding another BDD to it.]

  Description [This function is equivalent to first calling Extra_zddConvertToBdd()
  and then Cudd_bddOr() but is more efficient. Returns the reduced cube set on 
  success; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [Extra_zddConvertToBdd]

******************************************************************************/
DdNode	*
Extra_zddConvertToBddAndAdd(
  DdManager * dd,
  DdNode * zC,
  DdNode * bA)
{
    DdNode	*res;

    do {
	dd->reordered = 0;
	res = extraZddConvertToBddAndAdd(dd, zC, bA);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddConvertToBddAndAdd */


/**Function********************************************************************

  Synopsis    [Finds the area covered by only one cube from cover.]

  Description [This function computes the BDD of the area covered by only one cube
  from the give cover. Returns the reduced cube set on success; NULL otherwise.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
Extra_zddSingleCoveredArea(
  DdManager * dd,
  DdNode * zC)
{
    DdNode	*res;

    do {
	dd->reordered = 0;
	res = extraZddSingleCoveredArea(dd, zC);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddSingleCoveredArea */

/**Function********************************************************************

  Synopsis    [Converts the BDD cube into the ZDD cube.]

  Description [Returns the pointer to the ZDD on success; NULL otherwise.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
Extra_zddConvertBddCubeIntoZddCube(
  DdManager * dd,
  DdNode * bCube)
{
    DdNode	*zCube;
	int *VarValues, i;

	/* allocate temporary storage for variable values */
	assert( dd->sizeZ == 2 * dd->size );
	VarValues = (int*) malloc( dd->sizeZ * sizeof(int) );
	if ( VarValues == NULL ) 
	{
		dd->errorCode = CUDD_MEMORY_OUT;
		return NULL;
	}
	/* clean the storage */
	for ( i = 0; i < dd->sizeZ; i++ )
		VarValues[i] = 0;
	/* get the variable values */
	while ( bCube != b1 )
	{
		assert( !Cudd_IsComplement(bCube) );
		if ( Cudd_E(bCube) == b0 ) /* positive literal */
			VarValues[2*bCube->index] = 1, bCube = Cudd_T(bCube);
		else if ( Cudd_T(bCube) == b0 ) /* negative literal */
			VarValues[2*bCube->index+1] = 1, bCube = Cudd_E(bCube);
		else
			assert(0);
		assert( bCube != b0 );
	}
	/* get the ZDD cube */
	zCube = Extra_zddCombination( dd, VarValues, dd->sizeZ );
	free(VarValues);
	return zCube;
} /* end of Extra_zddConvertBddCubeIntoZddCube */

/**Function********************************************************************

  Synopsis    [Verifies whether the cover belongs to the specified range.]

  Description [Returns 1 if verification succeeds; 0 otherwise.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
int
Extra_zddVerifyCover(
  DdManager * dd,
  DdNode * zC,
  DdNode * bFuncOn,
  DdNode * bFuncOnDc)
{
	DdNode * bCover;
	int fVerificationOkay = 1;

	bCover = Extra_zddConvertToBdd( dd, zC );
	Cudd_Ref( bCover );

	if ( Cudd_bddIteConstant( dd, bFuncOn, bCover, dd->one ) != dd->one )
	{
		fVerificationOkay = 0;
		printf(" - Verification not okey: Solution does not cover the on-set\n");
	}
	if ( Cudd_bddIteConstant( dd, bCover, bFuncOnDc, dd->one ) != dd->one )
	{
		fVerificationOkay = 0;
		printf(" - Verification not okey: Solution overlaps with the off-set\n");
	}
	if ( fVerificationOkay )
		printf(" +\n");

	Cudd_RecursiveDeref( dd, bCover );
	return fVerificationOkay;
} /* end of Extra_zddVerifyCover */



/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis [Performs the recursive step of Extra_zddCoveredByArea.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
extraZddCoveredByArea(
  DdManager * dd,
  DdNode * zC,   /* the ZDD for the cover */
  DdNode * bA)   /* the BDD for the area */
{
    DdNode	*zRes;
    statLine(dd); 

	/* if there is no area, there are no contained cubes */
	if ( bA == b0 )  return z0;
	/* if the area is the total boolean space, the cover is contained completely */
	if ( bA == b1 )  return zC;
	/* if there is nothing in the cover, nothing is contained */
	if ( zC == z0 )  return z0;
	/* if the cover is one large cube (and the area is less than the total boolean space), 
	   nothing is contained */
	if ( zC == z1 )  return z0;

    /* check cache */
    zRes = cuddCacheLookup2Zdd(dd, extraZddCoveredByArea, zC, bA);
    if (zRes)
		return zRes;
	else
	{
		DdNode *bA0, *bA1, *bA01;

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

			/* find the intersection of cofactors */
			bA01 = cuddBddAndRecur( dd, bA0, bA1 );
			if ( bA01 == NULL ) return NULL;
			cuddRef( bA01 );

			/* only those cubes are contained which are contained in the intersection */
			zRes = extraZddCoveredByArea( dd, zC, bA01 );
			if ( zRes == NULL ) return NULL;
			cuddRef( zRes );
			Cudd_RecursiveDeref( dd, bA01 );
			cuddDeref( zRes );
		}
		else /* if ( levCover <= levArea ) */
		{
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

				/* find the intersection of cofactors */
				bA01 = cuddBddAndRecur( dd, bA0, bA1 );
				if ( bA01 == NULL ) return NULL;
				cuddRef( bA01 );

			}
			else /* if ( levCover < levArea ) */
			{
				/* assign the cofactors and their intersection */
				bA0 = bA1 = bA01 = bA;
				/* reference the intersection for uniformity with the above case */
				cuddRef( bA01 );
			}

			/* solve subproblems */

			/* cover without literal can only be contained in the intersection */
			zRes2 = extraZddCoveredByArea( dd, zC2, bA01 );
			if ( zRes2 == NULL )
			{
				Cudd_RecursiveDeref( dd, bA01 );
				return NULL;
			}
			cuddRef( zRes2 );
			Cudd_RecursiveDeref( dd, bA01 );

			/* cover with negative literal can only be contained in bA0 */
			zRes0 = extraZddCoveredByArea( dd, zC0, bA0 );
			if ( zRes0 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zRes2);
				return NULL;
			}
			cuddRef( zRes0 );

			/* cover with positive literal can only be contained in bA1 */
			zRes1 = extraZddCoveredByArea( dd, zC1, bA1 );
			if ( zRes1 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zRes0);
				Cudd_RecursiveDerefZdd(dd, zRes2);
				return NULL;
			}
			cuddRef( zRes1 );

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
		cuddCacheInsert2(dd, extraZddCoveredByArea, zC, bA, zRes);
		return zRes;
	}
} /* end of extraZddCoveredByArea */



/**Function********************************************************************

  Synopsis [Performs the recursive step of Cudd_zddNotCoveredByCover.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
extraZddNotCoveredByCover(
  DdManager * dd,
  DdNode * zC,   /* the ZDD for the cover */
  DdNode * zD)   /* the ZDD for the second cover */
{
    DdNode	*zRes;
    statLine(dd); 

	/* if the second cover is empty, the first cover is not covered */
	if ( zD == z0 )  return zC;
	/* if the second cover is full, not a part of the first cover is not covered */
	if ( zD == z1 )  return z0;
	/* if there is nothing in the cover, nothing is not covered */
	if ( zC == z0 )  return z0;
	/* if the first cover is full (and the second cover is not empty), 
	   the universe is partially covered, so the result is empty */
	if ( zC == z1 )  return z1;

    /* check cache */
    zRes = cuddCacheLookup2Zdd(dd, extraZddNotCoveredByCover, zC, zD);
    if (zRes)
		return zRes;
	else
	{
		/* find the level in terms of the original variable levels, not ZDD variable levels */
		int levC = dd->permZ[zC->index] >> 1;
		int levD = dd->permZ[zD->index] >> 1;

		if ( levC > levD )
		{
			DdNode *zD0, *zD1, *zD2;
			extraDecomposeCover(dd, zD, &zD0, &zD1, &zD2);

			/* C cannot be covered by zD0 and zD1 - only zD2 left to consider */
			zRes = extraZddNotCoveredByCover( dd, zC, zD2 );
			if ( zRes == NULL ) return NULL;
		}
		else /* if ( levC <= levD ) */
		{
			DdNode *zRes0, *zRes1, *zRes2;
			DdNode *zC0, *zC1, *zC2;
			DdNode *zD0, *zD1, *zD2, *zTemp;
			int TopZddVar;

			/* decompose covers */
			extraDecomposeCover(dd, zC, &zC0, &zC1, &zC2);
			if ( levC == levD )
				extraDecomposeCover(dd, zD, &zD0, &zD1, &zD2);
			else /* if ( levC < levD ) */
				zD0 = zD1 = z0, zD2 = zD;

			/* solve subproblems */

			/* cover with negative literal can only be contained in zD0 + zD2 */
			zTemp = cuddZddUnion( dd, zD0, zD2 );
			if ( zTemp == NULL )
				return NULL;
			cuddRef( zTemp );

			zRes0 = extraZddNotCoveredByCover( dd, zC0, zTemp );
			if ( zRes0 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zTemp);
				return NULL;
			}
			cuddRef( zRes0 );
			Cudd_RecursiveDerefZdd(dd, zTemp);

			/* cover with positive literal can only be contained in zD1 + zD2 */
			zTemp = cuddZddUnion( dd, zD1, zD2 );
			if ( zTemp == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zRes0);
				return NULL;
			}
			cuddRef( zTemp );

			zRes1 = extraZddNotCoveredByCover( dd, zC1, zTemp );
			if ( zRes1 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zTemp);
				Cudd_RecursiveDerefZdd(dd, zRes0);
				return NULL;
			}
			cuddRef( zRes1 );
			Cudd_RecursiveDerefZdd(dd, zTemp);


			/* cover without literal can only be contained in zD2 */
			zRes2 = extraZddNotCoveredByCover( dd, zC2, zD2 );
			if ( zRes2 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zRes0);
				Cudd_RecursiveDerefZdd(dd, zRes1);
				return NULL;
			}
			cuddRef( zRes2 );

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
		cuddCacheInsert2(dd, extraZddNotCoveredByCover, zC, zD, zRes);
		return zRes;
	}
} /* end of extraZddNotCoveredByCover */



/**Function********************************************************************

  Synopsis [Performs the recursive step of Cudd_zddOverlappingWithArea.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
extraZddOverlappingWithArea(
  DdManager * dd,
  DdNode * zC,   /* the ZDD for the cover */
  DdNode * bA)   /* the BDD for the area */
{
    DdNode	*zRes;
    statLine(dd); 

	/* if there is no area, there are no overlapping cubes */
	if ( bA == b0 )  return z0;
	/* if the area is the total boolean space, all cubes of the cover overlap with area */
	if ( bA == b1 )  return zC;
	/* if there is nothing in the cover, nothing to overlap */
	if ( zC == z0 )  return z0;
	/* if the cover is the universe (and the area is something), the universe overlaps */
	if ( zC == z1 )  return zC;

    /* check cache */
    zRes = cuddCacheLookup2Zdd(dd, extraZddOverlappingWithArea, zC, bA);
    if (zRes)
		return zRes;
	else
	{
		DdNode *bA0, *bA1, *bA01;

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
			bA01 = cuddBddAndRecur( dd, Cudd_Not(bA0), Cudd_Not(bA1) );
			if ( bA01 == NULL ) return NULL;
			cuddRef( bA01 );
			bA01 = Cudd_Not(bA01);

			/* those cubes overlap, which overlap with the union of cofactors */
			zRes = extraZddOverlappingWithArea( dd, zC, bA01 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDeref( dd, bA01 );
				return NULL;
			}
			cuddRef( zRes );
			Cudd_RecursiveDeref( dd, bA01 );
			cuddDeref( zRes );
		}
		else /* if ( levCover <= levArea ) */
		{
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

				/* find the union of cofactors */
				bA01 = cuddBddAndRecur( dd, Cudd_Not(bA0), Cudd_Not(bA1) );
				if ( bA01 == NULL ) return NULL;
				cuddRef( bA01 );
				bA01 = Cudd_Not(bA01);
			}
			else /* if ( levCover < levArea ) */
			{
				/* assign the cofactors and their union */
				bA0 = bA1 = bA01 = bA;
				/* reference the union for uniformity with the above case */
				cuddRef( bA01 );
			}

			/* solve subproblems */

			/* cover without literal overlaps with the union */
			zRes2 = extraZddOverlappingWithArea( dd, zC2, bA01 );
			if ( zRes2 == NULL )
			{
				Cudd_RecursiveDeref( dd, bA01 );
				return NULL;
			}
			cuddRef( zRes2 );
			Cudd_RecursiveDeref( dd, bA01 );

			/* cover with negative literal overlaps with bA0 */
			zRes0 = extraZddOverlappingWithArea( dd, zC0, bA0 );
			if ( zRes0 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zRes2);
				return NULL;
			}
			cuddRef( zRes0 );

			/* cover with positive literal overlaps with bA1 */
			zRes1 = extraZddOverlappingWithArea( dd, zC1, bA1 );
			if ( zRes1 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zRes0);
				Cudd_RecursiveDerefZdd(dd, zRes2);
				return NULL;
			}
			cuddRef( zRes1 );

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
		cuddCacheInsert2(dd, extraZddOverlappingWithArea, zC, bA, zRes);
		return zRes;
	}
} /* end of extraZddOverlappingWithArea */


/**Function********************************************************************

  Synopsis [Performs the recursive step of Cudd_zddConvertToBdd.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
extraZddConvertToBdd(
  DdManager * dd,
  DdNode * zC)
{
    DdNode	*bRes;
    statLine(dd); 

	/* if there is no cover, there is no BDD */
	if ( zC == z0 )  return b0;
	/* if the cover is the universe, the BDD is constant 1 */
	if ( zC == z1 )  return b1;

    /* check cache */
    bRes = cuddCacheLookup1(dd, extraZddConvertToBdd, zC);
    if (bRes)
		return(bRes);
	else
	{
		DdNode * bRes0, * bRes1, * bRes2, * bTemp;
		DdNode * zC0, * zC1, * zC2;
		int TopBddVar = (zC->index >> 1);

		/* cofactor the cover */
		extraDecomposeCover(dd, zC, &zC0, &zC1, &zC2);

		/* compute bdds for the three cofactors of the cover */
		bRes0 = extraZddConvertToBdd(dd, zC0);
		if ( bRes0 == NULL )
			return NULL;
		cuddRef( bRes0 );

		bRes1 = extraZddConvertToBdd(dd, zC1);
		if ( bRes1 == NULL )
		{
			Cudd_RecursiveDeref(dd, bRes0);
			return NULL;
		}
		cuddRef( bRes1 );

		bRes2 = extraZddConvertToBdd(dd, zC2);
		if ( bRes2 == NULL )
		{
			Cudd_RecursiveDeref(dd, bRes0);
			Cudd_RecursiveDeref(dd, bRes1);
			return NULL;
		}
		cuddRef( bRes2 );

		/* compute  bdd(zC0)+bdd(zC2) */
		bTemp = bRes0;
		bRes0 = cuddBddAndRecur(dd, Cudd_Not(bRes0), Cudd_Not(bRes2));
		if ( bRes0 == NULL )
		{
			Cudd_RecursiveDeref(dd, bTemp);
			Cudd_RecursiveDeref(dd, bRes1);
			Cudd_RecursiveDeref(dd, bRes2);
			return NULL;
		}
		cuddRef( bRes0 );
		bRes0 = Cudd_Not(bRes0);
		Cudd_RecursiveDeref(dd, bTemp);

		/* compute  bdd(zC1)+bdd(zC2) */
		bTemp = bRes1;
		bRes1 = cuddBddAndRecur(dd, Cudd_Not(bRes1), Cudd_Not(bRes2));
		if ( bRes1 == NULL )
		{
			Cudd_RecursiveDeref(dd, bRes0);
			Cudd_RecursiveDeref(dd, bTemp);
			Cudd_RecursiveDeref(dd, bRes2);
			return NULL;
		}
		cuddRef( bRes1 );
		bRes1 = Cudd_Not(bRes1);
		Cudd_RecursiveDeref(dd, bTemp);
		Cudd_RecursiveDeref(dd, bRes2);
		/* only bRes0 and bRes1 are referenced at this point */

		/* consider the case when Res0 and Res1 are the same node */
		if ( bRes0 == bRes1 )
			bRes = bRes1;
		/* consider the case when Res1 is complemented */
		else if ( Cudd_IsComplement(bRes1) ) 
		{
			bRes = cuddUniqueInter(dd, TopBddVar, Cudd_Not(bRes1), Cudd_Not(bRes0));
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes0);
				Cudd_RecursiveDeref(dd,bRes1);
				return NULL;
			}
			bRes = Cudd_Not(bRes);
		} 
		else 
		{
			bRes = cuddUniqueInter( dd, TopBddVar, bRes1, bRes0 );
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes0);
				Cudd_RecursiveDeref(dd,bRes1);
				return NULL;
			}
		}
		cuddDeref( bRes0 );
		cuddDeref( bRes1 );

		/* insert the result into cache */
		cuddCacheInsert1(dd, extraZddConvertToBdd, zC, bRes);
		return bRes;
	}
} /* end of extraZddConvertToBdd */


/**Function********************************************************************

  Synopsis [Performs the recursive step of Cudd_zddConvertEsopToBdd.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
extraZddConvertEsopToBdd(
  DdManager * dd,
  DdNode * zC)
{
    DdNode	*bRes;
    statLine(dd); 

	/* if there is no cover, there is no BDD */
	if ( zC == z0 )  return b0;
	/* if the cover is the universe, the BDD is constant 1 */
	if ( zC == z1 )  return b1;

    /* check cache */
    bRes = cuddCacheLookup1(dd, extraZddConvertEsopToBdd, zC);
    if (bRes)
		return(bRes);
	else
	{
		DdNode * bRes0, * bRes1, * bRes2, * bTemp;
		DdNode * zC0, * zC1, * zC2;
		int TopBddVar = (zC->index >> 1);

		/* cofactor the cover */
		extraDecomposeCover(dd, zC, &zC0, &zC1, &zC2);

		/* compute bdds for the three cofactors of the cover */
		bRes0 = extraZddConvertEsopToBdd(dd, zC0);
		if ( bRes0 == NULL )
			return NULL;
		cuddRef( bRes0 );

		bRes1 = extraZddConvertEsopToBdd(dd, zC1);
		if ( bRes1 == NULL )
		{
			Cudd_RecursiveDeref(dd, bRes0);
			return NULL;
		}
		cuddRef( bRes1 );

		bRes2 = extraZddConvertEsopToBdd(dd, zC2);
		if ( bRes2 == NULL )
		{
			Cudd_RecursiveDeref(dd, bRes0);
			Cudd_RecursiveDeref(dd, bRes1);
			return NULL;
		}
		cuddRef( bRes2 );

		/* compute  bdd(zC0) (+) bdd(zC2) */
		bRes0 = cuddBddXorRecur(dd, bTemp = bRes0, bRes2);
		if ( bRes0 == NULL )
		{
			Cudd_RecursiveDeref(dd, bTemp);
			Cudd_RecursiveDeref(dd, bRes1);
			Cudd_RecursiveDeref(dd, bRes2);
			return NULL;
		}
		cuddRef( bRes0 );
		Cudd_RecursiveDeref(dd, bTemp);

		/* compute  bdd(zC1) (+) bdd(zC2) */
		bRes1 = cuddBddXorRecur(dd, bTemp = bRes1, bRes2);
		if ( bRes1 == NULL )
		{
			Cudd_RecursiveDeref(dd, bRes0);
			Cudd_RecursiveDeref(dd, bTemp);
			Cudd_RecursiveDeref(dd, bRes2);
			return NULL;
		}
		cuddRef( bRes1 );
		Cudd_RecursiveDeref(dd, bTemp);
		Cudd_RecursiveDeref(dd, bRes2);
		/* only bRes0 and bRes1 are referenced at this point */

		/* consider the case when Res0 and Res1 are the same node */
		if ( bRes0 == bRes1 )
			bRes = bRes1;
		/* consider the case when Res1 is complemented */
		else if ( Cudd_IsComplement(bRes1) ) 
		{
			bRes = cuddUniqueInter(dd, TopBddVar, Cudd_Not(bRes1), Cudd_Not(bRes0));
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes0);
				Cudd_RecursiveDeref(dd,bRes1);
				return NULL;
			}
			bRes = Cudd_Not(bRes);
		} 
		else 
		{
			bRes = cuddUniqueInter( dd, TopBddVar, bRes1, bRes0 );
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes0);
				Cudd_RecursiveDeref(dd,bRes1);
				return NULL;
			}
		}
		cuddDeref( bRes0 );
		cuddDeref( bRes1 );

		/* insert the result into cache */
		cuddCacheInsert1(dd, extraZddConvertEsopToBdd, zC, bRes);
		return bRes;
	}
} /* end of extraZddConvertEsopToBdd */



/**Function********************************************************************

  Synopsis [Performs the recursive step of Cudd_zddConvertToBddAndAdd.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
extraZddConvertToBddAndAdd(
  DdManager * dd,
  DdNode * zC,     /* the cover */
  DdNode * bA)     /* the area */
{
    DdNode	*bRes;
    statLine(dd); 

	/* if there is no cover, return the BDD */
	if ( zC == z0 )  return bA;
	/* if the cover is the universe, the sum is constant 1 */
	if ( zC == z1 )  return b1;
	/* if the area is absent, convert cover to BDD */
	if ( bA == b0 )  return extraZddConvertToBdd(dd,zC);
	/* if the area is constant 1, the BDD is also constant 1 */
	if ( bA == b1 )  return b1;

    /* check cache */
    bRes = cuddCacheLookup2(dd, extraZddConvertToBddAndAdd, zC, bA);
    if (bRes)
		return(bRes);
	else
	{
		DdNode * bRes0, * bRes1, * bRes2;
		DdNode * bA0, * bA1, * bTemp;
		int TopBddVar;
		/* find the level in terms of the original variable levels, not ZDD variable levels */
		int levCover = dd->permZ[zC->index] >> 1;
		int levArea  = dd->perm[Cudd_Regular(bA)->index];
		/* determine whether the area is a complemented BDD */
		int fIsComp  = Cudd_IsComplement(bA);

		if ( levCover > levArea )
		{
			TopBddVar = Cudd_Regular(bA)->index;

			/* find the parts(cofactors) of the area */
			bA0 = Cudd_NotCond( Cudd_E(bA), fIsComp );
			bA1 = Cudd_NotCond( Cudd_T(bA), fIsComp );

			/* solve for the Else part */
			bRes0 = extraZddConvertToBddAndAdd( dd, zC, bA0 );
			if ( bRes0 == NULL ) return NULL;
			cuddRef( bRes0 );

			/* solve for the Then part */
			bRes1 = extraZddConvertToBddAndAdd( dd, zC, bA1 );
			if ( bRes1 == NULL ) 
			{
				Cudd_RecursiveDeref( dd, bRes0 );
				return NULL;
			}
			cuddRef( bRes1 );
			/* only bRes0 and bRes1 are referenced at this point */
		}
		else /* if ( levCover <= levArea ) */
		{
			/* decompose the cover */
			DdNode *zC0, *zC1, *zC2;
			extraDecomposeCover(dd, zC, &zC0, &zC1, &zC2);

			/* assign the top-most BDD variable */
			TopBddVar = (zC->index >> 1);

			/* find the parts(cofactors) of the area */
			if ( levCover == levArea )
			{ /* find the parts(cofactors) of the area */
				bA0 = Cudd_NotCond( Cudd_E( bA ), fIsComp );
				bA1 = Cudd_NotCond( Cudd_T( bA ), fIsComp );
			}
			else /* if ( levCover < levArea ) */
			{ /* assign the cofactors */
				bA0 = bA1 = bA;
			}

			/* start the Else part of the solution */
			bRes0 = extraZddConvertToBddAndAdd(dd, zC0, bA0);
			if ( bRes0 == NULL )
				return NULL;
			cuddRef( bRes0 );

			/* start the Then part of the solution */
			bRes1 = extraZddConvertToBddAndAdd(dd, zC1, bA1);
			if ( bRes1 == NULL )
			{
				Cudd_RecursiveDeref(dd, bRes0);
				return NULL;
			}
			cuddRef( bRes1 );

			/* cover without literals converted into BDD */
			bRes2 = extraZddConvertToBdd(dd, zC2);
			if ( bRes2 == NULL ) 
			{
				Cudd_RecursiveDeref(dd, bRes0);
				Cudd_RecursiveDeref(dd, bRes1);
				return NULL;
			}
			cuddRef( bRes2 );

			/* add this BDD to the Else part */
			bTemp = bRes0;
			bRes0 = cuddBddAndRecur(dd, Cudd_Not(bRes0), Cudd_Not(bRes2));
			if ( bRes0 == NULL )
			{
				Cudd_RecursiveDeref(dd, bTemp);
				Cudd_RecursiveDeref(dd, bRes1);
				Cudd_RecursiveDeref(dd, bRes2);
				return NULL;
			}
			cuddRef( bRes0 );
			bRes0 = Cudd_Not(bRes0);
			Cudd_RecursiveDeref(dd, bTemp);

			/* add this BDD to the Then part */
			bTemp = bRes1;
			bRes1 = cuddBddAndRecur(dd, Cudd_Not(bRes1), Cudd_Not(bRes2));
			if ( bRes1 == NULL )
			{
				Cudd_RecursiveDeref(dd, bRes0);
				Cudd_RecursiveDeref(dd, bTemp);
				Cudd_RecursiveDeref(dd, bRes2);
				return NULL;
			}
			cuddRef( bRes1 );
			bRes1 = Cudd_Not(bRes1);
			Cudd_RecursiveDeref(dd, bTemp);
			Cudd_RecursiveDeref(dd, bRes2);
			/* only bRes0 and bRes1 are referenced at this point */
		}

		/* consider the case when Res0 and Res1 are the same node */
		if ( bRes0 == bRes1 )
			bRes = bRes1;
		/* consider the case when Res1 is complemented */
		else if ( Cudd_IsComplement(bRes1) ) 
		{
			bRes = cuddUniqueInter(dd, TopBddVar, Cudd_Not(bRes1), Cudd_Not(bRes0));
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes0);
				Cudd_RecursiveDeref(dd,bRes1);
				return NULL;
			}
			bRes = Cudd_Not(bRes);
		} 
		else 
		{
			bRes = cuddUniqueInter( dd, TopBddVar, bRes1, bRes0 );
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes0);
				Cudd_RecursiveDeref(dd,bRes1);
				return NULL;
			}
		}
		cuddDeref( bRes0 );
		cuddDeref( bRes1 );

		/* insert the result into cache */
		cuddCacheInsert2(dd, extraZddConvertToBddAndAdd, zC, bA, bRes);
		return bRes;
	}
} /* end of extraZddConvertToBddAndAdd */


/**Function********************************************************************

  Synopsis [Performs the recursive step of Cudd_zddSingleCoveredArea.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
extraZddSingleCoveredArea(
  DdManager * dd,
  DdNode * zC)
{
    DdNode	*bRes;
    statLine(dd); 

	/* if there is no cover, there is no BDD */
	if ( zC == z0 )  return b0;
	/* if the cover is the universe, the BDD is constant 1 */
	if ( zC == z1 )  return b1;

    /* check cache */
    bRes = cuddCacheLookup1(dd, extraZddSingleCoveredArea, zC);
    if (bRes)
		return(bRes);
	else
	{
		DdNode * bRes0, * bRes1, * bRes2, * bG0, * bG1;
		DdNode * zC0, * zC1, * zC2;
		int TopBddVar = (zC->index >> 1);

		/* cofactor the cover */
		extraDecomposeCover(dd, zC, &zC0, &zC1, &zC2);

		/* compute bdds for the three cofactors of the cover */
		bRes0 = extraZddSingleCoveredArea(dd, zC0);
		if ( bRes0 == NULL )
			return NULL;
		cuddRef( bRes0 );

		bRes1 = extraZddSingleCoveredArea(dd, zC1);
		if ( bRes1 == NULL )
		{
			Cudd_RecursiveDeref(dd, bRes0);
			return NULL;
		}
		cuddRef( bRes1 );

		bRes2 = extraZddSingleCoveredArea(dd, zC2);
		if ( bRes2 == NULL )
		{
			Cudd_RecursiveDeref(dd, bRes0);
			Cudd_RecursiveDeref(dd, bRes1);
			return NULL;
		}
		cuddRef( bRes2 );
		/* only bRes0, bRes1 and bRes2 are referenced at this point */

		/* bRes0 = bRes0 & !bdd(zC2) + bRes2 & !bdd(zC0) */
		bG0 = extraZddConvertToBddAndAdd( dd, zC2, Cudd_Not(bRes0) );
		if ( bG0 == NULL )
		{
			Cudd_IterDerefBdd(dd, bRes0);
			Cudd_IterDerefBdd(dd, bRes1);
			Cudd_IterDerefBdd(dd, bRes2);
			return NULL;
		}
		cuddRef( bG0 );
		Cudd_IterDerefBdd(dd, bRes0);

		bG1 = extraZddConvertToBddAndAdd( dd, zC0, Cudd_Not(bRes2) );
		if ( bG1 == NULL )
		{
			Cudd_IterDerefBdd(dd, bG0);
			Cudd_IterDerefBdd(dd, bRes1);
			Cudd_IterDerefBdd(dd, bRes2);
			return NULL;
		}
		cuddRef( bG1 );

		bRes0 = cuddBddAndRecur( dd, bG0, bG1 );
		if ( bRes0 == NULL )
		{
			Cudd_IterDerefBdd(dd, bG0);
			Cudd_IterDerefBdd(dd, bG1);
			Cudd_IterDerefBdd(dd, bRes1);
			Cudd_IterDerefBdd(dd, bRes2);
			return NULL;
		}
		cuddRef( bRes0 );
		bRes0 = Cudd_Not(bRes0);
		Cudd_IterDerefBdd(dd, bG0);
		Cudd_IterDerefBdd(dd, bG1);

		/* bRes1 = bRes1 & !bdd(zC2) + bRes2 & !bdd(zC1) */
		bG0 = extraZddConvertToBddAndAdd( dd, zC2, Cudd_Not(bRes1) );
		if ( bG0 == NULL )
		{
			Cudd_IterDerefBdd(dd, bRes0);
			Cudd_IterDerefBdd(dd, bRes1);
			Cudd_IterDerefBdd(dd, bRes2);
			return NULL;
		}
		cuddRef( bG0 );
		Cudd_IterDerefBdd(dd, bRes1);

		bG1 = extraZddConvertToBddAndAdd( dd, zC1, Cudd_Not(bRes2) );
		if ( bG1 == NULL )
		{
			Cudd_IterDerefBdd(dd, bG0);
			Cudd_IterDerefBdd(dd, bRes0);
			Cudd_IterDerefBdd(dd, bRes2);
			return NULL;
		}
		cuddRef( bG1 );
		Cudd_IterDerefBdd(dd, bRes2);

		bRes1 = cuddBddAndRecur( dd, bG0, bG1 );
		if ( bRes1 == NULL )
		{
			Cudd_IterDerefBdd(dd, bG0);
			Cudd_IterDerefBdd(dd, bG1);
			Cudd_IterDerefBdd(dd, bRes0);
			return NULL;
		}
		cuddRef( bRes1 );
		bRes1 = Cudd_Not(bRes1);
		Cudd_IterDerefBdd(dd, bG0);
		Cudd_IterDerefBdd(dd, bG1);

		/* only bRes0 and bRes1 are referenced at this point */

		/* consider the case when Res0 and Res1 are the same node */
		if ( bRes0 == bRes1 )
			bRes = bRes1;
		/* consider the case when Res1 is complemented */
		else if ( Cudd_IsComplement(bRes1) ) 
		{
			bRes = cuddUniqueInter(dd, TopBddVar, Cudd_Not(bRes1), Cudd_Not(bRes0));
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes0);
				Cudd_RecursiveDeref(dd,bRes1);
				return NULL;
			}
			bRes = Cudd_Not(bRes);
		} 
		else 
		{
			bRes = cuddUniqueInter( dd, TopBddVar, bRes1, bRes0 );
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes0);
				Cudd_RecursiveDeref(dd,bRes1);
				return NULL;
			}
		}
		cuddDeref( bRes0 );
		cuddDeref( bRes1 );

		/* insert the result into cache */
		cuddCacheInsert1(dd, extraZddSingleCoveredArea, zC, bRes);
		return bRes;
	}
} /* end of extraZddSingleCoveredArea */


/**Function********************************************************************

  Synopsis    [Finds three cofactors of the cover w.r.t. to the topmost variable.]

  Description [Finds three cofactors of the cover w.r.t. to the topmost variable.
  Does not check the cover for being a constant. Assumes that ZDD variables encoding 
  positive and negative polarities are adjacent in the variable order. Is different 
  from cuddZddGetCofactors3() in that it does not compute the cofactors w.r.t. the 
  given variable but takes the cofactors with respent to the topmost variable. 
  This function is more efficient when used in recursive procedures because it does 
  not require referencing of the resulting cofactors (compare cuddZddProduct() 
  and cuddZddPrimeProduct()).]

  SideEffects [None]

  SeeAlso     [cuddZddGetCofactors3]

******************************************************************************/

//void 
//extraDecomposeCover( 
//  DdManager* dd,    /* the manager */
//  DdNode*  zC,      /* the cover */
//  DdNode** zC0,     /* the pointer to the negative var cofactor */ 
//  DdNode** zC1,     /* the pointer to the positive var cofactor */ 
//  DdNode** zC2 )    /* the pointer to the cofactor without var */ 
//{
//	if ( (zC->index & 1) == 0 ) 
//	{ /* the top variable is present in positive polarity and maybe in negative */
//
//		DdNode *Temp = cuddE( zC );
//		*zC1  = cuddT( zC );
//		if ( cuddIZ(dd,Temp->index) == cuddIZ(dd,zC->index) + 1 )
//		{	/* Temp is not a terminal node 
//			 * top var is present in negative polarity */
//			*zC2 = cuddE( Temp );
//			*zC0 = cuddT( Temp );
//		}
//		else
//		{	/* top var is not present in negative polarity */
//			*zC2 = Temp;
//			*zC0 = dd->zero;
//		}
//	}
//	else 
//	{ /* the top variable is present only in negative */
//		*zC1 = dd->zero;
//		*zC2 = cuddE( zC );
//		*zC0 = cuddT( zC );
//	}
//} /* extraDecomposeCover */

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/
