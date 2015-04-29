/**CFile***********************************************************************

  FileName    [hmIrred.c]

  PackageName [Rondo Heuristic]

  Synopsis    [Heuristic SOP minimizer implementing Espresso strategy using ZDDs.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_zddMinimumIrredundant();
				<li> Extra_zddMinimumIrredundantExplicit();
 				<li> Extra_bddCombination();
				</ul>
			Internal procedures included in this module:
				<ul>
				<li> extraZddMinimumIrredundant();
				</ul>
			Static procedures included in this module:
				<ul>
				</ul>
	      ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$hmIrred.c, v.1.2, January 16, 2001, alanmi $]

******************************************************************************/


#include "extra.h"
#include "sparse.h"
#include "mincov.h"

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

/* these two static variables are used by the cube traversal routing to collect BDD for columns */
int       s_nColumns;
DdNode ** s_pbColumns;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static int rmZddEnumerateCubes( DdManager *dd, DdNode *zCover, int levPrev, int *VarValues );

/**AutomaticEnd***************************************************************/


/**Function********************************************************************

  Synopsis    [Computes a minimal irredundant cover of the prime cover zC.]

  Description [Returns a pointer to the BDD if successful; NULL otherwise.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode	*
Extra_zddIrredundant(
  DdManager * dd,
  DdNode * zC,
  DdNode * bFuncOn)
{
	DdNode * zEssentials, * zPartiallyRedundant, * zMinimalCover, * zResult;
	DdNode * bEssentials, * bImportantArea;

printf("zC = \n");
Cudd_zddPrintCover( dd, zC );

	/* compute the set of relatively essential primes */
	zEssentials = Extra_zddEssential( dd, zC, bFuncOn );
	Cudd_Ref( zEssentials );

	/* find the BDD for the are covered by these cubes */
	bEssentials = Extra_zddConvertToBdd( dd, zEssentials );
	Cudd_Ref( bEssentials );

	/* compute the part of the on-set not covered by relatively essential primes */
	bImportantArea = Cudd_bddAnd( dd, bFuncOn, Cudd_Not(bEssentials) );
	Cudd_Ref( bImportantArea );
	Cudd_IterDerefBdd( dd, bEssentials );

	/* compute the partially redundant primes */
	zPartiallyRedundant = Extra_zddOverlappingWithArea( dd, zC, bImportantArea );
	Cudd_Ref( zPartiallyRedundant );

printf("zPartiallyRedundant = \n");
Cudd_zddPrintCover( dd, zPartiallyRedundant );

	/* find the minimal cover */
	zMinimalCover = Extra_zddMinimumIrredundantExplicit( dd, zPartiallyRedundant, bImportantArea );
