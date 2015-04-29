
/**CFile***********************************************************************

  FileName    [*.c]

  PackageName [extra]

  Synopsis    [The module description.]

  Description [External procedures included in this module:
				<ul>
				<li> Function1();
				</ul>
			Internal procedures included in this module:
				<ul>
				<li> Function2();
				</ul>
			Static procedures included in this module:
				<ul>
				<li> Function3();
				</ul>
	      ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$*.c, v.1.2, September 4, 2001, alanmi $]

******************************************************************************/

#include "util.h"
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


/**Automaticstart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/**Automaticend***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Checks unateness of one variable.]

  Description [Returns b0(b1) if iVar is negative(positive) unate, dd->vars[0]
  if iVars is not unate, dd->vars[1] if it is both neg and pos unate (in other 
  words, if it does not depend on iVar).]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_bddCheckVarUnate( 
  DdManager * dd,   /* the DD manager */
  DdNode * bF,
  int iVar) 
{
	// we need at least two variables for caching return values
	assert( dd->size >= 2 );
	assert( iVar < dd->size );
	return extraBddCheckVarUnate( dd, bF, dd->vars[iVar] );
} /* end of Extra_bddCheckVarUnate */

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/
/**Function********************************************************************

  Synopsis    [Performs the recursive step of Extra_bddCheckVarUnate().]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * extraBddCheckVarUnate( 
  DdManager * dd,    /* the DD manager */
  DdNode * bF,
  DdNode * bVar) 
{
	DdNode * bRes;
	DdNode * bFR = Cudd_Regular(bF); 
	int LevelF = cuddI(dd,bFR->index);
	int LevelV = dd->perm[bVar->index];

	if ( LevelF > LevelV )
		return dd->vars[1]; // can be both neg/pos unate

    if ( bRes = cuddCacheLookup2(dd, extraBddCheckVarUnate, bF, bVar) )
    	return bRes;
	else
	{
		DdNode * bF0, * bF1;             
		DdNode * bRes0, * bRes1;             

		// cofactor the functions
		if ( bFR != bF ) // bFunc is complemented 
		{
			bF0 = Cudd_Not( cuddE(bFR) );
			bF1 = Cudd_Not( cuddT(bFR) );
		}
		else
		{
			bF0 = cuddE(bFR);
			bF1 = cuddT(bFR);
		}

		if ( LevelF == LevelV )
		{
			if ( Cudd_bddLeq(dd,bF0,bF1) )
				bRes = b1; // pos unate
			else if Cudd_bddLeq(dd,bF1,bF0) )
				bRes = b0; // neg unate
			else
				bRes = dd->vars[0]; // not unate
		}
		else // if ( LevelF < LevelV )
		{
			bRes0 = extraBddCheckVarUnate( dd, bF0, bVar );
			if ( bRes0 == dd->vars[0] )
				bRes = bRes0; // not unate
			else
			{
				bRes1 = extraBddCheckVarUnate( dd, bF1, bVar );
				if ( bRes1 == dd->vars[0] ) 
					bRes = bRes1;  // not unate
				else if ( bRes1 == bRes0 ) 
					bRes = bRes1;  // unateness polarity is compatible
				else if ( bRes0 == dd->vars[1] )
					bRes = bRes1;  // neg cof is ANY; pos cof unateness is returned
				else if ( bRes1 == dd->vars[1] )
					bRes = bRes0;  // pos cof is ANY; neg cof unateness is returned
				else
					bRes = dd->vars[0]; // not unate
			}
		}

		cuddCacheInsert2(dd, extraBddCheckVarUnate, bF, bVar, bRes);
		return bRes;
	}
} /* end of extraBddCheckVarUnate */

/*---------------------------------------------------------------------------*/
/* Definition of static Functions                                            */
/*---------------------------------------------------------------------------*/
