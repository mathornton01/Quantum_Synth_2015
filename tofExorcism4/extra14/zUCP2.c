/**CFile***********************************************************************

  FileName    [zUCP2.c]

  PackageName [extra]

  Synopsis    [Experimental version of some ZDD operators.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_bddGetSingleOutput();
				<li> Extra_zddReduceMatrix();
				<li> Extra_zddPrimes();
				<li> Extra_zddMaxRowDom();
				<li> Extra_zddMaxColDom();
				<li> Extra_zddLitCountTotal();
				<li> printStatsZdd();
				<li> printStatsBdd();
				</ul>
			   Internal procedures included in this module:
				<ul>
				<li> extraZddPrimes();
				<li> extraZddMaxRowDom();
				<li> extraZddMaxColDomAux();
				<li> extraZddMaxColDom();
				</ul>
			   Static procedures included in this module:
				<ul>
				</ul>

              For additional details on the procedures of this module
			  refer to their author: O. Coudert. Two-Level Logic Minimization: 
			  An Overview. Integration. Vol. 17, No. 2, pp. 97-140, Oct 1994.

	          ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$zUCP2.c, v.1.2, November 26, 2000, alanmi $]

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

#define SECOND_OPTION 1

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static double extraZddLitCountTotal( DdManager* dd, const DdNode* Cover, double* nPaths );
static int s_RunCounter = 1;

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported Functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Computes the single output function.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int 
Extra_bddGetSingleOutput( 
  DdManager * dd,       /* the manager */
  DdNode ** bIns,       /* the elementary BDD vars for inputs (should include additional vars for outputs) */
  int nIns,             /* the number of inputs */
  DdNode ** bOuts,      /* the BDDs for output */
  DdNode ** bOutDcs,    /* the BDDs for output Don't-Cares */
  int nOuts,            /* the number of outputs */
  DdNode ** bFuncOn,    /* the place for the on-set of the derived function */
  DdNode ** bFuncOnDc ) /* the place for the on+dc-set of the derived function */
{
	int n, m;
	DdNode *bOnDc, *bTemp, *bAux, *bPart;

	/* save the reordering status */
    int autoDynZ = dd->autoDynZ;

	/* enable dynamic variable reordering to reduce the size of the resulting functions */
//	Cudd_AutodynEnable(dd, CUDD_REORDER_SIFT);

	/* start the on-set and dc-set */
	Cudd_Ref( *bFuncOn = Cudd_Not(dd->one) );
	Cudd_Ref( *bFuncOnDc = dd->one );

	for ( n = 0; n < nOuts; n++ )
	{
		/* add on-set and dc-set for this output */
		if ( bOutDcs )
			bOnDc = Cudd_bddOr( dd, bOuts[n], bOutDcs[n] );
		else
			bOnDc = bOuts[n];

		if ( bOnDc == NULL ) return 0;
		Cudd_Ref( bOnDc );

		/* add the negative polarity output var to the on-and-dc set */
		bTemp = Cudd_bddOr( dd, Cudd_Not( bIns[ nIns + n ] ), bOnDc );
		if ( bTemp == NULL ) return 0;
		Cudd_Ref( bTemp );
		Cudd_RecursiveDeref( dd, bOnDc );

		/* multiply this component by the on-and-dc set */
		*bFuncOnDc = Cudd_bddAnd( dd, bAux = *bFuncOnDc, bTemp );
		if ( *bFuncOnDc == NULL ) return 0;
		Cudd_Ref( *bFuncOnDc );
		Cudd_RecursiveDeref( dd, bAux );
		Cudd_RecursiveDeref( dd, bTemp );

		/* create the product part of the pure on-set */
		bPart = bOuts[n];
		Cudd_Ref( bPart );
		for ( m = 0; m < nOuts; m++ )
		{
			bTemp = Cudd_NotCond( bIns[ nIns + m ], (int)(m != n) );
			bPart = Cudd_bddAnd( dd, bAux = bPart, bTemp );
			if ( bPart == NULL ) return 0;
			Cudd_Ref( bPart );
			Cudd_RecursiveDeref( dd, bAux );
		}

		/* add this component to the on-set */
		*bFuncOn = Cudd_bddOr( dd, bAux = *bFuncOn, bPart );
		if ( *bFuncOn == NULL ) return 0;
		Cudd_Ref( *bFuncOn );
		Cudd_RecursiveDeref( dd, bAux );
		Cudd_RecursiveDeref( dd, bPart );
	}

	Cudd_Deref( *bFuncOn );
	Cudd_Deref( *bFuncOnDc );

	/* restore reordering parameter */
	dd->autoDynZ = autoDynZ;
    return 1;

} /* end of Extra_bddGetSingleOutput */


