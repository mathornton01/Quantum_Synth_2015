/**CFile***********************************************************************

  FileName    [aCoverStats.c]

  PackageName [extra]

  Synopsis    [Cover statistics implemented with ZDDs.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_zddGetMostCoveredArea()
				</ul>
			Internal procedures included in this module:
				<ul>
				<li> extraZddGetMostCoveredArea()
				</ul>
			Static procedures included in this module:
				<ul>
				</ul>
	      ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$zPerm.c, v.1.2, July 16, 2001, alanmi $]

******************************************************************************/

#include "util.h"
#include "cuddInt.h"
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

  Synopsis [Computes the BDD of the area covered by the max number of cubes in a ZDD.]

  Description [Given a cover represented as a ZDD, this function computes the
  area of the on-set where the largest number of cubes overlap. The optional argument
  nOverlaps, if it is non-NULL, contains the number of overlaps. Returns the 
  BDD of the area if successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode *
Extra_zddGetMostCoveredArea(
  DdManager * dd,
  DdNode * zCover,
  int * nOverlaps)
{
    DdNode *bRes, *aAreas, *aMaxNode;

    do {
		dd->reordered = 0;
		aAreas = extraZddGetMostCoveredArea(dd, zCover);
    } while (dd->reordered == 1);

	cuddRef( aAreas );

	/* get the maximal value area of this ADD */
	aMaxNode = Cudd_addFindMax( dd, aAreas );    Cudd_Ref( aMaxNode );

	if ( nOverlaps )
		*nOverlaps = (int)cuddV(aMaxNode);

	/* get the BDD representing the max value */
	bRes = Cudd_addBddThreshold( dd, aAreas, cuddV(aMaxNode) ); Cudd_Ref( bRes );

	/* dereference */
	Cudd_RecursiveDeref( dd, aMaxNode );
	Cudd_RecursiveDeref( dd, aAreas );
	cuddDeref( bRes );

    return(bRes);

} /* end of Extra_zddGetMostCoveredArea */

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis [Performs the recursive step of Extra_zddGetMostCoveredArea]

  Description [Computes the ADD, which gives for each minterm of the on-set of the
  function, the number of cubes covering this minterm. Returns the ADD 
  if successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode *
extraZddGetMostCoveredArea(
  DdManager * dd,
  DdNode * zC)
{
    DdNode	*aRes;
    statLine(dd); 

	/* if there is no cover, there are no covered minterms */
	if ( zC == z0 )  return dd->zero;
	/* if the cover is the universe, all minterms are covered once */
	if ( zC == z1 )  return dd->one;

    /* check cache */
    aRes = cuddCacheLookup1(dd, extraZddGetMostCoveredArea, zC);
    if (aRes)
		return(aRes);
	else
	{
		DdNode * aRes0, * aRes1, * aRes2, * aTemp;
		DdNode * zC0,   * zC1,   * zC2;
		int TopBddVar = (zC->index >> 1);

		/* cofactor the cover */
		extraDecomposeCover(dd, zC, &zC0, &zC1, &zC2);

		/* compute adds for the three cofactors of the cover */
		aRes0 = extraZddGetMostCoveredArea(dd, zC0);
		if ( aRes0 == NULL )
			return NULL;
		cuddRef( aRes0 );

		aRes1 = extraZddGetMostCoveredArea(dd, zC1);
		if ( aRes1 == NULL )
		{
			Cudd_RecursiveDeref(dd, aRes0);
			return NULL;
		}
		cuddRef( aRes1 );

		aRes2 = extraZddGetMostCoveredArea(dd, zC2);
		if ( aRes2 == NULL )
		{
			Cudd_RecursiveDeref(dd, aRes0);
			Cudd_RecursiveDeref(dd, aRes1);
			return NULL;
		}
		cuddRef( aRes2 );

		/* compute  add(zC0)+add(zC2) */
		aRes0 = cuddAddApplyRecur( dd, Cudd_addPlus, aTemp = aRes0, aRes2 );
		if ( aRes0 == NULL )
		{
			Cudd_RecursiveDeref(dd, aTemp);
			Cudd_RecursiveDeref(dd, aRes1);
			Cudd_RecursiveDeref(dd, aRes2);
			return NULL;
		}
		cuddRef( aRes0 );
		Cudd_RecursiveDeref(dd, aTemp);

		/* compute  add(zC1)+add(zC2) */
		aRes1 = cuddAddApplyRecur( dd, Cudd_addPlus, aTemp = aRes1, aRes2 );
		if ( aRes1 == NULL )
		{
			Cudd_RecursiveDeref(dd, aRes0);
			Cudd_RecursiveDeref(dd, aTemp);
			Cudd_RecursiveDeref(dd, aRes2);
			return NULL;
		}
		cuddRef( aRes1 );
		Cudd_RecursiveDeref(dd, aTemp);
		Cudd_RecursiveDeref(dd, aRes2);

		/* only aRes0 and aRes1 are referenced at this point */

		/* consider the case when Res0 and Res1 are the same node */
		aRes = (aRes1 == aRes0) ? aRes1 : cuddUniqueInter(dd,TopBddVar,aRes1,aRes0);
		if (aRes == NULL) 
		{
			Cudd_RecursiveDeref(dd, aRes1);
			Cudd_RecursiveDeref(dd, aRes0);
			return(NULL);
		}
		cuddDeref(aRes1);
		cuddDeref(aRes0);

		/* insert the result into cache */
		cuddCacheInsert1(dd, extraZddGetMostCoveredArea, zC, aRes);
		return aRes;
	}
} /* end of extraZddGetMostCoveredArea */
