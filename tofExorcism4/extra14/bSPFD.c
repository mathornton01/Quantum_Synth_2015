/**CFile***********************************************************************

  FileName    [bSPFD.c]

  PackageName [extra]

  Synopsis    [Specialized operators used for SPFD computation.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_bddInfoUnique();
				<li> Extra_bddInfoComplete();
				<li> Extra_bddInfoEqual();
				</ul>
			Internal procedures included in this module:
				<ul>
				<li> extraBddInfoEqual();
				</ul>
			Static procedures included in this module:
				<ul>
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

int s_Set1;
int s_Set2;
int s_Step;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**Automaticstart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static DdNode * extraBddInfoEqual( DdManager * dd, DdNode * bF, DdNode * bVar );

/**Automaticend***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Computes the unique information in SPFD provided by a variable.]

  Description [This function takes an SPFD (bF), which is encoded using two sets
  of interleaved variables (Set1 and Set2). The length of the variable range is
  Step. For example, if five variable sets are allocated, and bF depends on the 
  first two sets, Set1=0, Set2=1, Step=5. The number of variables in the function
  is given by the argument nVars. The variable number (iVar) is specified
  using the primary input numeration (not a BDD varible number). In fact, one
  primary input variable stands for two BDD variables in the relation. 
  This function computes the subset of the relation, which contains those minterm 
  pairs that can be distinguished by variable iVar only. For example, in the relation 
  (00 - 11, 01 - 00, 11 - 00, 10 - 01), all minterm pairs can distinguished by both 
  vars, except the pair (01 - 00), which can be distinquished by second var only.
  This function assumes that the relation is ordered: |m1| < |m2|. So it considers 
  only 01 quadrant.]

  SideEffects []

  SeeAlso     [Extra_bddInfoComplete]

******************************************************************************/
DdNode	*
Extra_bddInfoUnique(
  DdManager * dd,
  DdNode * bF,
  int iVar,
  int nVars,
  int Set1,
  int Set2,
  int Step)
{
	DdNode * bCube, * bCof, * bRes, * bTemp;

	assert( iVar < nVars );
	assert( Set1 != Set2 );
	assert( Set1 < Step );
	assert( Set2 < Step );

	bCube = Cudd_bddAnd( dd, Cudd_Not(dd->vars[iVar*Step+Set1]), dd->vars[iVar*Step+Set2] );  Cudd_Ref( bCube );
//Extra_bddPrint( dd, bCube );
//printf( "\n" );
	bCof  = Cudd_Cofactor( dd, bF, bCube );                                                   Cudd_Ref( bCof );
//Extra_bddPrint( dd, bCof );
//printf( "\n" );
	bRes  = Extra_bddInfoEqual( dd, bCof, iVar, nVars, Set1, Set2, Step );                          Cudd_Ref( bRes );
//Extra_bddPrint( dd, bRes );
//printf( "\n" );
	bRes  = Cudd_bddAnd( dd, bTemp = bRes, bCube );                                           Cudd_Ref( bRes );
	Cudd_RecursiveDeref( dd, bTemp );
	Cudd_RecursiveDeref( dd, bCube );
	Cudd_RecursiveDeref( dd, bCof );

	Cudd_Deref( bRes );
    return bRes;
} /* end of Extra_bddInfoUnique */


/**Function********************************************************************

  Synopsis    [Computes the complete information in SPFD provided by a variable.]

  Description []

  SideEffects []

  SeeAlso     [Extra_bddInfoUnique]

******************************************************************************/
DdNode	*
Extra_bddInfoComplete(
  DdManager * dd,
  DdNode * bF,
  int iVar,
  int nVars,
  int Set1,
  int Set2,
  int Step)
{
	DdNode * bCube, * bCof, * bRes;

	assert( iVar < nVars );
	assert( Set1 != Set2 );
	assert( Set1 < Step );
	assert( Set2 < Step );

	bCube = Cudd_bddAnd( dd, Cudd_Not(dd->vars[iVar*Step+Set1]), dd->vars[iVar*Step+Set2] );  Cudd_Ref( bCube );
	bCof  = Cudd_Cofactor( dd, bF, bCube );                                                   Cudd_Ref( bCof );
	bRes  = Cudd_bddAnd( dd, bCof, bCube );                                                   Cudd_Ref( bRes );
	Cudd_RecursiveDeref( dd, bCube );
	Cudd_RecursiveDeref( dd, bCof );

	Cudd_Deref( bRes );
    return bRes;
} /* end of Extra_bddInfoComplete */


