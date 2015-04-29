/**CFile***********************************************************************

  FileName    [zUCP1.c]

  PackageName [extra]

  Synopsis    [Experimental version of some ZDD operators.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_zddSolveUCP1();
				</ul>
			   Internal procedures included in this module:
				<ul>
				</ul>
			   Static procedures included in this module:
				<ul>
				<li> Extra_zddMaxTauRow();
				<li> Extra_zddMaxTauCol();
				<li> extraZddMaxTauRow();
				<li> extraZddMaxTauCol();
				</ul>

              The row/column dominance computation operators were proposed in 
			  O. Coudert, C.-J. R. Shi. Exact Multi-Layer Topological 
			  Planar Routing. Proc. of IEEE Custom Integrated Circuit Conference '96, 
			  pp. 179-182.
	          ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$zUCP1.c, v.1.2, November 26, 2000, alanmi $]

******************************************************************************/

#include "util.h"
#include "cuddInt.h"
#include "extra.h"
#include "time.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                     */
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

static DdNode * Extra_zddMaxTauRow ARGS((DdManager *dd, DdNode *Q, DdNode *P));
static DdNode * Extra_zddMaxTauCol ARGS((DdManager *dd, DdNode *Q, DdNode *P));
static DdNode * extraZddMaxTauRow ARGS((DdManager *dd, DdNode *Q, DdNode *P));
static DdNode * extraZddMaxTauCol ARGS((DdManager *dd, DdNode *Q, DdNode *P));

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported Functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Solves set covering problem using the set encoded in ZDDs.]

  Description [Solves set covering problem in the following representation:
               Each element is encoded using a separate ZDD variable. The only 
			   argument (S) is the set of subsets used to cover elements. 
			   The set elements are determined as the support of S. Returns
			   the minimal number of subsets that cover all the elements.]

  SideEffects [Dereferences the original set of subsets.]

  SeeAlso     [Cudd_zddSolveUCP2]