/**Function********************************************************************

  Synopsis    [Reduces the implicit matrix.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * 
Extra_zddReduceMatrix( 
  DdManager * dd,       /* the manager */
  DdNode * bRows,       /* the BDD for rows */
  DdNode * zCols,       /* the ZDD for columns */
  DdNode ** zRowsCC,    /* the rows of the Cyclic Core (or dd->zero if empty) */
  DdNode ** zColsCC,    /* the rows of the Cyclic Core (or dd->zero if empty) */
  int fVerbose )        /* outputs stats, if this flag is true */
{
	/* temporary variables */
	DdNode *zRows, *zColsOld, *zEssTotal; 
	int cIter = 0;
	long clk1, clk2 = clock();

	/* start the set of all esentials and the set of old columns */
	Cudd_Ref( zEssTotal = dd->zero );
	Cudd_Ref( zColsOld = zCols );

	/* reference the set of rows and columns before entring the loop */
	Cudd_Ref( bRows );
	Cudd_Ref( zCols );

	/* derive the cyclic core */
	while ( 1 )
	{
		DdNode *zEss, *zTemp; 

		if ( fVerbose )
		printf("\nITERATION #%d\n", ++cIter );

		/* compute the signature cubes */
		clk1 = clock();
		zRows = Extra_zddMaxRowDom( dd, bRows, zCols );
		if ( zRows == NULL ) return 0;
		Cudd_Ref( zRows );
		Cudd_RecursiveDeref( dd, bRows );

		if ( fVerbose )
		printStatsZdd( "DomRows:", dd, zRows, clock() - clk1 );

		/* remove the useless primes */
		zCols = Extra_zddSubSet( dd, zTemp = zCols, zRows );
		if ( zCols == NULL ) return 0;
		Cudd_Ref( zCols );
		Cudd_RecursiveDerefZdd( dd, zTemp );

		/* compute essential elements */
		clk1 = clock();
		zEss = Cudd_zddIntersect( dd, zRows, zCols );
		if ( zEss == NULL ) return 0;
		Cudd_Ref( zEss );

		/* add essential elements to the set */
		zEssTotal = Cudd_zddUnion( dd, zTemp = zEssTotal, zEss );
		if ( zEssTotal == NULL ) return 0;
		Cudd_Ref( zEssTotal );
		Cudd_RecursiveDerefZdd( dd, zTemp );

		if ( fVerbose )
		printStatsZdd( "Ess:", dd, zEss, clock() - clk1 );

		/* subtract essential elements from rows */
		clk1 = clock();
		zRows = Cudd_zddDiff( dd, zTemp = zRows, zEss );
		if ( zRows == NULL ) return 0;
		Cudd_Ref( zRows );
		Cudd_RecursiveDerefZdd( dd, zTemp );

		if ( fVerbose )
		printStatsZdd( "Rows:", dd, zRows, clock() - clk1 );

		/* subtract essential elements from columns */
		clk1 = clock();
		zCols = Cudd_zddDiff( dd, zTemp = zCols, zEss );
		if ( zCols == NULL ) return 0;
		Cudd_Ref( zCols );
		Cudd_RecursiveDerefZdd( dd, zTemp );
		Cudd_RecursiveDerefZdd( dd, zEss );

		if ( fVerbose )
		printStatsZdd( "Cols:", dd, zCols, clock() - clk1 );

		/* check whether it is time to stop */
		if ( zRows == dd->zero && zCols == dd->zero )
			break;
		else if ( zRows == dd->zero || zCols == dd->zero )
			assert( 0 );

		/* compute the dominated columns */
		clk1 = clock();
		zCols = Extra_zddMaxColDom( dd, zRows, zTemp = zCols );
		if ( zCols == NULL ) return 0;
		Cudd_Ref( zCols );
		Cudd_RecursiveDerefZdd( dd, zTemp );

		if ( fVerbose )
		printStatsZdd( "DomCols:", dd, zCols, clock() - clk1 );
	
		/* check whether this is a cyclic core */
		if ( zCols == zColsOld )
			break;
		/* otherwise keep reducing */

		/* find the BDD of the rows */
		clk1 = clock();
		bRows = Cudd_MakeBddFromZddCover( dd, zRows );
		if ( bRows == NULL ) return 0;
		Cudd_Ref( bRows );
		Cudd_RecursiveDerefZdd( dd, zRows );

		if ( fVerbose )
		printStatsBdd( "RowsBdd:", dd, bRows, clock() - clk1 );

		/* remember the state of columns */
		Cudd_RecursiveDerefZdd( dd, zColsOld );
		zColsOld = zCols;
		Cudd_Ref( zColsOld );
	}
	Cudd_RecursiveDerefZdd( dd, zColsOld );

	if ( fVerbose )
	printf( "\nThe implicit table reduction time = %.3f sec\n", (float)(clock() - clk2)/(float)(CLOCKS_PER_SEC) );

	/* prepare the return values */
	*zRowsCC = zRows;
	*zColsCC = zCols;

	Cudd_Deref( *zRowsCC );
	Cudd_Deref( *zColsCC );
	Cudd_Deref( zEssTotal );

	return zEssTotal;
}


/**Function********************************************************************

  Synopsis    [Solves the cyclic core approximately by computing ISOP of the non-covered area.]

  Description [Given the on-set and the off-set of the incompletely specified 
  function, upgraded the partial solution (zSolPart) of the covering matrix to
  the complete solution, which is returned.]

  SideEffects []

  SeeAlso     []

******************************************************************************/

DdNode *
Cudd_zddSolveCCbyISOP( 
  DdManager * dd, 
  DdNode * zSolPart,
  DdNode * bFuncOnSet,
  DdNode * bFuncOnDcSet )
{
	DdNode *bOnSetPart, *bOnSetNeedingCover, *bIrrCover, *zIrrCover, *zRes;
		
	/* find bdd for the currently found part of solution */
	bOnSetPart = Cudd_MakeBddFromZddCover( dd, zSolPart );
	if ( bOnSetPart == NULL ) 
		return NULL;
	Cudd_Ref( bOnSetPart );

	/* find the part of the remaining on-set outside the current solution */
	bOnSetNeedingCover = Cudd_bddAnd( dd, bFuncOnSet, Cudd_Not( bOnSetPart ) );
	if ( bOnSetNeedingCover == NULL ) 
		return NULL;
	Cudd_Ref( bOnSetNeedingCover );
	Cudd_RecursiveDeref( dd, bOnSetPart );

	/* call the irredundant cover computation for this part */
	bIrrCover = Cudd_zddIsop( dd, bOnSetNeedingCover, bFuncOnDcSet, &zIrrCover );
	/* both IrrCoverBdd and IrrCoverZdd are not referenced */
	if ( bIrrCover == NULL ) 
		return NULL;
	Cudd_Ref( zIrrCover );
	Cudd_Ref( bIrrCover );
	Cudd_RecursiveDeref( dd, bIrrCover );
	Cudd_RecursiveDeref( dd, bOnSetNeedingCover );

	/* add the cover to the partial solution found so far */
	zRes = Cudd_zddUnion( dd, zSolPart, zIrrCover );
	if ( zRes == NULL ) 
		return NULL;
	Cudd_Ref( zRes );
	Cudd_RecursiveDerefZdd( dd, zIrrCover );
	Cudd_Deref( zRes );

	return zRes;
}

