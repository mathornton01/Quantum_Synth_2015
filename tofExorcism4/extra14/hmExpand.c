/**CFile***********************************************************************

  FileName    [hmExpand.c]

  PackageName [Rondo Heuristic]

  Synopsis    [Heuristic SOP minimizer implementing Espresso strategy using ZDDs.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_zddExpand();
				</ul>
			Internal procedures included in this module:
				<ul>
				<li> extraZddExpand();
				</ul>
			Static procedures included in this module:
				<ul>
				</ul>
	      ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$hmExpand.c, v.1.2, January 16, 2001, alanmi $]

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

  Synopsis    [Expands cubes from the given set without exceeding the given area.]

  Description [This function implements EXPAND operation of the heuristic algorithm 
  used in Espresso. The idea is to transform the cover without increasing the 
  number of its cubes in such a way that each cube is as large as possible, 
  yet the cover is within the given area (on-set plus dc-set of the function). 
  The contained cubes are removed from the cover. Obviously, the result depends 
  on the order in which the cubes are processed and the order in which variables 
  are chosen to be reduced. The function solves the problem by first expanding
  those cubes that are not likely to be contained by other cubes. Returns the 
  expanded cube set on success; NULL otherwise.]

  SideEffects [Fires assertion or produces meaningless result if the area does 
  not cover the cover.]

  SeeAlso     [Extra_zddReduceThen]

******************************************************************************/
DdNode	*
Extra_zddExpand(
  DdManager * dd,
  DdNode * zCover,
  DdNode * bA,
  int fTopFirst) /* flag which determines literals that are expanded first */
{
    DdNode	*res;

    do {
	dd->reordered = 0;
	res = extraZddExpand(dd, zCover, bA, fTopFirst);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddExpand */


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis [Performs the recursive step of Extra_zddExpand.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
extraZddExpand(
  DdManager * dd,
  DdNode * zC,   /* the ZDD for the cover to be expanded */
  DdNode * bA,   /* the BDD for the area within which to stay (on-set + dc-set) */
  int fTopFirst) /* flag which determines literals that are expanded first */
{
    DdNode	*zRes;
	DdNode*(*cacheOp)(DdManager*,DdNode*,DdNode*);
    statLine(dd); 

	/* if there is no area (the cover should be empty), there is nowhere to expand */
	if ( bA == b0 )    { assert( zC == z0 ); return z0; }
	/* if the area is the total boolean space, the cover is expanded into a single large cube */
	if ( bA == b1 )    return z1;
	/* if there is no cover, nothing is expanded */
	if ( zC == z0 )    return z0;
	/* if the cover is the universe (the area is the total boolean space), the universe is returned */
	if ( zC == z1 )    { assert( bA == b1 ); return z1; }

    /* check cache */
	cacheOp = (DdNode*(*)(DdManager*,DdNode*,DdNode*))extraZddExpand;
    zRes = cuddCacheLookup2Zdd(dd, cacheOp, zC, bA);
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
			/* if the cover does not depend on this literal, 
			   there is no way to expand it w.r.t. this literal */

			/* find the parts(cofactors) of the area */
			bA0 = Cudd_NotCond( Cudd_E( bA ), fIsComp );
			bA1 = Cudd_NotCond( Cudd_T( bA ), fIsComp );

			/* find the intersection of cofactors */
			bA01 = cuddBddAndRecur( dd, bA0, bA1 );
			if ( bA01 == NULL ) return NULL;
			cuddRef( bA01 );

			/* the expansion can only continue within the intersection */
			zRes = extraZddExpand( dd, zC, bA01, fTopFirst );
			if ( zRes == NULL ) return NULL;
			cuddRef( zRes );
			Cudd_RecursiveDeref( dd, bA01 );
			cuddDeref( zRes );
		}
		else /* if ( levCover <= levArea ) */
		{
			DdNode *zRes0, *zRes1, *zRes2, *zTemp;
			DdNode *zCont0, *zCont1;
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

			if ( fTopFirst )
			{				
				/* next, allow the sub-covers to expand w.r.t. the current variable */

				/* find the cubes of zRes0 contained in bA1 (they can go without literal) */
				zCont0 = extraZddCoveredByArea( dd, zC0, bA1 ); 
				if ( zCont0 == NULL )
					return NULL;
				cuddRef( zCont0 );

				/* find the cubes of zRes1 contained in bA0 (they can go without literal) */
				zCont1 = extraZddCoveredByArea( dd, zC1, bA0 ); 
				if ( zCont1 == NULL )
				{
					Cudd_RecursiveDerefZdd(dd, zCont0);
					return NULL;
				}
				cuddRef( zCont1 );

				/* subtract the cubes in zCont0 from sub-cover zRes0 */
				zRes0 = cuddZddDiff( dd, zC0, zCont0 ); 
				if ( zRes0 == NULL )
				{
					Cudd_RecursiveDerefZdd(dd, zCont0);
					Cudd_RecursiveDerefZdd(dd, zCont1);
					return NULL;
				}
				cuddRef( zRes0 );

				/* subtract the cubes in zCont1 from sub-cover zRes1 */
				zRes1 = cuddZddDiff( dd, zC1, zCont1 ); 
				if ( zRes1 == NULL )
				{
					Cudd_RecursiveDerefZdd(dd, zCont0);
					Cudd_RecursiveDerefZdd(dd, zCont1);
					Cudd_RecursiveDerefZdd(dd, zRes0);
					return NULL;
				}
				cuddRef( zRes1 );

				/* add the cubes in zCont0 to the sub-cover without literal zRes2 */
				zRes2 = cuddZddUnion( dd, zC2, zCont0 ); 
				if ( zRes2 == NULL )
				{
					Cudd_RecursiveDerefZdd(dd, zCont0);
					Cudd_RecursiveDerefZdd(dd, zCont1);
					Cudd_RecursiveDerefZdd(dd, zRes0);
					Cudd_RecursiveDerefZdd(dd, zRes1);
					return NULL;
				}
				cuddRef( zRes2 );
				Cudd_RecursiveDerefZdd(dd, zCont0);

				/* add the cubes in zCont1 to the sub-cover without literal zRes2 */
				zRes2 = cuddZddUnion( dd, zTemp = zRes2, zCont1 ); 
				if ( zRes2 == NULL )
				{
					Cudd_RecursiveDerefZdd(dd, zCont1);
					Cudd_RecursiveDerefZdd(dd, zRes0);
					Cudd_RecursiveDerefZdd(dd, zRes1);
					Cudd_RecursiveDerefZdd(dd, zTemp);
					return NULL;
				}
				cuddRef( zRes2 );
				Cudd_RecursiveDerefZdd(dd, zTemp);
				Cudd_RecursiveDerefZdd(dd, zCont1);
				/* only zRes0, zRes1, zRes2 are referenced at this point */

				/* now, expand the sub-covers w.r.t. lower literals */

				
				/* expand the cover without literals in the intersection */
				zRes2 = extraZddExpand( dd, zTemp = zRes2, bA01, fTopFirst );
				if ( zRes2 == NULL )
				{
					Cudd_RecursiveDeref( dd, bA01 );
					Cudd_RecursiveDerefZdd(dd, zRes0);
					Cudd_RecursiveDerefZdd(dd, zRes1);
					Cudd_RecursiveDerefZdd(dd, zTemp);
					return NULL;
				}
				cuddRef( zRes2 );
				Cudd_RecursiveDeref( dd, bA01 );
				Cudd_RecursiveDerefZdd(dd, zTemp);



				/* remove cubes from zC0 which are covered by zRes2 */
				zRes0 = extraZddNotCoveredByCover( dd, zTemp = zRes0, zRes2 );
				if ( zRes0 == NULL )
				{
					Cudd_RecursiveDerefZdd(dd, zTemp);
					Cudd_RecursiveDerefZdd(dd, zRes1);
					Cudd_RecursiveDerefZdd(dd, zRes2);
					return NULL;
				}
				cuddRef( zRes0 );
				Cudd_RecursiveDerefZdd(dd, zTemp);

				/* expand the cover with the negative literal in bA0 */
				zRes0 = extraZddExpand( dd, zTemp = zRes0, bA0, fTopFirst );
				if ( zRes0 == NULL )
				{
					Cudd_RecursiveDerefZdd(dd, zTemp);
					Cudd_RecursiveDerefZdd(dd, zRes1);
					Cudd_RecursiveDerefZdd(dd, zRes2);
					return NULL;
				}
				cuddRef( zRes0 );
				Cudd_RecursiveDerefZdd(dd, zTemp);


				/* remove cubes from zC1 which are covered by the union */
				zRes1 = extraZddNotCoveredByCover( dd, zTemp = zRes1, zRes2 );
				if ( zRes1 == NULL )
				{
					Cudd_RecursiveDerefZdd(dd, zRes0);
					Cudd_RecursiveDerefZdd(dd, zTemp);
					Cudd_RecursiveDerefZdd(dd, zRes2);
					return NULL;
				}
				cuddRef( zRes1 );
				Cudd_RecursiveDerefZdd(dd, zTemp);
				/* only zRes0, zRes1, zRes2 are referenced at this point */

				/* expand the cover with the positive literal in bA1 */
				zRes1 = extraZddExpand( dd, zTemp = zRes1, bA1, fTopFirst );
				if ( zRes1 == NULL )
				{
					Cudd_RecursiveDerefZdd(dd, zRes0);
					Cudd_RecursiveDerefZdd(dd, zTemp);
					Cudd_RecursiveDerefZdd(dd, zRes2);
					return NULL;
				}
				cuddRef( zRes1 );
				Cudd_RecursiveDerefZdd(dd, zTemp);
				/* only zRes0, zRes1, zRes2 are referenced at this point */
			}
			else
			{

				/* first, expand the sub-covers w.r.t. lower literals */

				/* expand the cover without literals in the intersection */
				zRes2 = extraZddExpand( dd, zC2, bA01, fTopFirst );
				if ( zRes2 == NULL )
				{
					Cudd_RecursiveDeref( dd, bA01 );
					return NULL;
				}
				cuddRef( zRes2 );
				Cudd_RecursiveDeref( dd, bA01 );



				/* remove cubes from zC0 which are covered by zRes2 */
				zRes0 = extraZddNotCoveredByCover( dd, zC0, zRes2 );
				if ( zRes0 == NULL )
				{
					Cudd_RecursiveDerefZdd(dd, zRes2);
					return NULL;
				}
				cuddRef( zRes0 );

				/* expand the cover with the negative literal in bA0 */
				zRes0 = extraZddExpand( dd, zTemp = zRes0, bA0, fTopFirst );
				if ( zRes0 == NULL )
				{
					Cudd_RecursiveDerefZdd(dd, zTemp);
					Cudd_RecursiveDerefZdd(dd, zRes2);
					return NULL;
				}
				cuddRef( zRes0 );
				Cudd_RecursiveDerefZdd(dd, zTemp);


				/* remove cubes from zC1 which are covered by the union */
				zRes1 = extraZddNotCoveredByCover( dd, zC1, zRes2 );
				if ( zRes1 == NULL )
				{
					Cudd_RecursiveDerefZdd(dd, zRes0);
					Cudd_RecursiveDerefZdd(dd, zRes2);
					return NULL;
				}
				cuddRef( zRes1 );
				/* only zRes0, zRes1, zRes2 are referenced at this point */

				/* expand the cover with the positive literal in bA1 */
				zRes1 = extraZddExpand( dd, zTemp = zRes1, bA1, fTopFirst );
				if ( zRes1 == NULL )
				{
					Cudd_RecursiveDerefZdd(dd, zRes0);
					Cudd_RecursiveDerefZdd(dd, zTemp);
					Cudd_RecursiveDerefZdd(dd, zRes2);
					return NULL;
				}
				cuddRef( zRes1 );
				Cudd_RecursiveDerefZdd(dd, zTemp);
				/* only zRes0, zRes1, zRes2 are referenced at this point */


				/* next, allow the sub-covers to expand w.r.t. the current variable */

				/* find the cubes of zRes0 contained in bA1 (they can go without literal) */
				zCont0 = extraZddCoveredByArea( dd, zRes0, bA1 ); 
				if ( zCont0 == NULL )
				{
					Cudd_RecursiveDerefZdd(dd, zRes0);
					Cudd_RecursiveDerefZdd(dd, zRes1);
					Cudd_RecursiveDerefZdd(dd, zRes2);
					return NULL;
				}
				cuddRef( zCont0 );

				/* find the cubes of zRes1 contained in bA0 (they can go without literal) */
				zCont1 = extraZddCoveredByArea( dd, zRes1, bA0 ); 
				if ( zCont1 == NULL )
				{
					Cudd_RecursiveDerefZdd(dd, zCont0);
					Cudd_RecursiveDerefZdd(dd, zRes0);
					Cudd_RecursiveDerefZdd(dd, zRes1);
					Cudd_RecursiveDerefZdd(dd, zRes2);
					return NULL;
				}
				cuddRef( zCont1 );

				/* subtract the cubes in zCont0 from sub-cover zRes0 */
				zRes0 = cuddZddDiff( dd, zTemp = zRes0, zCont0 ); 
				if ( zRes0 == NULL )
				{
					Cudd_RecursiveDerefZdd(dd, zCont0);
					Cudd_RecursiveDerefZdd(dd, zCont1);
					Cudd_RecursiveDerefZdd(dd, zTemp);
					Cudd_RecursiveDerefZdd(dd, zRes1);
					Cudd_RecursiveDerefZdd(dd, zRes2);
					return NULL;
				}
				cuddRef( zRes0 );
				Cudd_RecursiveDerefZdd(dd, zTemp);

				/* subtract the cubes in zCont1 from sub-cover zRes1 */
				zRes1 = cuddZddDiff( dd, zTemp = zRes1, zCont1 ); 
				if ( zRes1 == NULL )
				{
					Cudd_RecursiveDerefZdd(dd, zCont0);
					Cudd_RecursiveDerefZdd(dd, zCont1);
					Cudd_RecursiveDerefZdd(dd, zRes0);
					Cudd_RecursiveDerefZdd(dd, zTemp);
					Cudd_RecursiveDerefZdd(dd, zRes2);
					return NULL;
				}
				cuddRef( zRes1 );
				Cudd_RecursiveDerefZdd(dd, zTemp);

				/* add the cubes in zCont0 to the sub-cover without literal zRes2 */
				zRes2 = cuddZddUnion( dd, zTemp = zRes2, zCont0 ); 
				if ( zRes2 == NULL )
				{
					Cudd_RecursiveDerefZdd(dd, zCont0);
					Cudd_RecursiveDerefZdd(dd, zCont1);
					Cudd_RecursiveDerefZdd(dd, zRes0);
					Cudd_RecursiveDerefZdd(dd, zRes1);
					Cudd_RecursiveDerefZdd(dd, zTemp);
					return NULL;
				}
				cuddRef( zRes2 );
				Cudd_RecursiveDerefZdd(dd, zTemp);
				Cudd_RecursiveDerefZdd(dd, zCont0);

				/* add the cubes in zCont1 to the sub-cover without literal zRes2 */
				zRes2 = cuddZddUnion( dd, zTemp = zRes2, zCont1 ); 
				if ( zRes2 == NULL )
				{
					Cudd_RecursiveDerefZdd(dd, zCont1);
					Cudd_RecursiveDerefZdd(dd, zRes0);
					Cudd_RecursiveDerefZdd(dd, zRes1);
					Cudd_RecursiveDerefZdd(dd, zTemp);
					return NULL;
				}
				cuddRef( zRes2 );
				Cudd_RecursiveDerefZdd(dd, zTemp);
				Cudd_RecursiveDerefZdd(dd, zCont1);
				/* only zRes0, zRes1, zRes2 are referenced at this point */
			}


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

		{
			DdNode * bTempC, * bTempRes;

			cuddRef( zRes );

			bTempC = Cudd_MakeBddFromZddCover( dd, zC );
			Cudd_Ref( bTempC );

			bTempRes = Cudd_MakeBddFromZddCover( dd, zRes );
			Cudd_Ref( bTempRes );
			
			if ( Cudd_bddIteConstant( dd, bTempC, bTempRes, dd->one ) != dd->one )
			{
				printf("Expand(): Verification not okey: Solution does not cover the on-set\n");
				assert(0);
			}
			if ( Cudd_bddIteConstant( dd, bTempRes, bA, dd->one ) != dd->one )
			{
				printf("Expand(): Verification not okey: Solution overlaps with the off-set\n");
				assert(0);
			}

			Cudd_RecursiveDeref(dd, bTempC);
			Cudd_RecursiveDeref(dd, bTempRes);

			cuddDeref( zRes );
		}

		/* insert the result into cache and return */
		cuddCacheInsert2(dd, cacheOp, zC, bA, zRes);
		return zRes;
	}
} /* end of extraZddExpand */



/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/