//	zMinimalCover = Extra_zddMinimumIrredundant( dd, zPartiallyRedundant );
	Cudd_Ref( zMinimalCover );
	Cudd_RecursiveDerefZdd( dd, zPartiallyRedundant );
	Cudd_IterDerefBdd( dd, bImportantArea );

	/* find the resulting cover: Essentials + MinimalCover */
	zResult = Cudd_zddUnion( dd, zEssentials, zMinimalCover );
	Cudd_Ref( zResult );
	Cudd_RecursiveDerefZdd( dd, zEssentials );
	Cudd_RecursiveDerefZdd( dd, zMinimalCover );

	Cudd_Deref( zResult );
    return zResult;

} /* end of Extra_zddIrredundant */

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
Extra_zddMinimumIrredundant(
  DdManager * dd,
  DdNode * zC)
{
    DdNode	*res;

    do {
	dd->reordered = 0;
	res = extraZddMinimumIrredundant(dd, zC);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddMinimumIrredundant */

/**Function********************************************************************

  Synopsis    [Computes the minimum cover of the partially redundant cubes]

  Description [This function implements the third subprocedure of the IRREDUNDANT step
  which is part of Espresso minimization procedure. It takes two argumenet: the ZDD of
  the relatively redundant cubes and the BDD of area that should be covered. After two 
  preceeding filtering steps, these two arguments satisfy the following property:
  each minterm in area is covered by at least two cubes in the cover. 
  Returns a pointer to the BDD if successful; NULL otherwise.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode	*
Extra_zddMinimumIrredundantExplicit(
  DdManager * dd,
  DdNode * zC,         /* the cover of partially redundant cubes */
  DdNode * bArea)      /* the critical area that should be covered */
{
	int nCubes, nCubesSol, nRows, nRowsPrev, c, r;
	int *pVarValues;
	DdNode ** pbRows, * zResult, * zTemp, * zColumn;
	DdNode * bIntersection;
	sm_matrix * M;
	sm_row * cover;
    sm_element *p;

	/* verify the arguments */
	{
		int VerifyFailed;
		DdNode	* bSingleCovered = Extra_zddSingleCoveredArea( dd, zC );
		Cudd_Ref( bSingleCovered );
		VerifyFailed = (int)( Cudd_bddIteConstant( dd, bArea, bSingleCovered, b1 ) != b1 );
		Cudd_IterDerefBdd( dd, bSingleCovered );
		if ( VerifyFailed )
		{
			printf("Extra_zddMinimumIrredundantExplicit(): Input argument verification failed\n");
			return NULL;
		}
	}

	if ( zC == z0 )
		return z0;

	/* find the number of cubes */
	nCubes = Cudd_zddCount( dd, zC );
	assert( nCubes > 2 );
	/* allocate storage for AreaBdds(rows) and CubeBdds(columns) */
	s_nColumns = 0;
	pbRows = (DdNode**) malloc( 10 * nCubes * sizeof(DdNode*) );
	s_pbColumns = (DdNode**) malloc( nCubes * sizeof(DdNode*) );
	pVarValues = (int*) malloc( dd->sizeZ * sizeof(int) );

	/* collect the CubeBdds into the storage */
	rmZddEnumerateCubes( dd, zC, -1, pVarValues );
	assert( s_nColumns == nCubes );

	/* start the sparse matrix and collect rows */
	nRows = 0;
	M = sm_alloc();
	for ( c = 0; c < nCubes; c++ )
	{
		/* memorize the number of rows before adding new */
		nRowsPrev = nRows;

		/* find those rows that need splitting */
		for ( r = 0; r < nRowsPrev; r++ )
		if ( Cudd_bddIteConstant( dd, pbRows[r], s_pbColumns[c], b0 ) != b0 ) 
		{ /* they intersect - determine intersection type */
			bIntersection = Cudd_bddAnd( dd, pbRows[r], s_pbColumns[c] );
			Cudd_Ref( bIntersection );
			assert( bIntersection != b0 );
			if ( bIntersection == pbRows[r] )
			{ /* the row is contained completely */
				/* add the column to this row in the sparse matrix */
				sm_insert( M, r, c );
				Cudd_IterDerefBdd( dd, bIntersection );
			}
			else
			{ /* the row are is split in two */
				/* leave in this rows the remainder of the area */
				pbRows[r] = Cudd_bddAnd( dd, pbRows[r], Cudd_Not(bIntersection) );
				Cudd_Ref( pbRows[r] );
				/* copy the row */
				sm_copy_row( M, nRows, sm_get_row(M,r) );
				/* add the column to this row */
				sm_insert( M, nRows, c );
				/* set the AreaBdd for this row */
				pbRows[nRows] = bIntersection;
				/* increment the row counter */
				nRows++;
				/* check whether the row counter is within the limit */
				if ( nRows == 10 * nCubes )
					assert( 0 );
			}
		}
	}
	printf("Extra_zddMinimumIrredundantExplicit(): Matrix size is %d x %d\n", nRows, s_nColumns );

	/* dereference the row BDDs */
	for ( r = 0; r < nRows; r++ )
		Cudd_IterDerefBdd( dd, pbRows[r] );

	/* solve the matrix */
	cover = sm_minimum_cover(M, NULL, /* heuristic */ 1, /* debug */ 0);

	/* remove those columns that are part of the solution */
	nCubesSol = 0;
    for( p = cover->first_col; p != 0; p = p->next_col ) 
	{
		assert( p->col_num < s_nColumns );
		Cudd_IterDerefBdd( dd, s_pbColumns[p->col_num] );
		s_pbColumns[p->col_num] = NULL;
		nCubesSol++;
	}
	assert( nCubesSol < nCubes );
	/* free the matrix and the solution */
	sm_free(M);
	sm_row_free(cover);

	/* collect the remaining columns */
	zResult = z0;
	Cudd_Ref( zResult );
	for ( c = 0; c < s_nColumns; c++ )
	if ( s_pbColumns[c] )
	{
		/* convert the column into ZDD */
		zColumn = Extra_zddConvertBddCubeIntoZddCube( dd, s_pbColumns[c] );
		Cudd_Ref( zColumn );
		Cudd_IterDerefBdd( dd, s_pbColumns[c] );

		Cudd_zddUnion( dd, zTemp = zResult, zColumn );
		Cudd_Ref( zResult );
		Cudd_RecursiveDerefZdd( dd, zTemp );
		Cudd_RecursiveDerefZdd( dd, zColumn );
	}

	/* subtract these columns from the given set */
	zResult = Cudd_zddDiff( dd, zC, zTemp = zResult );
	Cudd_Ref( zResult );
	Cudd_RecursiveDerefZdd( dd, zTemp );

	/* defer and return */
	cuddDeref( zResult );
	return zResult;
} /* end of Extra_zddMinimumIrredundantExplicit */

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis [Performs the recursive step of Extra_zddMinimumIrredundant.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode	*
extraZddMinimumIrredundant(
  DdManager * dd,
  DdNode * zC)
{
    DdNode	*zRes;
    statLine(dd); 

	/* if there is nothing in the cover, nothing is contained */
	if ( zC == z0 )  return z0;
	/* if the cover is the universe, return the universe */
	if ( zC == z1 )  return z1;

    /* check cache */
    zRes = cuddCacheLookup1Zdd(dd, extraZddMinimumIrredundant, zC);
    if (zRes)
		return zRes;
	else
	{
		DdNode *zC0, *zC1, *zC2;
		DdNode *zRes0, *zRes1, *zRes2, *zUnion, *zTemp;
		int TopZddVar;
		/* decompose the cover */
		extraDecomposeCover(dd, zC, &zC0, &zC1, &zC2);

		/* SUBPROBLEM 1: cover without literals */

		/* compute the irredundant cover for the sub-cover without literals */
		zRes2 = extraZddMinimumIrredundant( dd, zC2 );
		if ( zRes2 == NULL )
			return NULL;
		cuddRef( zRes2 );

		/* SUBPROBLEM 2: cover with the negative literal */

		/* remove the already covered cubes from the cover with the neg-literal */
		zRes0 = Extra_zddNotCoveredByCover( dd, zC0, zRes2 );
		if ( zRes0 == NULL )
		{
			Cudd_RecursiveDerefZdd(dd, zRes2);
			return NULL;
		}
		cuddRef( zRes0 );

		/* compute the irredundant cover for the sub-cover with the neg-literal */
		zRes0 = extraZddMinimumIrredundant( dd, zTemp = zRes0 );
		if ( zRes0 == NULL )
		{
			Cudd_RecursiveDerefZdd(dd, zTemp);
			Cudd_RecursiveDerefZdd(dd, zRes2);
			return NULL;
		}
		cuddRef( zRes0 );
		Cudd_RecursiveDerefZdd(dd, zTemp);

		/* SUBPROBLEM 3: cover with the positive literal */

		/* compute the union of these two covers: zRes0 + zRes2 */
		zUnion = cuddZddUnion( dd, zRes0, zRes2 );
		if ( zUnion == NULL )
		{
			Cudd_RecursiveDerefZdd(dd, zRes0);
			Cudd_RecursiveDerefZdd(dd, zRes2);
			return NULL;
		}
		cuddRef( zUnion );

		/* remove the already covered cubes from the cover with the pos-literal */
		zRes1 = Extra_zddNotCoveredByCover( dd, zC1, zUnion );
		if ( zRes1 == NULL )
		{
			Cudd_RecursiveDerefZdd(dd, zUnion);
			Cudd_RecursiveDerefZdd(dd, zRes0);
			Cudd_RecursiveDerefZdd(dd, zRes2);
			return NULL;
		}
		cuddRef( zRes1 );
		Cudd_RecursiveDerefZdd(dd, zUnion);

		/* compute the irredundant cover for the sub-cover with the neg-literal */
		zRes1 = extraZddMinimumIrredundant( dd, zTemp = zRes1 );
		if ( zRes1 == NULL )
		{
			Cudd_RecursiveDerefZdd(dd, zRes0);
			Cudd_RecursiveDerefZdd(dd, zTemp);
			Cudd_RecursiveDerefZdd(dd, zRes2);
			return NULL;
		}
		cuddRef( zRes1 );
		Cudd_RecursiveDerefZdd(dd, zTemp);

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

		/* insert the result into cache and return */
		cuddCacheInsert1(dd, extraZddMinimumIrredundant, zC, zRes);
		return zRes;
	}
} /* end of extraZddMinimumIrredundant */



/**Function********************************************************************

  Synopsis    [Builds the cube of variables in the given polarities.]

  Description [Assumes that each cell of the array VarValues contains a value, 
  0, 1, or 2, depending on whether the variable is included in negative or positive
  polatiry or missing in the cube. There should be as many entries in VarValues,
  as there are variables in the BDD part of the manager. Returns a pointer to the 
  cube represented as a BDD if successful; NULL otherwise.]

  SideEffects []

  SeeAlso     [Cudd_IndicesToCube, Cudd_bddComputeCube Extra_zddCombination]

******************************************************************************/
DdNode	*
Extra_bddCombination(
  DdManager * dd,
  int * VarValues)
{
	DdNode * bCube, * bTemp;
    int      i;

    bCube = dd->one;
    cuddRef(bCube);

    for (i = dd->size - 1; i >= 0; i--) 
	{
        if ( VarValues[i] == 0 ) 
		{
            bCube = Cudd_bddAnd(dd,Cudd_Not(Cudd_bddIthVar(dd,i)),bTemp = bCube);
			cuddRef(bCube);
			Cudd_IterDerefBdd(dd,bTemp);
		}
        else if ( VarValues[i] == 1 ) 
		{
            bCube = Cudd_bddAnd(dd,Cudd_bddIthVar(dd,i),bTemp = bCube);
			cuddRef(bCube);
			Cudd_IterDerefBdd(dd,bTemp);
		}
        else if ( VarValues[i] != 2 ) 
			assert( 0 );
	}

    cuddDeref(bCube);
    return(bCube);

} /* end of Cudd_bddCombination */


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


static int rmZddEnumerateCubes( DdManager *dd, DdNode *zCover, int levPrev, int *VarValues )
// enumerates the cubes of zCover and writes them into s_pVerValues
// upon reaching the bottom level, adds the cube to the PLA
// as long as the return value is 1, continues iteration; on 0, stops
{
	int lev;
	if ( zCover == dd->zero )
		return 1;
	if ( zCover == dd->one )
	{
		DdNode * bCube;
		int k;

		/* fill in the remaining variables */
		for ( lev = levPrev + 1; lev < dd->sizeZ; lev++ )
			VarValues[ dd->invpermZ[ lev ] ] = 0;

		/* convert ZDD var values to the BDD var values */
		for ( k = 0; k < dd->sizeZ/2; k++ )
			if ( VarValues[2*k] == 1 && VarValues[2*k+1] == 0 )
				VarValues[k] = 0;
			else if ( VarValues[2*k] == 0 && VarValues[2*k+1] == 1 )
				VarValues[k] = 1;
			else
				VarValues[k] = 2;

		bCube = Extra_bddCombination( dd, VarValues );
		Cudd_Ref( bCube );

		s_pbColumns[ s_nColumns++ ] = bCube;
		return 1;
	}
	else
	{
		/* find the level of the top variable */
		int TopLevel = dd->permZ[ zCover->index ];

		/* fill in the remaining variables */
		for ( lev = levPrev + 1; lev < TopLevel; lev++ )
			VarValues[ dd->invpermZ[ lev ] ] = 0;

		/* fill in this variable */
		/* the given var has negative polarity */
		VarValues[ zCover->index ] = 0;
		// iterate through the else branch
		if ( rmZddEnumerateCubes( dd, cuddE(zCover), TopLevel, VarValues ) == 0 )
			return 0;
		
		// the given var has positive polarity
		VarValues[ zCover->index ] = 1;
		// iterate through the then branch
		return rmZddEnumerateCubes( dd, cuddT(zCover), TopLevel, VarValues );
	}
} /* end of zddUnDecEnumeratePaths */