/**Function********************************************************************

  Synopsis    [Computes the set of primes as a ZDD.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode *Extra_zddPrimes( DdManager * dd, DdNode * F )
{
    DdNode	*res;
    do {
		dd->reordered = 0;
		res = extraZddPrimes(dd, F);
		if ( dd->reordered == 1 )
			printf("\nReordering in Extra_zddPrimes()\n");
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddPrimes */

/**Function********************************************************************

  Synopsis    [Computes the dominating rows.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode *Extra_zddMaxRowDom( DdManager * dd, DdNode* F, DdNode* P )
{
    DdNode	*res;
    do {
		dd->reordered = 0;
		res = extraZddMaxRowDom(dd, F, P);
		if ( dd->reordered == 1 )
			printf("\nReordering in Extra_zddMaxRowDom()\n");
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddMaxRowDom */

/**Function********************************************************************

  Synopsis    [Computes the dominating columns.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode *Extra_zddMaxColDom( DdManager * dd, DdNode* Q, DdNode* P )
{
    DdNode	*res;
    do {
		dd->reordered = 0;
		res = extraZddMaxColDomAux(dd, Q, P);
		if ( dd->reordered == 1 )
			printf("\nReordering in Extra_zddMaxColDom()\n");
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddMaxColDom */

/**Function********************************************************************

  Synopsis    [Counts the number of literals and cubes in the cover.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
double Extra_zddLitCountTotal( DdManager * dd, const DdNode * Cover, double * nPaths )
{
	double CountPaths;
	double Result;
	s_RunCounter++;
    Result = extraZddLitCountTotal( dd, Cover, &CountPaths );
	if ( nPaths )
		*nPaths = CountPaths;
	return Result;
}

/**Function********************************************************************

  Synopsis    [Prints statistics for a ZDD.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void printStatsZdd( char* Name, DdManager* dd, DdNode* Zdd, long Time )
{
	double nPath = 0.0, nLiterals = Extra_zddLitCountTotal( dd, Zdd, &nPath );
	int nNodes = Cudd_DagSize( Zdd );

	printf( "%-8.8s", Name );

	if ( nPath < 100000000 )
		printf( " Cube=%8.0f", nPath );
	else
		printf( " Cube=%8.1e", nPath );

	if ( nLiterals < 100000000 )
		printf( "  Lits=%8.0f", nLiterals );
	else
		printf( "  Lits=%8.1e", nLiterals );

	printf( "  Node=%6d", nNodes );

	if ( nLiterals/nNodes < 1000000 )
		printf( "  Dens=%8.1f", nLiterals/nNodes );
	else
		printf( "  Dens=%8.1e", nLiterals/nNodes );

	if ( Time != -1 )
		printf( "  Time= %.2f", (float)Time/(float)CLOCKS_PER_SEC );

	printf( "\n" );
	fflush( stdout );
}

/**Function********************************************************************

  Synopsis    [Prints statistics for a ZDD.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void printStatsBdd( char* Name, DdManager* dd, DdNode* Bdd, long Time )
{
	double nMinterms = Cudd_CountMinterm( dd, Bdd, dd->size );
	double nPath = Cudd_CountPath( Bdd );
	int nNodes = Cudd_DagSize( Bdd );

	printf( "%-8.8s", Name );

	if ( nMinterms < 100000000 )
		printf( " Mint=%8.0f", nMinterms );
	else
		printf( " Mint=%8.1e", nMinterms );

	if ( nPath < 100000000 )
		printf( "  Path=%8.0f", nPath );
	else
		printf( "  Path=%8.1e", nPath );

	printf( "  Node=%6d", nNodes );

	if ( nMinterms/nNodes < 1000000 )
		printf( "  Dens=%8.1f", nMinterms/nNodes );
	else
		printf( "  Dens=%8.1e", nMinterms/nNodes );

	if ( Time != -1 )
		printf( "  Time= %.2f", (float)Time/(float)CLOCKS_PER_SEC );

	printf( "\n" );
	fflush( stdout );
}

/*---------------------------------------------------------------------------*/
/* Definition of internal Functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Performs the recursive step of prime computation.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode* extraZddPrimes( DdManager *dd, DdNode* F )
{
	DdNode *zRes;

	if ( F == Cudd_Not( dd->one ) )
		return dd->zero;
	if ( F == dd->one )
		return dd->one;

    /* check cache */
    zRes = cuddCacheLookup1Zdd(dd, extraZddPrimes, F);
    if (zRes)
    	return(zRes);
	{
		/* temporary variables */
		DdNode *bF01, *zP0, *zP1;
		/* three components of the prime set */
		DdNode *zResE, *zResP, *zResN;
		int fIsComp = Cudd_IsComplement( F );

		/* find cofactors of F */
		DdNode * bF0 = Cudd_NotCond( Cudd_E( F ), fIsComp );
		DdNode * bF1 = Cudd_NotCond( Cudd_T( F ), fIsComp );

		/* find the intersection of cofactors */
		bF01 = cuddBddAndRecur( dd, bF0, bF1 );
		if ( bF01 == NULL ) return NULL;
		cuddRef( bF01 );

		/* solve the problems for cofactors */
		zP0 = extraZddPrimes( dd, bF0 );
		if ( zP0 == NULL )
		{
			Cudd_RecursiveDeref( dd, bF01 );
			return NULL;
		}
		cuddRef( zP0 );

		zP1 = extraZddPrimes( dd, bF1 );
		if ( zP1 == NULL )
		{
			Cudd_RecursiveDeref( dd, bF01 );
			Cudd_RecursiveDerefZdd( dd, zP0 );
			return NULL;
		}
		cuddRef( zP1 );

		/* check for local unateness */
		if ( bF01 == bF0 )       /* unate increasing */
		{
			/* intersection is useless */
			cuddDeref( bF01 );
			/* the primes of intersection are the primes of F0 */
			zResE = zP0;
			/* there are no primes with negative var */
			zResN = dd->zero;
			cuddRef( zResN );
			/* primes with positive var are primes of F1 that are not primes of F01 */
			zResP = cuddZddDiff( dd, zP1, zP0 );
			if ( zResP == NULL )  
			{
				Cudd_RecursiveDerefZdd( dd, zResE );
				Cudd_RecursiveDerefZdd( dd, zResN );
				Cudd_RecursiveDerefZdd( dd, zP1 );
				return NULL;
			}
			cuddRef( zResP );
			Cudd_RecursiveDerefZdd( dd, zP1 );
		}
		else if ( bF01 == bF1 ) /* unate decreasing */
		{
			/* intersection is useless */
			cuddDeref( bF01 );
			/* the primes of intersection are the primes of F1 */
			zResE = zP1;
			/* there are no primes with positive var */
			zResP = dd->zero;
			cuddRef( zResP );
			/* primes with negative var are primes of F0 that are not primes of F01 */
			zResN = cuddZddDiff( dd, zP0, zP1 );
			if ( zResN == NULL )  
			{
				Cudd_RecursiveDerefZdd( dd, zResE );
				Cudd_RecursiveDerefZdd( dd, zResP );
				Cudd_RecursiveDerefZdd( dd, zP0 );
				return NULL;
			}
			cuddRef( zResN );
			Cudd_RecursiveDerefZdd( dd, zP0 );
		}
		else /* not unate */
		{
			/* primes without the top var are primes of F10 */
			zResE = extraZddPrimes( dd, bF01 );
			if ( zResE == NULL )  
			{
				Cudd_RecursiveDerefZdd( dd, bF01 );
				Cudd_RecursiveDerefZdd( dd, zP0 );
				Cudd_RecursiveDerefZdd( dd, zP1 );
				return NULL;
			}
			cuddRef( zResE );
			Cudd_RecursiveDeref( dd, bF01 );

			/* primes with the negative top var are those of P0 that are not in F10 */
			zResN = cuddZddDiff( dd, zP0, zResE );
			if ( zResN == NULL )  
			{
				Cudd_RecursiveDerefZdd( dd, zResE );
				Cudd_RecursiveDerefZdd( dd, zP0 );
				Cudd_RecursiveDerefZdd( dd, zP1 );
				return NULL;
			}
			cuddRef( zResN );
			Cudd_RecursiveDerefZdd( dd, zP0 );

			/* primes with the positive top var are those of P1 that are not in F10 */
			zResP = cuddZddDiff( dd, zP1, zResE );
			if ( zResP == NULL )  
			{
				Cudd_RecursiveDerefZdd( dd, zResE );
				Cudd_RecursiveDerefZdd( dd, zResN );
				Cudd_RecursiveDerefZdd( dd, zP1 );
				return NULL;
			}
			cuddRef( zResP );
			Cudd_RecursiveDerefZdd( dd, zP1 );
		}

		zRes = extraComposeCover( dd, zResN, zResP, zResE, Cudd_Regular(F)->index );
		if ( zRes == NULL ) return NULL;

		/* insert the result into cache */
		cuddCacheInsert1(dd, extraZddPrimes, F, zRes);
		return zRes;
	}
} /* end of extraZddPrimes */