/**Function********************************************************************

  Synopsis    [Computes the equal part of the relation.]

  Description [The equal part is defined as all the minterm pairs (m1 - m2),
  for which |m1| = |m2|.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode	*
Extra_bddInfoEqual(
  DdManager * dd,
  DdNode * bF,
  int iVar,
  int nVars,
  int Set1,
  int Set2,
  int Step)
{
    DdNode * bRes;
	DdNode * bRelationEqual;
	DdNode * bComp, * bTemp;
	int x;

	assert( Set1 != Set2 );
	assert( Set1 < Step );
	assert( Set2 < Step );

	// compute the relation X == Y
	bRelationEqual = b1;  Cudd_Ref( bRelationEqual );
	for ( x = 0; x < nVars; x++ )
	if ( x != iVar )
	{
		bComp = Cudd_bddXnor( dd, dd->vars[x*Step+Set1], dd->vars[x*Step+Set2] );    Cudd_Ref( bComp );
		bRelationEqual = Cudd_bddAnd( dd, bTemp = bRelationEqual, bComp );           Cudd_Ref( bRelationEqual );
		Cudd_RecursiveDeref( dd, bComp );
		Cudd_RecursiveDeref( dd, bTemp );
	}

	bRes = Cudd_bddAnd( dd, bF, bRelationEqual );  Cudd_Ref( bRes );
	Cudd_RecursiveDeref( dd, bRelationEqual );

	Cudd_Deref( bRes );
    return bRes;

} /* end of Extra_bddInfoEqual */