******************************************************************************/
DdNode	*
Extra_zddSolveUCP1(
  DdManager * dd,
  DdNode * S)
{
    DdNode	*Support, *Vertices;
	DdNode  *zRows, *zCols, *zColsOld;
	DdNode  *zEss, *zEssTotal, *zTemp, *Result;
	int fSolveCyclicCore;
	int cIter;
	long clk, clk0;
	int fStop;

//	return S;

	/* just in case, compute the maximal of the set */
	clk = clock();
	S = Extra_zddMaximal( dd, zTemp = S );
	if ( S == NULL )
		return NULL;
	Cudd_Ref( S );

	//////////////////////////////////////////////////////////////
	printf( "\nOriginal set" );
	Cudd_zddPrintDebug( dd, zTemp, dd->sizeZ, 1 );
	printf( "\nMaximal set" );
	Cudd_zddPrintDebug( dd, S, dd->sizeZ, 1 );
	printf( "\nMaximal set computation time = %.2f sec\n",  
			 (float)(clock() - clk)/(float)(CLOCKS_PER_SEC) );
	//////////////////////////////////////////////////////////////

	/* dereference the original set */
//	Cudd_RecursiveDerefZdd( dd, zTemp );

	/* find the vertices to be covered */
	clk0 = clock();
	Support = Extra_zddSupport( dd, S );
	if ( Support == NULL )
	{
		Cudd_RecursiveDerefZdd( dd, S );
		return NULL;
	}
	Cudd_Ref( Support );

	/* convert them into singletons */
	Vertices = Extra_zddTuples( dd, 1, Support );
	if ( Vertices == NULL )
	{
		Cudd_RecursiveDerefZdd( dd, S );
		Cudd_RecursiveDerefZdd( dd, Support );
		return NULL;
	}
	Cudd_Ref( Vertices );
	Cudd_RecursiveDerefZdd( dd, Support );

	//////////////////////////////////////////////////////////////
	printf( "\nVertices (two nodes stand for terminals)" );
	Cudd_zddPrintDebug( dd, Vertices, dd->sizeZ, 1 );
	//////////////////////////////////////////////////////////////
	printf( "\nVertices are\n" );
	{ int x; for ( x = 1; x <= dd->sizeZ; x++ ); printf("\n"); }
	Cudd_zddPrintMinterm( dd, Vertices );

//	cuddGarbageCollectZdd( dd, 1 );

	/* start sets of rows and columns */
	zRows = Vertices;
	zCols = S; 
	zColsOld = zCols;
	Cudd_Ref( zColsOld );
	/* start the set of all esentials */
	zEssTotal = dd->zero; 
	Cudd_Ref( zEssTotal );

	/* at this point, referenced are zRows, zCols, zColsOld, zEssTotal */

	// derive the cyclic core
	cIter = 0;
	fStop = 0;
	while ( 1 )
	{
		printf("\nITERATION #%d\n", ++cIter);

		/* compute dominating columns */
		clk = clock();
		zCols = Extra_zddMaxTauCol( dd, zRows, zTemp = zCols );
		if ( zCols == NULL ) 
			return NULL;
		Cudd_Ref( zCols );
		Cudd_RecursiveDerefZdd( dd, zTemp );

		//////////////////////////////////////////////////////////////
		printf( "\nMaxDomCols" );
		Cudd_zddPrintDebug( dd, zCols, dd->sizeZ, 1 );
		printf( "\nMaxDomCols computation time = %.2f sec\n",  
				 (float)(clock() - clk)/(float)(CLOCKS_PER_SEC) );
		//////////////////////////////////////////////////////////////
	printf( "\nMaxDomCols are\n" );
	{ int x; for ( x = 1; x <= dd->sizeZ; x++ ); printf("\n"); }
	Cudd_zddPrintMinterm( dd, zCols );

		/* compute dominating rows */
		clk = clock();
		zRows = Extra_zddMaxTauRow( dd, zTemp = zRows, zCols );
		if ( zRows == NULL ) 
			return NULL;
		Cudd_Ref( zRows );
		Cudd_RecursiveDerefZdd( dd, zTemp );

		//////////////////////////////////////////////////////////////
		printf( "\nMaxDomRows" );
		Cudd_zddPrintDebug( dd, zRows, dd->sizeZ, 1 );
		printf( "\nMaxDomRows computation time = %.2f sec\n",  
				 (float)(clock() - clk)/(float)(CLOCKS_PER_SEC) );
		//////////////////////////////////////////////////////////////
	printf( "\nMaxDomRows are\n" );
	{ int x; for ( x = 1; x <= dd->sizeZ; x++ ); printf("\n"); }
	Cudd_zddPrintMinterm( dd, zRows );

		/* compute essential elements */
		clk = clock();
		zEss = Cudd_zddIntersect( dd, zRows, zCols );
		if ( zEss == NULL ) 
			return NULL;
		Cudd_Ref( zEss );

		/* add essential elements to the set */
		zEssTotal = Cudd_zddUnion( dd, zTemp = zEssTotal, zEss );
		if ( zEssTotal == NULL ) 
			return NULL;
		Cudd_Ref( zEssTotal );
		Cudd_RecursiveDerefZdd( dd, zTemp );

		//////////////////////////////////////////////////////////////
		printf( "\nTotalEss" );
		Cudd_zddPrintDebug( dd, zEssTotal, dd->sizeZ, 1 );
		printf( "\nTotalEss computation time = %.2f sec\n",  
				 (float)(clock() - clk)/(float)(CLOCKS_PER_SEC) );
		//////////////////////////////////////////////////////////////
	printf( "\nTotalEss are\n" );
	{ int x; for ( x = 1; x <= dd->sizeZ; x++ ); printf("\n"); }
	Cudd_zddPrintMinterm( dd, zEssTotal );

		/* subtract essential elements from rows */
		zRows = Cudd_zddDiff( dd, zTemp = zRows, zEss );
		if ( zRows == NULL ) 
			return NULL;
		Cudd_Ref( zRows );
		Cudd_RecursiveDerefZdd( dd, zTemp );

		//////////////////////////////////////////////////////////////
		printf( "\nRows" );
		Cudd_zddPrintDebug( dd, zRows, dd->sizeZ, 1 );
		//////////////////////////////////////////////////////////////

		/* subtract essential elements from columns */
		zCols = Cudd_zddDiff( dd, zTemp = zCols, zEss );
		if ( zCols == NULL ) 
			return NULL;
		Cudd_Ref( zCols );
		Cudd_RecursiveDerefZdd( dd, zTemp );
		Cudd_RecursiveDerefZdd( dd, zEss );

		//////////////////////////////////////////////////////////////
		printf( "\nCols" );
		Cudd_zddPrintDebug( dd, zCols, dd->sizeZ, 1 );
		//////////////////////////////////////////////////////////////

		// check whether it is time to stop
//		if ( zRows == dd->zero && zCols == dd->zero )
		if ( zRows == dd->zero )
		{
			Cudd_RecursiveDerefZdd( dd, zCols );
			zCols = dd->zero;
			Cudd_Ref( zCols );
			break;
		}
//		else if ( zRows == dd->zero || zCols == dd->zero )
		else if ( zCols == dd->zero )
			assert( 0 );
	
		// check whether this is a cyclic core
		if ( zCols == zColsOld )
		{
			if ( fStop )
				break;
			fStop = 1;
		}
		else 
			fStop = 0;
;
		// otherwise keep reducing

		// remember the state of columns
		Cudd_RecursiveDerefZdd( dd, zColsOld );
		zColsOld = zCols;
		Cudd_Ref( zColsOld );
	}
	Cudd_RecursiveDerefZdd( dd, zColsOld );

	//////////////////////////////////////////////////////////////
	printf( "\nThe set of essential produced" );
	Cudd_zddPrintDebug( dd, zEssTotal, dd->sizeZ, 1 );
	printf( "\nCovering matrix reduction time = %.2f sec\n",  
			 (float)(clock() - clk0)/(float)(CLOCKS_PER_SEC) );
	//////////////////////////////////////////////////////////////

	/* check the status of the cyclic core */
	fSolveCyclicCore = 0;
	if ( zRows == dd->zero && zCols == dd->zero )
	{
		printf("\nThe cyclic core is empty\n");
	}
	else
	{
		printf("\nThe cyclic core (CC) has size: %d x %d\n",
			      Cudd_zddCount( dd, zRows ), Cudd_zddCount( dd, zCols ) );
		printf( "\nCCRows" );
		Cudd_zddPrintDebug( dd, zRows, dd->sizeZ, 1 );
		printf( "\nCCCols" );
		Cudd_zddPrintDebug( dd, zCols, dd->sizeZ, 1 );

		fSolveCyclicCore = 1;
	}

	/* greedily solve the cyclic core */
	Result = Cudd_zddUnion( dd, zCols, zEssTotal );
	if ( Result == NULL )
	{
		Cudd_RecursiveDerefZdd( dd, zRows );
		Cudd_RecursiveDerefZdd( dd, zCols );
		Cudd_RecursiveDerefZdd( dd, zEssTotal );
		return NULL;
	}
	Cudd_Ref( Result );
	Cudd_RecursiveDerefZdd( dd, zRows );
	Cudd_RecursiveDerefZdd( dd, zCols );
	Cudd_RecursiveDerefZdd( dd, zEssTotal );

	Cudd_Deref( Result );
	return Result;
} /* end of Extra_zddSolveUCP1 */