/**Function********************************************************************

  Synopsis    [Performs the recursive step of dominating row computation.]

  Description [F is the bdd of the function to be covered. P is the zdd of 
  primes to cover it. Returns the maximal classes of row dominance relation.]

  SideEffects []

  SeeAlso     []

******************************************************************************/

DdNode* extraZddMaxRowDom( DdManager *dd, DdNode* F, DdNode* P )
{
	DdNode *zRes;

	if ( F == Cudd_Not( dd->one ) || P == dd->zero ) /* nothing to cover */
		return dd->zero;
	if ( P == dd->one )
		return dd->one;

	/* if ( F == dd->one ), P should contain {()} because P covers F
	   It is possible to check whether P - {()} covers dd->one
	   If yes, call procedure recursively; if no, the result is {()}
	   However, it is expensive to compute P - {()} */

    /* check cache */
    zRes = cuddCacheLookup2Zdd(dd, extraZddMaxRowDom, F, P);
    if (zRes)
    	return(zRes);
	else
	{
		/* get the top levels for F and P */
		int TopLevelF = cuddI(dd,Cudd_Regular(F)->index);
		int TopLevelP = dd->permZ[ P->index ] >> 1;
		/* find cofactors of F */
		int fIsComp   = Cudd_IsComplement( F );
		DdNode *bF0   = Cudd_NotCond( Cudd_E( F ), fIsComp );
		DdNode *bF1   = Cudd_NotCond( Cudd_T( F ), fIsComp );

		if ( TopLevelF < TopLevelP )
		{
			DdNode* bF01 = cuddBddAndRecur( dd, Cudd_Not(bF0), Cudd_Not(bF1) );
			if ( bF01 == NULL ) 
				return NULL;
			cuddRef( bF01 );
			bF01 = Cudd_Not( bF01 );

			zRes = extraZddMaxRowDom( dd, bF01, P );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDeref( dd, bF01 );
				return NULL;
			}
			cuddRef( zRes );
			Cudd_RecursiveDeref( dd, bF01 );
			cuddDeref( zRes );
		}
		else /* if ( TopLevelF >= TopLevelP ) */
		{
			/* cofactors of the cover (zCof0 - neg var; zCof1 - pos var; zCof2 - no var) */
			DdNode *zCof0, *zCof1, *zCof2;
			DdNode *bNegSumX0, *bNegSumX1, *bNegSumX2, *bGenX01;
			DdNode *zMax0, *zMax1, *zMax2;

			extraDecomposeCover( dd, P, &zCof0, &zCof1, &zCof2 );

			/* minterms not covered by cubes of P containing X' */
			bNegSumX0 = cuddMakeBddFromZddCover( dd, zCof0 );
			if ( bNegSumX0 == NULL ) 
				return NULL;
			cuddRef( bNegSumX0 );
			bNegSumX0 = Cudd_Not( bNegSumX0 );

			/* minterms not covered by cubes of P containing X */
			bNegSumX1 = cuddMakeBddFromZddCover( dd, zCof1 );
			if ( bNegSumX1 == NULL ) 
			{
				Cudd_RecursiveDeref( dd, bNegSumX0 );
				return NULL;
			}
			cuddRef( bNegSumX1 );
			bNegSumX1 = Cudd_Not( bNegSumX1 );

			if ( TopLevelF == TopLevelP )
			{
				DdNode *bGenX0, *bGenX1;

				/* minterms with X' covered only by products in P without var */
				bGenX0 = cuddBddAndRecur( dd, bF0, bNegSumX0 );
				if ( bGenX0 == NULL ) 
				{
					Cudd_RecursiveDeref( dd, bNegSumX0 );
					Cudd_RecursiveDeref( dd, bNegSumX1 );
					return NULL;
				}
				cuddRef( bGenX0 );
				Cudd_RecursiveDeref( dd, bNegSumX0 );

				/* minterms with X covered only by products in P without var */
				bGenX1 = cuddBddAndRecur( dd, bF1, bNegSumX1 );
				if ( bGenX1 == NULL ) 
				{
					Cudd_RecursiveDeref( dd, bNegSumX1 );
					Cudd_RecursiveDeref( dd, bGenX0 );
					return NULL;
				}
				cuddRef( bGenX1 );
				Cudd_RecursiveDeref( dd, bNegSumX1 );

				/* all minterms covered only by products in P without var */
				bGenX01 = cuddBddAndRecur( dd, Cudd_Not(bGenX0), Cudd_Not(bGenX1) );
				if ( bGenX01 == NULL ) 
				{
					Cudd_RecursiveDeref( dd, bGenX0 );
					Cudd_RecursiveDeref( dd, bGenX1 );
					return NULL;
				}
				cuddRef( bGenX01 );
				bGenX01 = Cudd_Not( bGenX01 );
				Cudd_RecursiveDeref( dd, bGenX0 );
				Cudd_RecursiveDeref( dd, bGenX1 );
				/* referenced is bGenX01 */
			}
			else
			{ /* the same result computed simpler */
				DdNode* bNegSum = cuddBddAndRecur( dd, Cudd_Not(bNegSumX0), Cudd_Not(bNegSumX1) );
				if ( bNegSum == NULL ) 
				{
					Cudd_RecursiveDeref( dd, bNegSumX0 );
					Cudd_RecursiveDeref( dd, bNegSumX1 );
					return NULL;
				}
				cuddRef( bNegSum );
				bNegSum = Cudd_Not( bNegSum );
				Cudd_RecursiveDeref( dd, bNegSumX0 );
				Cudd_RecursiveDeref( dd, bNegSumX1 );

				/* all minterms covered only by products in P without var */
				bGenX01 = cuddBddAndRecur( dd, F, bNegSum );
				if ( bGenX01 == NULL ) 
				{
					Cudd_RecursiveDeref( dd, bNegSum );
					return NULL;
				}
				cuddRef( bGenX01 );
				Cudd_RecursiveDeref( dd, bNegSum );

				/* correct cofactors */
				bF0 = F;
				bF1 = F;
				/* referenced is bGenX01 */
			}

			/* the set of signature cubes containing neither X nor X' */
			zMax2 = extraZddMaxRowDom( dd, bGenX01, zCof2 );
			if ( zMax2 == NULL ) 
			{
				Cudd_RecursiveDeref( dd, bGenX01 );
				return NULL;
			}
			cuddRef( zMax2 );
			Cudd_RecursiveDeref( dd, bGenX01 );

			/* find the area covered NOT covered by these cubes */
			bNegSumX2 = cuddMakeBddFromZddCover( dd, zMax2 );
			if ( bNegSumX2 == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zMax2 );
				return NULL;
			}
			cuddRef( bNegSumX2 );
			bNegSumX2 = Cudd_Not( bNegSumX2 );


			/* compute sets of signature cubes containing X' and X */
			{
				DdNode *bGenX0, *zPeX0;
				/* minterms with X' not covered by cubes without this var */
				bGenX0 = cuddBddAndRecur( dd, bF0, bNegSumX2 );
				if ( bGenX0 == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zMax2 );
					Cudd_RecursiveDeref( dd, bNegSumX2 );
					return NULL;
				}
				cuddRef( bGenX0 );

				/* set of cubes that may cover these minterms */
				zPeX0 = cuddZddUnion( dd, zCof0, zCof2 );
				if ( zPeX0 == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zMax2 );
					Cudd_RecursiveDeref( dd, bNegSumX2 );
					Cudd_RecursiveDeref( dd, bGenX0 );
					return NULL;
				}
				cuddRef( zPeX0 );

				/* find the cubes the cover these minterms */
				/* no need to compute zMax0 = NotSubSet( zMax0, zMax2 ) */
				zMax0 = extraZddMaxRowDom( dd, bGenX0, zPeX0 );
				if ( zMax0 == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zMax2 );
					Cudd_RecursiveDeref( dd, bNegSumX2 );
					Cudd_RecursiveDeref( dd, bGenX0 );
					Cudd_RecursiveDerefZdd( dd, zPeX0 );
					return NULL;
				}
				cuddRef( zMax0 );
				Cudd_RecursiveDeref( dd, bGenX0 );
				Cudd_RecursiveDerefZdd( dd, zPeX0 );
			}
			{
				DdNode *bGenX1, *zPeX1;
				/* minterms with X not covered by cubes without this var */
				bGenX1 = cuddBddAndRecur( dd, bF1, bNegSumX2 );
				if ( bGenX1 == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zMax0 );
					Cudd_RecursiveDerefZdd( dd, zMax2 );
					Cudd_RecursiveDeref( dd, bNegSumX2 );
					return NULL;
				}
				cuddRef( bGenX1 );
				Cudd_RecursiveDeref( dd, bNegSumX2 );

				/* set of cubes that may cover these minterms */
				zPeX1 = cuddZddUnion( dd, zCof1, zCof2 );
				if ( zPeX1 == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zMax0 );
					Cudd_RecursiveDerefZdd( dd, zMax2 );
					Cudd_RecursiveDeref( dd, bGenX1 );
					return NULL;
				}
				cuddRef( zPeX1 );

				/* find the cubes the cover these minterms */
				/* no need to compute zMax1 = NotSubSet( zMax1, zMax2 ) */
				zMax1 = extraZddMaxRowDom( dd, bGenX1, zPeX1 );
				if ( zMax1 == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zMax0 );
					Cudd_RecursiveDerefZdd( dd, zMax2 );
					Cudd_RecursiveDeref( dd, bGenX1 );
					Cudd_RecursiveDerefZdd( dd, zPeX1 );
					return NULL;
				}
				cuddRef( zMax1 );
				Cudd_RecursiveDeref( dd, bGenX1 );
				Cudd_RecursiveDerefZdd( dd, zPeX1 );
			}

			zRes = extraComposeCover( dd, zMax0, zMax1, zMax2, P->index >> 1 );
			if ( zRes == NULL ) return NULL;
		}

		/* insert the result into cache */
		cuddCacheInsert2(dd, extraZddMaxRowDom, F, P, zRes);
		return zRes;
	}

} /* end of extraZddMaxRowDom */


