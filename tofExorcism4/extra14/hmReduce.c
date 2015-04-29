/**CFile***********************************************************************

  FileName    [hmReduce.c]

  PackageName [Rondo Heuristic]

  Synopsis    [Heuristic SOP minimizer implementing Espresso strategy using ZDDs.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_zddReduce();
				</ul>
			Internal procedures included in this module:
				<ul>
				<li> extraZddReduce();
				</ul>
			Static procedures included in this module:
				<ul>
				</ul>
	      ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$hmReduce.c, v.1.2, January 16, 2001, alanmi $]

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

  Synopsis    [Shrinks cubes from the given set while preserving the area coverage.]

  Description [This function implements REDUCE operation of the heuristic algorithm 
  used in Espresso. The idea is to transform the cube set without increasing the 
  number of its cubes in such a way that each cube is as small as possible yet 
  the cube set still cover the given area (on-set of the function). Obviously,
  the result depends on the order in which the cubes are processed and the order 
  of variables chosen to be reduced. The current implementation of the function 
  solves the problem by preferring the left branch to the right branch and the upper 
  literals to the lower literals. Returns the reduced cube set on success; 
  NULL otherwise.]

  SideEffects [Fires assertion or produces meaningless result if the area is not 
  completely covered by the cover.]

  SeeAlso     [Extra_zddReduceThen]

******************************************************************************/
DdNode	*
Extra_zddReduce(
  DdManager * dd,
  DdNode * zC,
  DdNode * bA,
  int fTopFirst) /* flag which determines literals that are expanded first */
{
    DdNode	*res;

    do {
	dd->reordered = 0;
	res = extraZddReduce(dd, zC, bA, fTopFirst);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddReduce */


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis [Performs the recursive step of Extra_zddReduce.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
extraZddReduce(
  DdManager * dd,
  DdNode * zC,  /* the ZDD for the cover to be reduced */
  DdNode * bA,   /* the BDD for the area that should stay covered (on-set) */
  int fTopFirst)    /* flag which determines literals that are reduced first */
{
    DdNode	*zRes;
	DdNode*(*cacheOp)(DdManager*,DdNode*,DdNode*);
    statLine(dd); 

	/* if there is no area, the cover reduces to the empty cube set */
	if ( bA == b0 )   return z0;
	/* if the area is constant 1 (and the cover should be the universe), the universe is returned */
	if ( bA == b1 )   { /*assert( zC == z1 );*/ return z1; }
	/* if there is no cover (the area should be contant 0), nothing reduces to nothing */
	if ( zC == z0 )   { assert( bA == b0 ); return z0; }
	/* if the cover is the universe, return the smallest cube containing the area */
	if ( zC == z1 )   
	{
		DdNode * bIrrCover, * zIrrCover;
		DdNode * bCube = Cudd_FindEssential( dd, bA ); 
		cuddRef( bCube );
		
		if ( Cudd_bddIteConstant( dd, bA, bCube, dd->one ) != dd->one )
		{
			printf("Reduce(): Verification not okey: Cube does not cover the area\n");
			assert(0);
		}

        /* call the irredundant cover computation */
        bIrrCover = cuddZddIsop( dd, bCube, bCube, &zIrrCover );
        if ( bIrrCover == NULL )
		{
			Cudd_RecursiveDeref( dd, bCube );
			return NULL;
		}
        cuddRef( zIrrCover );
        cuddRef( bIrrCover );
        Cudd_RecursiveDeref( dd, bIrrCover );
        Cudd_RecursiveDeref( dd, bCube );

        cuddDeref( zIrrCover );
        return zIrrCover;
	}

    /* check cache */
	cacheOp = (DdNode*(*)(DdManager*,DdNode*,DdNode*))extraZddReduce;
    zRes = cuddCacheLookup2Zdd(dd, cacheOp, zC, bA);
    if (zRes)
		return zRes;
	else
	{
		DdNode *zC0, *zC1, *zC2, *zC2_0, *zC2_1;
		DdNode *bA0, *bA1, *bTemp;
		DdNode *bPA0, *bPA1, *bPA2, *zTemp;
		DdNode *zRes0, *zRes1, *zRes2;

		/* find the level in terms of the original variable levels, not ZDD variable levels */
		int levCover = dd->permZ[zC->index] >> 1;
		int levArea  = dd->perm[Cudd_Regular(bA)->index];
		/* determine the top ZDD variable */
		int TopZddVar = (levCover <= levArea)? ((zC->index>>1)<<1): ((Cudd_Regular(bA)->index)<<1);
		/* determine whether the area is a complemented BDD */
		int fIsComp  = Cudd_IsComplement( bA );

		/* cofactor the cover */
		if ( levCover <= levArea )
			extraDecomposeCover(dd, zC, &zC0, &zC1, &zC2);
		else
			zC0 = zC1 = z0, zC2 = zC;

		/* cofactor the area */
		if ( levCover >= levArea )
		{
			bA0 = Cudd_NotCond( Cudd_E( bA ), fIsComp );
			bA1 = Cudd_NotCond( Cudd_T( bA ), fIsComp );
		}
		else
			bA0 = bA1 = bA;

		if ( fTopFirst )
		{ // reducing the current literal first   
		  /*
			Cover is cofactored into C0, C1, C2
			Area is cofactored into A0 and A1

			// find the cubes in C2 (wo this var) that can be reduced to have the negative literal
			// it means that the positive part of their cubes 
			   * either does not cover anything useful (is outside A1)
			   * or is overlapping with C1
			C2_0 = Contained( C2, !A1 + bdd(C1) )
			// update the subcovers
			R2 = C2 - C2_0
			R0 = C0 + C2_0

			// find the cubes in C2 (wo this var) that can be reduced to have the positive literal
			// it means that the negative part of their cubes 
			   * either does not cover anything useful (is outside A0)
			   * or is overlapping with C0
			C2_1 = Contained( R2, !A0 + bdd(R0) )
			// update the subcovers
  			R2 = R2 - C2_1
			R1 = C1 + C2_1

			// since we have added to R1(C1), theoretically, it is possible to repeat these steps again
			// but it may be not practical
			// also, these two steps can be reversed

			// several reduction orders are possible
			// reduction order R2, R0, R1
				// find the critical part of A w.r.t. R2 (the part that can be covered only by C2)
				A2 = (A0 - bdd(R0)) + (A1 - bdd(R1))
				// cover this part
				R2 = Reduce( R2, A2 )

				// find the critical part of A w.r.t. R0 (the part that can be covered only by C0)
				A0c = A0 - bdd(R2)
				// cover this part
				R0 = Reduce( R0, A0c )

				// find the remaining part to be covered
				A1c = A1 - bdd(R2)
				// cover this part
				R1 = Reduce( R1, A1c )

			// reduction order R0, R1, R2
				// find the critical part of A w.r.t. R0 (the part that can be covered only by C0)
				A0c = A0 - bdd(R2)
				// cover this part
				R0 = Reduce( R0, A0c )

				// find the critical part of A w.r.t. R1 (the part that can be covered only by C1)
				A1c = A1 - bdd(R2)
				// cover this part
				R1 = Reduce( R1, A1c )

				// find the remaining part to be covered
				A2 = (A0 - bdd(R0)) + (A1 - bdd(R1))
				// cover this part
				R1 = Reduce( R2, A2 )

			// compose the result
			R = ZDD( R0, R1, R2 )
			*/

			/* reducing the current variable */

			/* C2_0 = Contained( C2, !A1 + bdd(C1) ) */
			bTemp = extraZddConvertToBddAndAdd( dd, zC1, Cudd_Not(bA1) );
			if ( bTemp == NULL ) return NULL;
			cuddRef( bTemp );

			zC2_0 = extraZddCoveredByArea( dd, zC2, bTemp ); 
			if ( zC2_0 == NULL )
			{
				Cudd_RecursiveDeref( dd, bTemp );
				return NULL;
			}
			cuddRef( zC2_0 );
			Cudd_RecursiveDeref( dd, bTemp );

			/* R2 = C2 - C2_0 */
			/* R0 = C0 + C2_0 */
			zRes2 = cuddZddDiff( dd, zC2, zC2_0 ); 
			if ( zRes2 == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zC2_0 );
				return NULL;
			}
			cuddRef( zRes2 );

			zRes0 = cuddZddUnion( dd, zC0, zC2_0 ); 
			if ( zRes0 == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zC2_0 );
				Cudd_RecursiveDerefZdd( dd, zRes2 );
				return NULL;
			}
			cuddRef( zRes0 );
			Cudd_RecursiveDerefZdd( dd, zC2_0 );
			/* only zRes0, zRes2 are referenced at this point */

			/* C2_1 = Contained( R2, !A0 + bdd(R0) ) */
			bTemp = extraZddConvertToBddAndAdd( dd, zRes0, Cudd_Not(bA0) );
			if ( bTemp == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes2 );
				return NULL;
			}
			cuddRef( bTemp );

			zC2_1 = extraZddCoveredByArea( dd, zRes2, bTemp ); 
			if ( zC2_1 == NULL )
			{
				Cudd_RecursiveDeref( dd, bTemp );
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes2 );
				return NULL;
			}
			cuddRef( zC2_1 );
			Cudd_RecursiveDeref( dd, bTemp );

			/* R2 = R2 - C2_1 */
			/* R1 = C1 + C2_1 */
			zRes2 = cuddZddDiff( dd, zTemp = zRes2, zC2_1 ); 
			if ( zRes2 == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zC2_1 );
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zTemp );
				return NULL;
			}
			cuddRef( zRes2 );
			Cudd_RecursiveDerefZdd( dd, zTemp );

			zRes1 = cuddZddUnion( dd, zC1, zC2_1 ); 
			if ( zRes1 == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zC2_1 );
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes2 );
				return NULL;
			}
			cuddRef( zRes1 );
			Cudd_RecursiveDerefZdd( dd, zC2_1 );
			/* only zRes0, zRes1, zRes2 are referenced at this point */

			/* reducing other variables */

			/* A2 = (A0 - bdd(R0)) + (A1 - bdd(R1)) */
			bPA0 = extraZddConvertToBddAndAdd( dd, zRes0, Cudd_Not(bA0) );
			if ( bPA0 == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				Cudd_RecursiveDerefZdd( dd, zRes2 );
				return NULL;
			}
			cuddRef( bPA0 );

			bPA1 = extraZddConvertToBddAndAdd( dd, zRes1, Cudd_Not(bA1) );
			if ( bPA1 == NULL )
			{
				Cudd_RecursiveDeref( dd, bPA0 );
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				Cudd_RecursiveDerefZdd( dd, zRes2 );
				return NULL;
			}
			cuddRef( bPA1 );

			bPA2 = cuddBddAndRecur( dd, bPA0, bPA1 );
			if ( bPA2 == NULL )
			{
				Cudd_RecursiveDeref( dd, bPA0 );
				Cudd_RecursiveDeref( dd, bPA1 );
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				Cudd_RecursiveDerefZdd( dd, zRes2 );
				return NULL;
			}
			cuddRef( bPA2 );
			Cudd_RecursiveDeref( dd, bPA0 );
			Cudd_RecursiveDeref( dd, bPA1 );


			/* R2 = Reduce( R2, A2 ) */
			zRes2 = extraZddReduce( dd, zTemp = zRes2, Cudd_Not(bPA2), fTopFirst );
			if ( zRes2 == NULL )
			{
				Cudd_RecursiveDeref(dd, bPA2);
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				Cudd_RecursiveDerefZdd( dd, zTemp );
				return NULL;
			}
			cuddRef( zRes2 );
			Cudd_RecursiveDerefZdd( dd, zTemp );
			Cudd_RecursiveDeref( dd, bPA2 );


			/* A0c = A0 - bdd(R2) */
			bPA0 = extraZddConvertToBddAndAdd( dd, zRes2, Cudd_Not(bA0) );
			if ( bPA0 == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				Cudd_RecursiveDerefZdd( dd, zRes2 );
				return NULL;
			}
			cuddRef( bPA0 );

			/* R0 = Reduce( R0, A0c ) */
			zRes0 = extraZddReduce( dd, zTemp = zRes0, Cudd_Not(bPA0), fTopFirst );
			if ( zRes0 == NULL )
			{
				Cudd_RecursiveDeref(dd, bPA0);
				Cudd_RecursiveDerefZdd( dd, zTemp );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				Cudd_RecursiveDerefZdd( dd, zRes2 );
				return NULL;
			}
			cuddRef( zRes0 );
			Cudd_RecursiveDerefZdd( dd, zTemp );
			Cudd_RecursiveDeref( dd, bPA0 );


			/* A1c = A1 - bdd(R2) */
			bPA1 = extraZddConvertToBddAndAdd( dd, zRes2, Cudd_Not(bA1) );
			if ( bPA1 == NULL )
			{
				Cudd_RecursiveDeref( dd, bPA1 );
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				Cudd_RecursiveDerefZdd( dd, zRes2 );
				return NULL;
			}
			cuddRef( bPA1 );

			/* R1 = Reduce( R1, A1c ) */
			zRes1 = extraZddReduce( dd, zTemp = zRes1, Cudd_Not(bPA1), fTopFirst );
			if ( zRes1 == NULL )
			{
				Cudd_RecursiveDeref(dd, bPA1);
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zTemp );
				Cudd_RecursiveDerefZdd( dd, zRes2 );
				return NULL;
			}
			cuddRef( zRes1 );
			Cudd_RecursiveDerefZdd( dd, zTemp );
			Cudd_RecursiveDeref( dd, bPA1 );
			/* only zRes0, zRes1, zRes2 are referenced at this point */
		}
		else
		{ // reducing other literals first

			
			/* reducing other variables */

			/* A2 = (A0 - bdd(R0)) + (A1 - bdd(R1)) */
			bPA0 = extraZddConvertToBddAndAdd( dd, zC0, Cudd_Not(bA0) );
			if ( bPA0 == NULL )
				return NULL;
			cuddRef( bPA0 );

			bPA1 = extraZddConvertToBddAndAdd( dd, zC1, Cudd_Not(bA1) );
			if ( bPA1 == NULL )
			{
				Cudd_RecursiveDeref( dd, bPA0 );
				return NULL;
			}
			cuddRef( bPA1 );

			bPA2 = cuddBddAndRecur( dd, bPA0, bPA1 );
			if ( bPA2 == NULL )
			{
				Cudd_RecursiveDeref( dd, bPA0 );
				Cudd_RecursiveDeref( dd, bPA1 );
				return NULL;
			}
			cuddRef( bPA2 );
			Cudd_RecursiveDeref( dd, bPA0 );
			Cudd_RecursiveDeref( dd, bPA1 );


			/* R2 = Reduce( R2, A2 ) */
			zRes2 = extraZddReduce( dd, zC2, Cudd_Not(bPA2), fTopFirst );
			if ( zRes2 == NULL )
			{
				Cudd_RecursiveDeref(dd, bPA2);
				return NULL;
			}
			cuddRef( zRes2 );
			Cudd_RecursiveDeref( dd, bPA2 );


			/* A0c = A0 - bdd(R2) */
			bPA0 = extraZddConvertToBddAndAdd( dd, zRes2, Cudd_Not(bA0) );
			if ( bPA0 == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zRes2 );
				return NULL;
			}
			cuddRef( bPA0 );

			/* R0 = Reduce( R0, A0c ) */
			zRes0 = extraZddReduce( dd, zC0, Cudd_Not(bPA0), fTopFirst );
			if ( zRes0 == NULL )
			{
				Cudd_RecursiveDeref(dd, bPA0);
				Cudd_RecursiveDerefZdd( dd, zRes2 );
				return NULL;
			}
			cuddRef( zRes0 );
			Cudd_RecursiveDeref( dd, bPA0 );


			/* A1c = A1 - bdd(R2) */
			bPA1 = extraZddConvertToBddAndAdd( dd, zRes2, Cudd_Not(bA1) );
			if ( bPA1 == NULL )
			{
				Cudd_RecursiveDeref( dd, bPA1 );
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes2 );
				return NULL;
			}
			cuddRef( bPA1 );

			/* R1 = Reduce( R1, A1c ) */
			zRes1 = extraZddReduce( dd, zC1, Cudd_Not(bPA1), fTopFirst );
			if ( zRes1 == NULL )
			{
				Cudd_RecursiveDeref(dd, bPA1);
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes2 );
				return NULL;
			}
			cuddRef( zRes1 );
			Cudd_RecursiveDeref( dd, bPA1 );
			/* only zRes0, zRes1, zRes2 are referenced at this point */

			/* reducing the current variable */

			/* C2_0 = Contained( C2, !A1 + bdd(C1) ) */
			bTemp = extraZddConvertToBddAndAdd( dd, zRes1, Cudd_Not(bA1) );
			if ( bTemp == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				Cudd_RecursiveDerefZdd( dd, zRes2 );
				return NULL;
			}
			cuddRef( bTemp );

			zC2_0 = extraZddCoveredByArea( dd, zRes2, bTemp ); 
			if ( zC2_0 == NULL )
			{
				Cudd_RecursiveDeref( dd, bTemp );
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				Cudd_RecursiveDerefZdd( dd, zRes2 );
				return NULL;
			}
			cuddRef( zC2_0 );
			Cudd_RecursiveDeref( dd, bTemp );

			/* R2 = C2 - C2_0 */
			/* R0 = C0 + C2_0 */
			zRes2 = cuddZddDiff( dd, zTemp = zRes2, zC2_0 ); 
			if ( zRes2 == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zC2_0 );
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				Cudd_RecursiveDerefZdd( dd, zTemp );
				return NULL;
			}
			cuddRef( zRes2 );
			Cudd_RecursiveDerefZdd( dd, zTemp );

			zRes0 = cuddZddUnion( dd, zTemp = zRes0, zC2_0 ); 
			if ( zRes0 == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zC2_0 );
				Cudd_RecursiveDerefZdd( dd, zTemp );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				Cudd_RecursiveDerefZdd( dd, zRes2 );
				return NULL;
			}
			cuddRef( zRes0 );
			Cudd_RecursiveDerefZdd( dd, zC2_0 );
			Cudd_RecursiveDerefZdd( dd, zTemp );


			/* C2_1 = Contained( R2, !A0 + bdd(R0) ) */
			bTemp = extraZddConvertToBddAndAdd( dd, zRes0, Cudd_Not(bA0) );
			if ( bTemp == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				Cudd_RecursiveDerefZdd( dd, zRes2 );
				return NULL;
			}
			cuddRef( bTemp );

			zC2_1 = extraZddCoveredByArea( dd, zRes2, bTemp ); 
			if ( zC2_1 == NULL )
			{
				Cudd_RecursiveDeref( dd, bTemp );
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				Cudd_RecursiveDerefZdd( dd, zRes2 );
				return NULL;
			}
			cuddRef( zC2_1 );
			Cudd_RecursiveDeref( dd, bTemp );

			/* R2 = R2 - C2_1 */
			/* R1 = C1 + C2_1 */
			zRes2 = cuddZddDiff( dd, zTemp = zRes2, zC2_1 ); 
			if ( zRes2 == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zC2_1 );
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				Cudd_RecursiveDerefZdd( dd, zTemp );
				return NULL;
			}
			cuddRef( zRes2 );
			Cudd_RecursiveDerefZdd( dd, zTemp );

			zRes1 = cuddZddUnion( dd, zTemp = zRes1, zC2_1 ); 
			if ( zRes1 == NULL )
			{
				Cudd_RecursiveDerefZdd( dd, zC2_1 );
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zTemp );
				Cudd_RecursiveDerefZdd( dd, zRes2 );
				return NULL;
			}
			cuddRef( zRes1 );
			Cudd_RecursiveDerefZdd( dd, zC2_1 );
			Cudd_RecursiveDerefZdd( dd, zTemp );
			/* only zRes0, zRes1, zRes2 are referenced at this point */
		}


		/* --------------- compose the result ------------------ */
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

		{
			DdNode * bTempC, * bTempRes;

			cuddRef( zRes );

			bTempC = Cudd_MakeBddFromZddCover( dd, zC );
			Cudd_Ref( bTempC );

			bTempRes = Cudd_MakeBddFromZddCover( dd, zRes );
			Cudd_Ref( bTempRes );
			
			if ( Cudd_bddIteConstant( dd, bA, bTempRes, dd->one ) != dd->one )
			{
				printf("Reduce(): Verification not okey: Solution does not cover the on-set\n");
				assert(0);
			}
			if ( Cudd_bddIteConstant( dd, bTempRes, bTempC, dd->one ) != dd->one )
			{
				printf("Reduce(): Verification not okey: Solution overlaps with the off-set\n");
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
} /* end of extraZddReduce */



/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/