/*---------------------------------------------------------------------------*/
/* Definition of internal Functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static Functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Computes the set of maximal dominating rows in UCP.]

  Description []

  SideEffects []

  SeeAlso     [Extra_zddMaxTauCol]

******************************************************************************/
static DdNode	*
Extra_zddMaxTauRow(
  DdManager * dd,
  DdNode * Q,
  DdNode * P)
{
    DdNode	*res;
    int		autoDynZ;

    autoDynZ = dd->autoDynZ;
    dd->autoDynZ = 0;

    do {
	dd->reordered = 0;
	res = extraZddMaxTauRow(dd, Q, P);
    } while (dd->reordered == 1);
    dd->autoDynZ = autoDynZ;
    return(res);

} /* end of Extra_zddMaxTauRow */


/**Function********************************************************************

  Synopsis    [Computes the set of maximal dominating columns in UCP.]

  Description []

  SideEffects []

  SeeAlso     [Extra_zddMaxTauCol]

******************************************************************************/
static DdNode	*
Extra_zddMaxTauCol(
  DdManager * dd,
  DdNode * Q,
  DdNode * P)
{
    DdNode	*res;
    int		autoDynZ;

    autoDynZ = dd->autoDynZ;
    dd->autoDynZ = 0;

    do {
	dd->reordered = 0;
	res = extraZddMaxTauCol(dd, Q, P);
    } while (dd->reordered == 1);
    dd->autoDynZ = autoDynZ;
    return(res);

} /* end of Extra_zddMaxTauCol */