/**Function********************************************************************

  Synopsis    [Performs the recursive step of dominating column computation.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode* extraZddMaxColDomAux( DdManager *dd, DdNode* Q, DdNode* P )
{
	DdNode *res;

#ifdef SECOND_OPTION
	/* select relevant cubes from P */
	DdNode *newP = extraZddSubSet( dd, P, Q );
	if ( newP == NULL ) 
		return NULL;
	cuddRef( newP );
	/* compute the result */
	res = extraZddMaxColDom( dd, Q, newP );
	if ( res == NULL )
	{
		Cudd_RecursiveDeref( dd, newP );
		cuddRef( res );
		return NULL;
	}
	cuddRef( res );
	Cudd_RecursiveDeref( dd, newP );
	cuddDeref( res );
#else
	/* compute the result */
	res = extraZddMaxColDom( dd, Q, P );
	if ( res == NULL ) 
		return NULL;
#endif

	return res;

} /* end of extraZddMaxColDomAux */


/**Function********************************************************************

  Synopsis    [Performs the recursive step of dominating column computation.]

  Description [Q is the zdd of the signature cubes. P is the zdd of primes. 
  Returns the maximal classes of the column dominance relation.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode* extraZddMaxColDom( DdManager *dd, DdNode* Q, DdNode* P )
{
	DdNode *zRes;
	if ( Q == P )
		return P;  /* return P because it is already maximal */
	if ( Q == dd->zero || P == dd->zero ) /* nothing to cover */
		return dd->zero;
	if ( P == dd->one && Extra_zddEmptyBelongs( dd, Q ) )
		/* if P is {()} and Q contains {()}, return {()} */
		return dd->one;