/**Function********************************************************************

  Synopsis    [Computes the equal part of the relation.]

  Description [Recursive implementation]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode *
Extra_bddInfoEqual2(
  DdManager * dd,
  DdNode * bF,
  int nVars,
  int Set1,
  int Set2,
  int Step)
{
    DdNode * res;

	assert( Set1 != Set2 );
	assert( Set1 < Step );
	assert( Set2 < Step );
	if ( Set1 < Set2 )
	{
		s_Set1 = Set1;
		s_Set2 = Set2;
	}
	else
	{
		s_Set2 = Set1;
		s_Set1 = Set2;
	}
	s_Step = Step;

    do {
		dd->reordered = 0;
		res = extraBddInfoEqual( dd, bF, dd->vars[nVars] );
    } while (dd->reordered == 1);

    return(res);

} /* end of Extra_bddInfoEqual */


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis [Performs the recursive step of Extra_bddInfoEqual.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
extraBddInfoEqual(
  DdManager * dd,
  DdNode * bF,
  DdNode * bVar)
{
	DdNode * bRes;
    statLine(dd);

	if ( bF == b0 )
		return b0;
	// another terminal case (b1) requires special treatment

	if ( bRes = cuddCacheLookup2(dd, extraBddInfoEqual, bF, bVar) )
		return bRes;
	else
	{
		DdNode * bFR = Cudd_Regular(bF);
		DdNode * bTemp;
		int i, VarTemp, VarTemp2;
		int VarTop;
		int nVars;
		int iVarPrev;

		///////////////////////////////////////////////////////////////////
		// decipher the variables in bVar
		VarTemp = bVar->index;
		if ( cuddT(bVar) == b1 ) // called from the upper level
		{
			iVarPrev = -1;
			nVars = VarTemp;
		}
		else // // called from an intermediate level
		{
			assert( cuddT(cuddT(bVar)) == b1 );
			VarTemp2 = cuddT(bVar)->index;
			if ( VarTemp < VarTemp2 )
			{
				iVarPrev = VarTemp;
				nVars    = VarTemp2;
			}
			else
			{
				iVarPrev = VarTemp2;
				nVars    = VarTemp;
			}
		}
		///////////////////////////////////////////////////////////////////

		///////////////////////////////////////////////////////////////////
		// get the topmost variable (in terms of the primary input vars)
		if ( bF == b1 )
			VarTop = nVars;
		else
			VarTop = bFR->index / s_Step;
		///////////////////////////////////////////////////////////////////


		///////////////////////////////////////////////////////////////////
		// compute the equal-product of all the variables in the range
		bRes = b1;  cuddRef( bRes );
		for ( i = iVarPrev + 1; i < nVars; i++ )
		{
			DdNode * bProd;

			bProd = cuddBddXorRecur( dd, dd->vars[i*s_Step + s_Set1], dd->vars[i*s_Step + s_Set2] );
			if ( bProd == NULL )
			{
				Cudd_RecursiveDeref( dd, bRes );
				return NULL;
			}
			cuddRef( bProd );

			bRes = cuddBddAndRecur( dd, bTemp = bRes, Cudd_Not(bProd) ); 
			if ( bRes == NULL )
			{
				Cudd_RecursiveDeref( dd, bProd );
				Cudd_RecursiveDeref( dd, bTemp );
				return NULL;
			}
			cuddRef( bRes );
			Cudd_RecursiveDeref( dd, bProd );
			Cudd_RecursiveDeref( dd, bTemp );
		}
		///////////////////////////////////////////////////////////////////

		///////////////////////////////////////////////////////////////////
		// consider the case, when the relation is not the constant 1
		///////////////////////////////////////////////////////////////////
		if ( bF != b1 )
		{
			DdNode * bF0,  * bF1;
			DdNode * bFR0, * bFR1;
			DdNode * bF00, * bF01, * bF10, * bF11;
			DdNode * bCacheObject;
			DdNode * bRes0, * bRes1, * bRes2;

			///////////////////////////////////////////////////////////////////
			// compute the cofactors
			if ( bFR->index == VarTop*s_Step + s_Set1 )
			{
				if ( bFR != bF )
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

			bFR0 = Cudd_Regular(bF0);
			if ( bFR0->index == VarTop*s_Step + s_Set2 )
			{
				if ( bFR0 != bF0 )
				{
					bF00 = Cudd_Not( cuddE(bFR0) );
					bF01 = Cudd_Not( cuddT(bFR0) );
				}
				else
				{
					bF00 = cuddE(bFR0);
					bF01 = cuddT(bFR0);
				}
			}
			else
				bF00 = bF01 = bF0;

			bFR1 = Cudd_Regular(bF1);
			if ( bFR1->index == VarTop*s_Step + s_Set2 )
			{
				if ( bFR1 != bF1 )
				{
					bF10 = Cudd_Not( cuddE(bFR1) );
					bF11 = Cudd_Not( cuddT(bFR1) );
				}
				else
				{
					bF10 = cuddE(bFR1);
					bF11 = cuddT(bFR1);
				}
			}
			else
				bF10 = bF11 = bF1;
			///////////////////////////////////////////////////////////////////

			///////////////////////////////////////////////////////////////////
			// call recursively
			bCacheObject = cuddBddAndRecur( dd, dd->vars[VarTop], dd->vars[nVars] );
			if ( bCacheObject == NULL )
			{
				Cudd_RecursiveDeref( dd, bRes );
				return NULL;
			}
			cuddRef( bCacheObject );

			bRes0 = extraBddInfoEqual( dd, bF00, bCacheObject );
			if ( bRes0 == NULL )
			{
				Cudd_RecursiveDeref( dd, bRes );
				Cudd_RecursiveDeref( dd, bCacheObject );
				return NULL;
			}
			cuddRef( bRes0 );

			bRes1 = extraBddInfoEqual( dd, bF11, bCacheObject );
			if ( bRes1 == NULL )
			{
				Cudd_RecursiveDeref( dd, bRes );
				Cudd_RecursiveDeref( dd, bRes0 );
				Cudd_RecursiveDeref( dd, bCacheObject );
				return NULL;
			}
			cuddRef( bRes1 );
			Cudd_RecursiveDeref( dd, bCacheObject );
			///////////////////////////////////////////////////////////////////

			// at this point, only bRes, bRes0, and bRes1 are referenced

			///////////////////////////////////////////////////////////////////
			// compose bRes1 - x1 and x2 should be in the positive polarity
			if ( bRes1 == b0 )
				bRes = b0;
			else if ( Cudd_IsComplement(bRes1) ) 
			{
				bRes1 = cuddUniqueInter(dd, VarTop*s_Step + s_Set2, bTemp = Cudd_Not(bRes1), Cudd_Not(b0) );
				if ( bRes1 == NULL ) 
				{
					Cudd_RecursiveDeref(dd,bRes0);
					Cudd_RecursiveDeref(dd,bTemp);
					Cudd_RecursiveDeref(dd,bRes);
					return NULL;
				}
				bRes1 = Cudd_Not(bRes1);
			} 
			else 
			{
				bRes1 = cuddUniqueInter( dd, VarTop*s_Step + s_Set2, bTemp = bRes1, b0 );
				if ( bRes1 == NULL ) 
				{
					Cudd_RecursiveDeref(dd,bRes0);
					Cudd_RecursiveDeref(dd,bTemp);
					Cudd_RecursiveDeref(dd,bRes);
					return NULL;
				}
			}
			cuddRef( bRes1 );
			cuddDeref( bTemp );

			if ( bRes1 == b0 )
				bRes = b0;
			else if ( Cudd_IsComplement(bRes1) ) 
			{
				bRes1 = cuddUniqueInter(dd, VarTop*s_Step + s_Set1, bTemp = Cudd_Not(bRes1), Cudd_Not(b0) );
				if ( bRes1 == NULL ) 
				{
					Cudd_RecursiveDeref(dd,bRes0);
					Cudd_RecursiveDeref(dd,bTemp);
					Cudd_RecursiveDeref(dd,bRes);
					return NULL;
				}
				bRes1 = Cudd_Not(bRes1);
			} 
			else 
			{
				bRes1 = cuddUniqueInter( dd, VarTop*s_Step + s_Set1, bTemp = bRes1, b0 );
				if ( bRes1 == NULL ) 
				{
					Cudd_RecursiveDeref(dd,bRes0);
					Cudd_RecursiveDeref(dd,bTemp);
					Cudd_RecursiveDeref(dd,bRes);
					return NULL;
				}
			}
			cuddRef( bRes1 );
			cuddDeref( bTemp );
			///////////////////////////////////////////////////////////////////

			///////////////////////////////////////////////////////////////////
			// compose bRes0 - x1 and x2 should be in the negative polarity
			if ( bRes0 == b0 )
				bRes = b0;
			else if ( Cudd_IsComplement(b0) ) 
			{
				bRes0 = cuddUniqueInter(dd, VarTop*s_Step + s_Set2, Cudd_Not(b0), bTemp = Cudd_Not(bRes0) );
				if ( bRes0 == NULL ) 
				{
					Cudd_RecursiveDeref(dd,bRes1);
					Cudd_RecursiveDeref(dd,bTemp);
					Cudd_RecursiveDeref(dd,bRes);
					return NULL;
				}
				bRes0 = Cudd_Not(bRes0);
			} 
			else 
			{
				assert(0);
				bRes0 = cuddUniqueInter( dd, VarTop*s_Step + s_Set2, b0, bTemp = bRes0 );
				if ( bRes0 == NULL ) 
				{
					Cudd_RecursiveDeref(dd,bRes1);
					Cudd_RecursiveDeref(dd,bTemp);
					Cudd_RecursiveDeref(dd,bRes);
					return NULL;
				}
			}
			cuddRef( bRes0 );
			cuddDeref( bTemp );

			if ( bRes0 == b0 )
				bRes = b0;
			else if ( Cudd_IsComplement(b0) ) 
			{
				bRes0 = cuddUniqueInter(dd, VarTop*s_Step + s_Set1, Cudd_Not(b0), bTemp = Cudd_Not(bRes0) );
				if ( bRes0 == NULL ) 
				{
					Cudd_RecursiveDeref(dd,bRes1);
					Cudd_RecursiveDeref(dd,bTemp);
					Cudd_RecursiveDeref(dd,bRes);
					return NULL;
				}
				bRes0 = Cudd_Not(bRes0);
			} 
			else 
			{
				assert(0);
				bRes0 = cuddUniqueInter( dd, VarTop*s_Step + s_Set1, b0, bTemp = bRes0 );
				if ( bRes0 == NULL ) 
				{
					Cudd_RecursiveDeref(dd,bRes1);
					Cudd_RecursiveDeref(dd,bTemp);
					Cudd_RecursiveDeref(dd,bRes);
					return NULL;
				}
			}
			cuddRef( bRes0 );
			cuddDeref( bTemp );
			///////////////////////////////////////////////////////////////////


			///////////////////////////////////////////////////////////////////
			// compute the sum of the two
			bRes2 = cuddBddAndRecur( dd, Cudd_Not(bRes0), Cudd_Not(bRes1) );
			if ( bRes2 == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bRes1);
				Cudd_RecursiveDeref(dd,bRes0);
				Cudd_RecursiveDeref(dd,bRes);
				return NULL;
			}
			cuddRef( bRes2 );
			bRes2 = Cudd_Not(bRes2);
			Cudd_RecursiveDeref(dd,bRes0);
			Cudd_RecursiveDeref(dd,bRes1);
			///////////////////////////////////////////////////////////////////

			///////////////////////////////////////////////////////////////////
			// multiply by the equality of the preceeding vars
			bRes = cuddBddAndRecur( dd, bTemp = bRes, bRes2 );
			if ( bRes == NULL ) 
			{
				Cudd_RecursiveDeref(dd,bTemp);
				Cudd_RecursiveDeref(dd,bRes2);
				return NULL;
			}
			cuddRef( bRes );
			Cudd_RecursiveDeref(dd,bTemp);
			Cudd_RecursiveDeref(dd,bRes2);
			///////////////////////////////////////////////////////////////////
		}

		cuddCacheInsert2(dd, extraBddInfoEqual, bF, bVar, bRes);

		cuddDeref( bRes );
		return bRes;
	}

} /* end of extraBddInfoUnique */


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/