/**Function********************************************************************

  Synopsis [Performs the recursive step of Extra_zddMaxTauRow.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
static DdNode	*
extraZddMaxTauRow(
  DdManager * dd,
  DdNode * Q,
  DdNode * P)
{
	DdNode *zRes;
    statLine(dd); 

	if ( Q == dd->zero || P == dd->zero )
		return dd->zero;

	if ( P == dd->one )
	{
		if ( Extra_zddEmptyBelongs( dd, Q ) )
			return dd->one;
		else
			return dd->zero;
	}

	if ( Q == dd->one && Extra_zddEmptyBelongs( dd, P ) )
		return dd->one;


    /* check cache */
	zRes = cuddCacheLookup2Zdd(dd, extraZddMaxTauRow, Q, P);
	if (zRes)
		return(zRes);
	else
	{
//		int TopQ = dd->permZ[Q->index];
//		int TopP = dd->permZ[P->index];
		int TopQ = cuddIZ( dd, Q->index );
		int TopP = cuddIZ( dd, P->index );

		if ( TopQ < TopP )
			return extraZddMaxTauRow( dd, cuddE(Q), P );
		else 
		{
			DdNode *zQ0, *zQ1, *zQsp, *zP01;
			DdNode *zTemp, *zRes0, *zRes1;

			if ( TopQ > TopP )
			{
				zQ0 = Q;
				zQ1 = dd->zero;
			}
			else /* if ( TopQ == TopP ) */
			{
				zQ0 = cuddE(Q);
				zQ1 = cuddT(Q);
			}

			zQsp = extraZddNotSubSet( dd, zQ0, cuddE(P) );
			if ( zQsp == NULL )
				return NULL;
			cuddRef( zQsp );

			zTemp = cuddZddUnion( dd, zQ1, zQsp );
			if ( zTemp == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zQsp);
				return NULL;
			}
			cuddRef( zTemp );

			zRes1 = extraZddMaxTauRow( dd, zTemp, cuddT(P) );
			if ( zRes1 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zQsp);
				Cudd_RecursiveDerefZdd(dd, zTemp);
				return NULL;
			}
			cuddRef( zRes1 );
			Cudd_RecursiveDerefZdd(dd, zTemp);

			zTemp = cuddZddDiff( dd, zQ0, zQsp );
			if ( zTemp == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zQsp);
				Cudd_RecursiveDerefZdd(dd, zRes1);
				return NULL;
			}
			cuddRef( zTemp );
			Cudd_RecursiveDerefZdd(dd, zQsp);

			zP01 = cuddZddUnion( dd, cuddE(P), cuddT(P) );
			if ( zP01 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zTemp);
				Cudd_RecursiveDerefZdd(dd, zRes1);
				return NULL;
			}
			cuddRef( zP01 );

			zRes0 = extraZddMaxTauRow( dd, zTemp, zP01 );
			if ( zRes0 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zP01);
				Cudd_RecursiveDerefZdd(dd, zTemp);
				Cudd_RecursiveDerefZdd(dd, zRes1);
				return NULL;
			}
			cuddRef( zRes0 );
			Cudd_RecursiveDerefZdd(dd, zP01);
			Cudd_RecursiveDerefZdd(dd, zTemp);

			/* remove subsets without this element covered by subsets with this element */
			zRes0 = extraZddNotSubSet(dd, zTemp = zRes0, zRes1);
			if ( zRes0 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zTemp);
				Cudd_RecursiveDerefZdd(dd, zRes1);
				return NULL;
			}
			cuddRef( zRes0 );
			Cudd_RecursiveDerefZdd(dd, zTemp);

			/* create the new node */
			zRes = cuddZddGetNode( dd, P->index, zRes1, zRes0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				return NULL;
			}
			cuddDeref( zRes0 );
			cuddDeref( zRes1 );
		}

		/* insert the result into cache */
		cuddCacheInsert2(dd, extraZddMaxTauRow, Q, P, zRes);
		return zRes;
	}
} /* end of extraZddMaxTauRow */