#ifndef SECOND_OPTION
	/* should never happen if P contains only cubes covering at least one cube in Q */
	if ( Q == dd->one )
		return dd->zero;
#endif

    /* check cache */
    zRes = cuddCacheLookup2Zdd(dd, extraZddMaxColDom, Q, P);
    if (zRes)
    	return(zRes);
	else
	{
		/* get the top levels of Q and P */
		int TopLevelQ = (cuddIZ(dd,Q->index)) >> 1;
		int TopLevelP = (cuddIZ(dd,P->index)) >> 1;

#ifndef SECOND_OPTION
		/* should never happen if P contains only cubes covering at least one cube in Q */
		/* otherwise, we look for products that cover at least one cube in Q */
		if ( TopLevelQ > TopLevelP )
		{
			DdNode *zP0, *zP1, *zP2;
			extraDecomposeCover( dd, P, &zP0, &zP1, &zP2 );
			zRes = extraZddMaxColDom( dd, Q, zP2 );
			if ( zRes == NULL ) return NULL;
		}
		else
#endif
		{
			DdNode *zQ0, *zQ1, *zQ2;
			DdNode *zP0, *zP1, *zP2;
			DdNode *zPeQ0,  *zPeQ1, *zPeQ2;
			DdNode *zGenQ2, *zGenP2, *zTemp, *zTemp1, *zTemp2;
			DdNode *zMax0, *zMax1, *zMax2;

			/* below X is the top var of Q */
			/* decompose Q */
			extraDecomposeCover( dd, Q, &zQ0, &zQ1, &zQ2 );
		
			/* decompose P */
			if ( TopLevelQ == TopLevelP )
				extraDecomposeCover( dd, P, &zP0, &zP1, &zP2 );
			else
			{
				zP0 = dd->zero;
				zP1 = dd->zero;
				zP2 = P;
			}

			/* cubes of zP2 that cover at least one cube in Q with X' */
			zPeQ0 = extraZddSubSet( dd, zP2, zQ0 );
			if ( zPeQ0 == NULL ) 
				return NULL;
			cuddRef( zPeQ0 );

			/* cubes of zP2 that cover at least one cube in Q with X */
			zPeQ1 = extraZddSubSet( dd, zP2, zQ1 );
			if ( zPeQ1 == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zPeQ0 );
				return NULL;
			}
			cuddRef( zPeQ1 );

			/* cubes of zP2 that cover at least one cube in Q without X */
			zPeQ2 = extraZddSubSet( dd, zP2, zQ2 );
			if ( zPeQ2 == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zPeQ0 );
				Cudd_RecursiveDerefZdd( dd, zPeQ1 );
				return NULL;
			}
			cuddRef( zPeQ2 );

			/* zGenP2 is the set of cubes in P whose dominator does not contain X or X'
			   a cube is in zGenP2 if it covers a cube in zQ2 OR 
			   if it covers a cube in zQ0 AND a cube in zQ1
			   such cube can only be in zP2 */
			zTemp = cuddZddIntersect( dd, zPeQ0, zPeQ1 );
			if ( zTemp == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zPeQ0 );
				Cudd_RecursiveDerefZdd( dd, zPeQ1 );
				Cudd_RecursiveDerefZdd( dd, zPeQ2 );
				return NULL;
			}
			cuddRef( zTemp );

			zGenP2 = cuddZddUnion( dd, zPeQ2, zTemp );
			if ( zGenP2 == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zPeQ0 );
				Cudd_RecursiveDerefZdd( dd, zPeQ1 );
				Cudd_RecursiveDerefZdd( dd, zPeQ2 );
				Cudd_RecursiveDerefZdd( dd, zTemp );
				return NULL;
			}
			cuddRef( zGenP2 );
			Cudd_RecursiveDerefZdd( dd, zPeQ2 );
			Cudd_RecursiveDerefZdd( dd, zTemp );

			/* zGenQ2 contains at least all the cubes in Q covered by zGenP2 */
			zTemp = cuddZddUnion( dd, zQ0, zQ1 );
			if ( zTemp == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zPeQ0 );
				Cudd_RecursiveDerefZdd( dd, zPeQ1 );
				Cudd_RecursiveDerefZdd( dd, zGenP2 );
				return NULL;
			}
			cuddRef( zTemp );

			zGenQ2 = cuddZddUnion( dd, zQ2, zTemp );
			if ( zGenQ2 == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zPeQ0 );
				Cudd_RecursiveDerefZdd( dd, zPeQ1 );
				Cudd_RecursiveDerefZdd( dd, zGenP2 );
				Cudd_RecursiveDerefZdd( dd, zTemp );
				return NULL;
			}
			cuddRef( zGenQ2 );
			Cudd_RecursiveDerefZdd( dd, zTemp );

			/* find the set of dominators of cubes in zGenP2 */
			zMax2 = extraZddMaxColDom( dd, zGenQ2, zGenP2 );
			if ( zMax2 == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zPeQ0 );
				Cudd_RecursiveDerefZdd( dd, zPeQ1 );
				Cudd_RecursiveDerefZdd( dd, zGenP2 );
				Cudd_RecursiveDerefZdd( dd, zGenQ2 );
				return NULL;
			}
			cuddRef( zMax2 );
			Cudd_RecursiveDerefZdd( dd, zGenQ2 );
			/* at this point, zPeQ0, zPeQ1, zGenP2, zMax2 are referenced */

			{
				DdNode *zGenP0;
				/* zGenP0 is the set of cubes in P whose dominator contains X'
				   it contains all the cubes in zP0 (X' removed) (zTemp1)
				   and all the cubes in zP2 containing only cubes in zQ0
				   and not covered by any cubes in zTemp1 (zTemp2) */
#ifdef SECOND_OPTION
				zTemp1 = zP0;
				cuddRef( zTemp1 );
#else
				zTemp1 = extraZddSubSet( dd, zP0, zQ0 );
				if ( zTemp1 == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zPeQ0 );
					Cudd_RecursiveDerefZdd( dd, zPeQ1 );
					Cudd_RecursiveDerefZdd( dd, zGenP2 );
					Cudd_RecursiveDerefZdd( dd, zMax2 );
					return NULL;
				}
				cuddRef( zTemp1 );
#endif
				zTemp = cuddZddDiff( dd, zPeQ0, zGenP2 );
				if ( zTemp == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zPeQ0 );
					Cudd_RecursiveDerefZdd( dd, zPeQ1 );
					Cudd_RecursiveDerefZdd( dd, zGenP2 );
					Cudd_RecursiveDerefZdd( dd, zMax2 );
					Cudd_RecursiveDerefZdd( dd, zTemp1 );
					return NULL;
				}
				cuddRef( zTemp );
				Cudd_RecursiveDerefZdd( dd, zPeQ0 );

				zTemp2 = extraZddNotSupSet( dd, zTemp, zTemp1 );
				if ( zTemp2 == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zPeQ1 );
					Cudd_RecursiveDerefZdd( dd, zGenP2 );
					Cudd_RecursiveDerefZdd( dd, zMax2 );
					Cudd_RecursiveDerefZdd( dd, zTemp );
					Cudd_RecursiveDerefZdd( dd, zTemp1 );
					return NULL;
				}
				cuddRef( zTemp2 );
				Cudd_RecursiveDerefZdd( dd, zTemp );

				zGenP0 = cuddZddUnion( dd, zTemp1, zTemp2 );
				if ( zGenP0 == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zPeQ1 );
					Cudd_RecursiveDerefZdd( dd, zGenP2 );
					Cudd_RecursiveDerefZdd( dd, zMax2 );
					Cudd_RecursiveDerefZdd( dd, zTemp1 );
					Cudd_RecursiveDerefZdd( dd, zTemp2 );
					return NULL;
				}
				cuddRef( zGenP0 );
				Cudd_RecursiveDerefZdd( dd, zTemp1 );
				Cudd_RecursiveDerefZdd( dd, zTemp2 );

				/* zMax0 is the set of dominators of cubes in zGenP0
				   not covered by cubes in zMax2 */
				zTemp = extraZddMaxColDom( dd, zQ0, zGenP0 );
				if ( zTemp == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zPeQ1 );
					Cudd_RecursiveDerefZdd( dd, zGenP0 );
					Cudd_RecursiveDerefZdd( dd, zGenP2 );
					Cudd_RecursiveDerefZdd( dd, zMax2 );
					return NULL;
				}
				cuddRef( zTemp );
				Cudd_RecursiveDerefZdd( dd, zGenP0 );

				zMax0 = extraZddNotSupSet( dd, zTemp, zMax2 );
				if ( zMax0 == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zPeQ1 );
					Cudd_RecursiveDerefZdd( dd, zGenP2 );
					Cudd_RecursiveDerefZdd( dd, zMax2 );
					Cudd_RecursiveDerefZdd( dd, zTemp );
					return NULL;
				}
				cuddRef( zMax0 );
				Cudd_RecursiveDerefZdd( dd, zTemp );
				/* referenced are zPeQ1, zGenP2, zMax0, zMax2 */
			}
			{
				DdNode *zGenP1;

#ifdef SECOND_OPTION
				zTemp1 = zP1;
				cuddRef( zTemp1 );
#else
				zTemp1 = extraZddSubSet( dd, zP1, zQ1 );
				if ( zTemp1 == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zPeQ1 );
					Cudd_RecursiveDerefZdd( dd, zGenP2 );
					Cudd_RecursiveDerefZdd( dd, zMax0 );
					Cudd_RecursiveDerefZdd( dd, zMax2 );
					return NULL;
				}
				cuddRef( zTemp1 );