/**Function********************************************************************

  Synopsis [Performs the recursive step of Extra_zddMaxTauCol.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
static DdNode	*
extraZddMaxTauCol(
  DdManager * dd,
  DdNode * Q,
  DdNode * P)
{
	DdNode *zRes;
    statLine(dd); 

	if ( Q == dd->zero || P == dd->zero )
		return dd->zero;

	if ( P == dd->one )
	{
		if ( Extra_zddEmptyBelongs( dd, Q ) )
			return dd->one;
		else
			return dd->zero;
	}

	if ( Q == dd->one )
		return dd->one;


    /* check cache */
	zRes = cuddCacheLookup2Zdd(dd, extraZddMaxTauCol, Q, P);
	if (zRes)
		return(zRes);
	else
	{
		int TopQ = dd->permZ[Q->index];
		int TopP = dd->permZ[P->index];

		if ( TopQ < TopP )
			return extraZddMaxTauCol( dd, cuddE(Q), P );
		else if ( TopQ > TopP ) 
		{
			DdNode *zP01 = cuddZddUnion( dd, cuddE(P), cuddT(P) );
			if ( zP01 == NULL )
				return NULL;
			cuddRef( zP01 );

			zRes = extraZddMaxTauCol( dd, Q, zP01 );
			if ( zRes == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zP01);
				return NULL;
			}
			cuddRef( zRes );
			Cudd_RecursiveDerefZdd(dd, zP01);
			cuddDeref( zRes );
		}
		else /* if ( TopQ == TopP ) */
		{
			DdNode *zPsp, *zQ01, *zTemp, *zRes0, *zRes1;

			zPsp = extraZddNotSupSet( dd, cuddT(P), cuddT(Q) );
			if ( zPsp == NULL )
				return NULL;
			cuddRef( zPsp );

			zTemp = cuddZddUnion( dd, cuddE(P), zPsp );
			if ( zTemp == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zPsp);
				return NULL;
			}
			cuddRef( zTemp );

			zRes0 = extraZddMaxTauCol( dd, cuddE(Q), zTemp );
			if ( zRes0 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zPsp);
				Cudd_RecursiveDerefZdd(dd, zTemp);
				return NULL;
			}
			cuddRef( zRes0 );
			Cudd_RecursiveDerefZdd(dd, zTemp);

			zTemp = cuddZddDiff( dd, cuddT(P), zPsp );
			if ( zTemp == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zPsp);
				Cudd_RecursiveDerefZdd(dd, zRes0);
				return NULL;
			}
			cuddRef( zTemp );
			Cudd_RecursiveDerefZdd(dd, zPsp);

			zQ01 = cuddZddUnion( dd, cuddE(Q), cuddT(Q) );
			if ( zQ01 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zTemp);
				Cudd_RecursiveDerefZdd(dd, zRes0);
				return NULL;
			}
			cuddRef( zQ01 );

			zRes1 = extraZddMaxTauCol( dd, zQ01, zTemp );
			if ( zRes1 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zQ01);
				Cudd_RecursiveDerefZdd(dd, zTemp);
				Cudd_RecursiveDerefZdd(dd, zRes0);
				return NULL;
			}
			cuddRef( zRes1 );
			Cudd_RecursiveDerefZdd(dd, zQ01);
			Cudd_RecursiveDerefZdd(dd, zTemp);

			/* remove subsets without this element covered by subsets with this element */
			zRes0 = extraZddNotSubSet(dd, zTemp = zRes0, zRes1);
			if ( zRes0 == NULL )
			{
				Cudd_RecursiveDerefZdd(dd, zTemp);
				Cudd_RecursiveDerefZdd(dd, zRes1);
				return NULL;
			}
			cuddRef( zRes0 );
			Cudd_RecursiveDerefZdd(dd, zTemp);

			/* create the new node */
			zRes = cuddZddGetNode( dd, P->index, zRes1, zRes0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zRes0 );
				Cudd_RecursiveDerefZdd( dd, zRes1 );
				return NULL;
			}
			cuddDeref( zRes0 );
			cuddDeref( zRes1 );
		}

		/* insert the result into cache */
		cuddCacheInsert2(dd, extraZddMaxTauCol, Q, P, zRes);
		return zRes;
	}
} /* end of extraZddMaxTauCol */