#endif
				zTemp = cuddZddDiff( dd, zPeQ1, zGenP2 );
				if ( zTemp == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zPeQ1 );
					Cudd_RecursiveDerefZdd( dd, zGenP2 );
					Cudd_RecursiveDerefZdd( dd, zMax0 );
					Cudd_RecursiveDerefZdd( dd, zMax2 );
					Cudd_RecursiveDerefZdd( dd, zTemp1 );
					return NULL;
				}
				cuddRef( zTemp );
				Cudd_RecursiveDerefZdd( dd, zPeQ1 );
				Cudd_RecursiveDerefZdd( dd, zGenP2 );

				zTemp2 = extraZddNotSupSet( dd, zTemp, zTemp1 );
				if ( zTemp2 == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zMax0 );
					Cudd_RecursiveDerefZdd( dd, zMax2 );
					Cudd_RecursiveDerefZdd( dd, zTemp );
					Cudd_RecursiveDerefZdd( dd, zTemp1 );
					return NULL;
				}
				cuddRef( zTemp2 );
				Cudd_RecursiveDerefZdd( dd, zTemp );

				zGenP1 = cuddZddUnion( dd, zTemp1, zTemp2 );
				if ( zGenP1 == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zMax0 );
					Cudd_RecursiveDerefZdd( dd, zMax2 );
					Cudd_RecursiveDerefZdd( dd, zTemp1 );
					Cudd_RecursiveDerefZdd( dd, zTemp2 );
					return NULL;
				}
				cuddRef( zGenP1 );
				Cudd_RecursiveDerefZdd( dd, zTemp1 );
				Cudd_RecursiveDerefZdd( dd, zTemp2 );

				zTemp = extraZddMaxColDom( dd, zQ1, zGenP1 );
				if ( zTemp == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zMax0 );
					Cudd_RecursiveDerefZdd( dd, zMax2 );
					Cudd_RecursiveDerefZdd( dd, zGenP1 );
					return NULL;
				}
				cuddRef( zTemp );
				Cudd_RecursiveDerefZdd( dd, zGenP1 );

				zMax1 = extraZddNotSupSet( dd, zTemp, zMax2 );
				if ( zMax1 == NULL ) 
				{
					Cudd_RecursiveDerefZdd( dd, zMax0 );
					Cudd_RecursiveDerefZdd( dd, zMax2 );
					Cudd_RecursiveDerefZdd( dd, zTemp );
					return NULL;
				}
				cuddRef( zMax1 );
				Cudd_RecursiveDerefZdd( dd, zTemp );
			}

			zRes = extraComposeCover( dd, zMax0, zMax1, zMax2, Q->index >> 1 );
			if ( zRes == NULL ) return NULL;
		}

		/* insert the result into cache */
		cuddCacheInsert2(dd, extraZddMaxColDom, Q, P, zRes);
		return zRes;
	}
} /* end of extraZddMaxColDom */

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/* ============================================================== */
/* HASH TABLE data structure and variables */
/* TODO: add dynamic allocation of the hash table */
#define TABLESIZE 61111
struct hash_table
{
	DdNode* F;
	int cRun;
	double nPaths;
	double nLits;
};
static int HashSuccess = 0;
static int HashFailure = 0;
static int g_FlagHashEnable = 1;
/* ============================================================== */

/**Function********************************************************************

  Synopsis    [Performs the recursive step of counting literals/cubes in a cover.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
double extraZddLitCountTotal( DdManager* dd, const DdNode* Cover, double* nPaths )
// counts the number of path and the number of literals 
// (the number occurences of variables in all paths of the Zdd)
{
	static struct hash_table HTableCount[TABLESIZE];

    int HashValue;
    if ( Cover == dd->zero )
	{
		*nPaths = 0.0;
        return 0.0;
	}
	else if ( Cover == dd->one )
	{
		*nPaths = 1.0;
        return 0.0;
	}
	else
	{
		double nPaths0, nPaths1, nLits0, nLits1, Result;

		// check cache for results
		if ( g_FlagHashEnable )
		{
			HashValue = (unsigned)Cover % TABLESIZE;
			if ( HTableCount[HashValue].F == Cover && HTableCount[HashValue].cRun == s_RunCounter )
			{
				HashSuccess++;
				*nPaths = HTableCount[HashValue].nPaths;
				return HTableCount[HashValue].nLits;
			}
			HashFailure++;
		}
		nLits0 = extraZddLitCountTotal( dd, Cudd_E( Cover ), &nPaths0 );
		nLits1 = extraZddLitCountTotal( dd, Cudd_T( Cover ), &nPaths1 );

		// add all the paths
		*nPaths = nPaths0 + nPaths1;
		// add all the literals + the literals introduced by this node
		Result = nLits0 + nLits1 + nPaths1;

			// insert into cache
		if ( g_FlagHashEnable )
		{
			HTableCount[HashValue].F = (DdNode*)Cover;
			HTableCount[HashValue].cRun = s_RunCounter;
			HTableCount[HashValue].nPaths = *nPaths;
			HTableCount[HashValue].nLits = Result;
		}
		return Result;
	}
}
