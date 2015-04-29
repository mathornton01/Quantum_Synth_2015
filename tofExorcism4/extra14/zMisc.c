/**CFile***********************************************************************

  FileName    [zMisc.c]

  PackageName [extra]

  Synopsis    [Miscelleneous operators.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_zddCombination()
				<li> Extra_zddUniverse()
				<li> Extra_zddTuples()
				<li> Extra_zddSinglesToComb()
				<li> Extra_zddMaximum()
				<li> Extra_zddMinimum()
				<li> Extra_zddRandomSet()
				<li> Extra_zddCofactor0()
				<li> Extra_zddCofactor1()
				</ul>
			Internal procedures included in this module:
				<ul>
				<li> extraZddCombination()
				<li> extraZddUniverse()
				<li> extraZddTuples()
				<li> extraZddTuplesFromBdd()
				<li> extraZddSinglesToComb()
				<li> extraZddMaximum()
				<li> extraZddCofactor0()
				<li> extraZddCofactor1()
				</ul>
			Static procedures included in this module:
				<ul>
				<li> ddSupportStep()
				<li> ddClearFlag()
				<li> zddCountVars()
				</ul>
	      ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$zMisc.h, v.1.2, July 16, 2001, alanmi $]

******************************************************************************/

#include "util.h"
#include "time.h"
#include "cuddInt.h"
#include "extra.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

const int s_LargeNum = 1000000;

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

// traversal structure for the enumeration of paths in the ZDD
// (can be used for both printing and ZDD remapping)
typedef struct
{
	FILE *fp;       /* the file to write graph */
	DdManager *dd;  /* the pointer to the DD manager */
	int Lev[2];     /* levels of the first and the second variables on the path */
	int nIter;      /* the counter of cubes traversed */
} userdata;

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
static int zddCountVars ARGS((DdManager *dd, DdNode* S));

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Creates ZDD of the combination containing given variables.]

  Description [Creates ZDD of the combination containing given variables.
               VarValues contains 1 for a variable that belongs to the 
               combination and 0 for a varible that does not belong. 
			   nVars is number of ZDD variables in the array.]

  SideEffects [New ZDD variables are created if indices of the variables
               present in the combination are larger than the currently
			   allocated number of ZDD variables.]

  SeeAlso     []

******************************************************************************/
DdNode * Extra_zddCombination( 
  DdManager *dd, 
  int* VarValues, 
  int nVars )
{
    DdNode	*res;
    do {
	dd->reordered = 0;
	res = extraZddCombination(dd, VarValues, nVars);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddCombination */


/**Function********************************************************************

  Synopsis    [Creates all possible combinations of given variables.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_zddUniverse( 
  DdManager * dd,    /* the manager */
  DdNode * VarSet)   /* the variables whose universe it to be built */
{
    DdNode	*res;
    do {
	dd->reordered = 0;
	res = extraZddUniverse(dd, VarSet);
    } while (dd->reordered == 1);
    return(res);

} /* end of Extra_zddUniverse */


/**Function********************************************************************

  Synopsis    [Builds ZDD representing the set of fixed-size variable tuples.]

  Description [Creates ZDD of all combinations of variables in Support that
  is represented by a ZDD.]

  SideEffects [New ZDD variables are created if indices of the variables
               present in the combination are larger than the currently
			   allocated number of ZDD variables.]

  SeeAlso     []

******************************************************************************/
DdNode* Extra_zddTuples( 
  DdManager * dd,   /* the DD manager */
  int K,            /* the number of variables in tuples */
  DdNode * zVarsN)   /* the set of all variables represented as a ZDD */
{
    DdNode	*zRes;
    int		autoDynZ;

    autoDynZ = dd->autoDynZ;
    dd->autoDynZ = 0;

    do {
		/* transform the numeric arguments (K) into a DdNode* argument;
		 * this allows us to use the standard internal CUDD cache */
		DdNode *zVarSet = zVarsN, *zVarsK = zVarsN;
		int     nVars = 0, i;

		/* determine the number of variables in VarSet */
		while ( zVarSet != z1 )
		{
			nVars++;
			/* make sure that the VarSet is a cube */
			if ( cuddE( zVarSet ) != z0 )
				return NULL;
			zVarSet = cuddT( zVarSet );
		}
		/* make sure that the number of variables in VarSet is less or equal 
		   that the number of variables that should be present in the tuples
		*/
		if ( K > nVars )
			return NULL;

		/* the second argument in the recursive call stannds for <n>;
		/* reate the first argument, which stands for <k> 
		 * as when we are talking about the tuple of <k> out of <n> */
		for ( i = 0; i < nVars-K; i++ )
			zVarsK = cuddT( zVarsK );

		dd->reordered = 0;
		zRes = extraZddTuples(dd, zVarsK, zVarsN );

    } while (dd->reordered == 1);
    dd->autoDynZ = autoDynZ;
    return zRes;

} /* end of Extra_zddTuples */


/**Function********************************************************************

  Synopsis    [Builds ZDD representing the set of fixed-size variable tuples.]

  Description [Creates ZDD of all combinations of variables in Support that
  is represented by a BDD.]

  SideEffects [New ZDD variables are created if indices of the variables
               present in the combination are larger than the currently
			   allocated number of ZDD variables.]

  SeeAlso     []

******************************************************************************/
DdNode* Extra_zddTuplesFromBdd( 
  DdManager * dd,   /* the DD manager */
  int K,            /* the number of variables in tuples */
  DdNode * bVarsN)   /* the set of all variables represented as a BDD */
{
    DdNode	*zRes;
    int		autoDynZ;

    autoDynZ = dd->autoDynZ;
    dd->autoDynZ = 0;

    do {
		/* transform the numeric arguments (K) into a DdNode* argument;
		 * this allows us to use the standard internal CUDD cache */
		DdNode *bVarSet = bVarsN, *bVarsK = bVarsN;
		int     nVars = 0, i;

		/* determine the number of variables in VarSet */
		while ( bVarSet != b1 )
		{
			nVars++;
			/* make sure that the VarSet is a cube */
			if ( cuddE( bVarSet ) != b0 )
				return NULL;
			bVarSet = cuddT( bVarSet );
		}
		/* make sure that the number of variables in VarSet is less or equal 
		   that the number of variables that should be present in the tuples
		*/
		if ( K > nVars )
			return NULL;

		/* the second argument in the recursive call stannds for <n>;
		/* reate the first argument, which stands for <k> 
		 * as when we are talking about the tuple of <k> out of <n> */
		for ( i = 0; i < nVars-K; i++ )
			bVarsK = cuddT( bVarsK );

		dd->reordered = 0;
		zRes = extraZddTuplesFromBdd(dd, bVarsK, bVarsN );

    } while (dd->reordered == 1);
    dd->autoDynZ = autoDynZ;
    return zRes;

} /* end of Extra_zddTuplesFromBdd */


/**Function********************************************************************

  Synopsis    [Converts the set of singleton combinations into one combination.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode* Extra_zddSinglesToComb( 
  DdManager * dd,   /* the DD manager */
  DdNode * Singles) /* the set of singleton combinations */
{
    DdNode	*res;
    int		autoDynZ;

    autoDynZ = dd->autoDynZ;
    dd->autoDynZ = 0;

    do {
	dd->reordered = 0;
	res = extraZddSinglesToComb(dd, Singles);
    } while (dd->reordered == 1);
    dd->autoDynZ = autoDynZ;
    return(res);

} /* end of Extra_zddSinglesToComb */


/**Function********************************************************************

  Synopsis    [Returns all combinations containing the maximum number of elements.]

  Description []

  SideEffects []

  SeeAlso     [Extra_zddMaximal]

******************************************************************************/
DdNode* Extra_zddMaximum( 
  DdManager * dd,   /* the DD manager */
  DdNode * S,       /* the set of combinations whose maximum is sought */
  int * nVars)      /* if non-zero, stores the number of elements in max combinations */
{
    DdNode	*res;
	int NumVars;
    do {
		dd->reordered = 0;
		res = extraZddMaximum(dd, S, &NumVars);
    } while (dd->reordered == 1);

	/* count the number of elements in max combinations */
	if ( nVars )
		*nVars = NumVars;
    return(res);

} /* end of Extra_zddMaximum */


/**Function********************************************************************

  Synopsis    [Returns all combinations containing the minimum number of elements.]

  Description []

  SideEffects []

  SeeAlso     [Extra_zddMinimal]

******************************************************************************/
DdNode* Extra_zddMinimum( 
  DdManager * dd,   /* the DD manager */
  DdNode * S,       /* the set of combinations whose maximum is sought */
  int * nVars)      /* if non-zero, stores the number of elements in max combinations */
{
    DdNode	*res;
	int NumVars;
    do {
		dd->reordered = 0;
		res = extraZddMinimum(dd, S, &NumVars);
    } while (dd->reordered == 1);

	/* count the number of elements in max combinations */
	if ( nVars )
		*nVars = NumVars;
    return(res);

} /* end of Extra_zddMinimum */


/**Function********************************************************************

  Synopsis    [Generates a random set of combinations.]

  Description [Given a set of n elements, each of which is encoded using one
               ZDD variable, this function generates a random set of k subsets 
			   (combinations of elements) with density d. Assumes that k and n
			   are positive integers. Returns NULL if density is less than 0.0 
			   or more than 1.0.]

  SideEffects [Allocates new ZDD variables if their current number is less than n.]

  SeeAlso     []

******************************************************************************/
DdNode* Extra_zddRandomSet( 
  DdManager * dd,   /* the DD manager */
  int n,            /* the number of elements */
  int k,            /* the number of combinations (subsets) */
  double d)         /* average density of elements in combinations */
{
	DdNode *Result, *TempComb, *Aux;
	int c, v, Limit, *VarValues;

	/* sanity check the parameters */
	if ( n <= 0 || k <= 0 || d < 0.0 || d > 1.0 )
		return NULL;

	/* allocate temporary storage for variable values */
	VarValues = ALLOC( int, n );
	if (VarValues == NULL) 
	{
		dd->errorCode = CUDD_MEMORY_OUT;
		return NULL;
	}

	/* start the new set */
	Result = dd->zero;
	Cudd_Ref( Result );

	/* seed random number generator */
	Cudd_Srandom( time(NULL) );
//	Cudd_Srandom( 4 );
	/* determine the limit below which var belongs to the combination */
	Limit = (int)(d * 2147483561.0);

	/* add combinations one by one */
	for ( c = 0; c < k; c++ )
	{
		for ( v = 0; v < n; v++ )
			if ( Cudd_Random() <= Limit )
				VarValues[v] = 1;
			else
				VarValues[v] = 0;

		TempComb = Extra_zddCombination( dd, VarValues, n );
		Cudd_Ref( TempComb );

		/* make sure that this combination is not already in the set */
		if ( c )
		{ /* at least one combination is already included */

			Aux = Cudd_zddDiff( dd, Result, TempComb );
			Cudd_Ref( Aux );
			if ( Aux != Result )
			{
				Cudd_RecursiveDerefZdd( dd, Aux );
				Cudd_RecursiveDerefZdd( dd, TempComb );
				c--;
				continue;
			}
			else 
			{ /* Aux is the same node as Result */
				Cudd_Deref( Aux );
			}
		}

		Result = Cudd_zddUnion( dd, Aux = Result, TempComb );
		Cudd_Ref( Result );
		Cudd_RecursiveDerefZdd( dd, Aux );
		Cudd_RecursiveDerefZdd( dd, TempComb );
	}

	FREE( VarValues );
	Cudd_Deref( Result );
	return Result;

} /* end of Extra_zddRandomSet */


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Performs the reordering-sensitive step of Extra_zddCombination().]

  Description [Generates in a bottom-up fashion ZDD for one combination 
               whose var values are given in the array VarValues. If necessary,
			   creates new variables on the fly.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * extraZddCombination(
  DdManager* dd, 
  int* VarValues, 
  int nVars )
{
	int lev, index;
	DdNode *zRes, *zTemp;

	/* transform the combination from the array VarValues into a ZDD cube. */
    zRes = dd->one;
    cuddRef(zRes);

	/*  go through levels starting bottom-up and create nodes 
	 *  if these variables are present in the comb
	 */
    for (lev = nVars - 1; lev >= 0; lev--) 
	{ 
		index = (lev >= dd->sizeZ) ? lev : dd->invpermZ[lev];
		if (VarValues[index] == 1) 
		{
			/* compose zRes with ZERO for the given ZDD variable */
			zRes = cuddZddGetNode( dd, index, zTemp = zRes, dd->zero );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zTemp );
				return NULL;
			}
			cuddRef( zRes );
			cuddDeref( zTemp );
		}
	}
	cuddDeref( zRes );
	return zRes;

} /* end of extraZddCombination */



/**Function********************************************************************

  Synopsis    [Performs the reordering-sensitive step of Extra_zddUniverse().]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * extraZddUniverse(
  DdManager * dd, 
  DdNode * VarSet)
{
	DdNode *zRes, *zPart;
    statLine(dd); 

	if ( VarSet == dd->zero )
	{
		printf("\nextraZddUniverse(): Singles is not a ZDD!\n");
		return NULL;
	}
	if ( VarSet == dd->one )
		return dd->one;

    /* check cache */
    zRes = cuddCacheLookup1Zdd(dd, extraZddUniverse, VarSet);
    if (zRes)
		return(zRes);

	/* make sure that VarSet is a single combination */
	if ( cuddE( VarSet ) != dd->zero )
	{
		printf("\nextraZddUniverse(): VarSet is not a single combination!\n");
		return NULL;
	}

	/* solve the problem recursively */
	zPart = extraZddUniverse( dd, cuddT( VarSet ) );
	if ( zPart == NULL ) 
		return NULL;
	cuddRef( zPart );

	/* create new node with this variable */
	zRes = cuddZddGetNode( dd, VarSet->index, zPart, zPart );
	if ( zRes == NULL ) 
	{
		Cudd_RecursiveDerefZdd( dd, zPart );
		return NULL;
	}
	cuddDeref( zPart );

    cuddCacheInsert1(dd, extraZddUniverse, VarSet, zRes);
	return zRes;

} /* end of extraZddUniverse */


/**Function********************************************************************

  Synopsis    [Performs the reordering-sensitive step of Extra_zddTuples().]

  Description [Generates in a bottom-up fashion ZDD for all combinations
               composed of k variables out of variables belonging to Support.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode* extraZddTuples( 
  DdManager * dd,   /* the DD manager */
  DdNode * zVarsK,   /* the number of variables in tuples */
  DdNode * zVarsN)   /* the set of all variables */
{
	DdNode *zRes, *zRes0, *zRes1;
    statLine(dd); 

	/* terminal cases */
/*	if ( k < 0 || k > n )
 *		return dd->zero;
 *	if ( n == 0 )
 *		return dd->one; 
 */
	if ( cuddIZ( dd, zVarsK->index ) < cuddIZ( dd, zVarsN->index ) )
		return z0;
	if ( zVarsN == z1 )
		return z1;

    /* check cache */
    zRes = cuddCacheLookup2Zdd(dd, extraZddTuples, zVarsK, zVarsN);
    if (zRes)
    	return(zRes);

	/* ZDD in which this variable is 0 */
/*	zRes0 = extraZddTuples( dd, k,     n-1 ); */
	zRes0 = extraZddTuples( dd, zVarsK, cuddT(zVarsN) );
	if ( zRes0 == NULL ) 
		return NULL;
	cuddRef( zRes0 );

	/* ZDD in which this variable is 1 */
/*	zRes1 = extraZddTuples( dd, k-1,          n-1 ); */
	if ( zVarsK == z1 )
	{
		zRes1 = z0;
		cuddRef( zRes1 );
	}
	else
	{
		zRes1 = extraZddTuples( dd, cuddT(zVarsK), cuddT(zVarsN) );
		if ( zRes1 == NULL ) 
		{
			Cudd_RecursiveDerefZdd( dd, zRes0 );
			return NULL;
		}
		cuddRef( zRes1 );
	}

	/* compose Res0 and Res1 with the given ZDD variable */
	zRes = cuddZddGetNode( dd, zVarsN->index, zRes1, zRes0 );
	if ( zRes == NULL ) 
	{
		Cudd_RecursiveDerefZdd( dd, zRes0 );
		Cudd_RecursiveDerefZdd( dd, zRes1 );
		return NULL;
	}
	cuddDeref( zRes0 );
	cuddDeref( zRes1 );

	/* insert the result into cache */
	cuddCacheInsert2(dd, extraZddTuples, zVarsK, zVarsN, zRes);
	return zRes;

} /* end of extraZddTuples */


/**Function********************************************************************

  Synopsis    [Performs the reordering-sensitive step of Extra_zddTupleFromBdd().]

  Description [Generates in a bottom-up fashion ZDD for all combinations
               composed of k variables out of variables belonging to Support.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode* extraZddTuplesFromBdd( 
  DdManager * dd,   /* the DD manager */
  DdNode * bVarsK,   /* the number of variables in tuples */
  DdNode * bVarsN)   /* the set of all variables */
{
	DdNode *zRes, *zRes0, *zRes1;
    statLine(dd); 

	/* terminal cases */
/*	if ( k < 0 || k > n )
 *		return dd->zero;
 *	if ( n == 0 )
 *		return dd->one; 
 */
	if ( cuddI( dd, bVarsK->index ) < cuddI( dd, bVarsN->index ) )
		return z0;
	if ( bVarsN == b1 )
		return z1;

    /* check cache */
    zRes = cuddCacheLookup2Zdd(dd, extraZddTuplesFromBdd, bVarsK, bVarsN);
    if (zRes)
    	return(zRes);

	/* ZDD in which this variable is 0 */
/*	zRes0 = extraZddTuplesFromBdd( dd, k,     n-1 ); */
	zRes0 = extraZddTuplesFromBdd( dd, bVarsK, cuddT(bVarsN) );
	if ( zRes0 == NULL ) 
		return NULL;
	cuddRef( zRes0 );

	/* ZDD in which this variable is 1 */
/*	zRes1 = extraZddTuplesFromBdd( dd, k-1,          n-1 ); */
	if ( bVarsK == b1 )
	{
		zRes1 = z0;
		cuddRef( zRes1 );
	}
	else
	{
		zRes1 = extraZddTuplesFromBdd( dd, cuddT(bVarsK), cuddT(bVarsN) );
		if ( zRes1 == NULL ) 
		{
			Cudd_RecursiveDerefZdd( dd, zRes0 );
			return NULL;
		}
		cuddRef( zRes1 );
	}

	/* compose Res0 and Res1 with the given ZDD variable */
	zRes = cuddZddGetNode( dd, 2*bVarsN->index, zRes1, zRes0 );
	if ( zRes == NULL ) 
	{
		Cudd_RecursiveDerefZdd( dd, zRes0 );
		Cudd_RecursiveDerefZdd( dd, zRes1 );
		return NULL;
	}
	cuddDeref( zRes0 );
	cuddDeref( zRes1 );

	/* insert the result into cache */
	cuddCacheInsert2(dd, extraZddTuplesFromBdd, bVarsK, bVarsN, zRes);
	return zRes;

} /* end of extraZddTuplesFromBdd */

/**Function********************************************************************

  Synopsis    [Performs the reordering-sensitive step of Extra_zddSinglesToComb().]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * extraZddSinglesToComb(
  DdManager * dd,   /* the DD manager */
  DdNode * Singles) /* the set of singleton combinations */
{
	DdNode *zRes, *zPart;
    statLine(dd); 

	if ( Singles == dd->one )
	{
		printf("\nextraZddSinglesToComb(): Singles is not a ZDD!\n");
		return NULL;
	}
	if ( Singles == dd->zero )
		return dd->one;

    /* check cache */
    zRes = cuddCacheLookup1Zdd(dd, extraZddSinglesToComb, Singles);
    if (zRes)
		return(zRes);

	/* make sure that this is the set of singletons */
	if ( cuddT( Singles ) != dd->one )
	{
		printf("\nextraZddSinglesToComb(): Singles is not a set of singletons!\n");
		return NULL;
	}

	/* solve the problem recursively */
	zPart = extraZddSinglesToComb( dd, cuddE( Singles ) );
	if ( zPart == NULL )
		return NULL;
	cuddRef( zPart );

	/* create new node with this variable */
	/* compose Res0 and Res1 with the given ZDD variable */
	zRes = cuddZddGetNode( dd, Singles->index, zPart, dd->zero );
	if ( zRes == NULL ) 
	{
		Cudd_RecursiveDerefZdd( dd, zPart );
		return NULL;
	}
	cuddDeref( zPart );

    cuddCacheInsert1(dd, extraZddSinglesToComb, Singles, zRes);
	return zRes;

} /* end of extraZddSinglesToComb */


/**Function********************************************************************

  Synopsis    [Performs the reordering-sensitive step of Extra_zddMaximum().]

  Description [Generates in a bottom-up fashion ZDD for all combinations
  that have maximum cardinality in the cover.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * extraZddMaximum(
  DdManager * dd,   /* the DD manager */
  DdNode * S,       /* the set of combinations */
  int * nVars)      /* the pointer where cardinality goes */ 
{
	DdNode *zRes;
	DdNode*(*cacheOp)(DdManager*,DdNode*);
    statLine(dd); 

	/* terminal cases */
	if ( S == dd->zero || S == dd->one )
	{
		*nVars = 0;
		return S;
	}

    /* check cache */
	cacheOp = (DdNode*(*)(DdManager*,DdNode*))extraZddMaximum;

    zRes = cuddCacheLookup1Zdd(dd, cacheOp, S);
    if (zRes)
	{
		*nVars = zddCountVars( dd, zRes );
		return(zRes);
	}
	else
	{
		DdNode *zSet0, *zSet1;
		int     Card0,  Card1;

		/* solve the else problem recursively */
		zSet0 = extraZddMaximum( dd, cuddE( S ), &Card0 );
		if ( zSet0 == NULL )
			return NULL;
		cuddRef( zSet0 );

		/* solve the then problem recursively */
		zSet1 = extraZddMaximum( dd, cuddT( S ), &Card1 );
		if ( zSet1 == NULL )
		{
			Cudd_RecursiveDerefZdd( dd, zSet0 );
			return NULL;
		}
		cuddRef( zSet1 );

		/* compare the solutions */
		if ( Card0 == Card1 + 1 ) 
		{  /* both sets are good */

			/* create new node with this variable */
			zRes = cuddZddGetNode( dd, S->index, zSet1, zSet0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zSet0 );
				Cudd_RecursiveDerefZdd( dd, zSet1 );
				return NULL;
			}
			cuddDeref( zSet0 );
			cuddDeref( zSet1 );

			*nVars = Card0;
		}
		else if ( Card0 > Card1 + 1 )
		{  /* take only zSet0 */

			Cudd_RecursiveDerefZdd( dd, zSet1 );
			zRes = zSet0;
			cuddDeref( zRes );

			*nVars = Card0;
		}
		else /* if ( Card0 < Card1 + 1 ) */
		{  /* take only zSet1 */

			Cudd_RecursiveDerefZdd( dd, zSet0 );

			/* create new node with this variable and empty else child */
			zRes = cuddZddGetNode( dd, S->index, zSet1, dd->zero );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zSet1 );
				return NULL;
			}
			cuddDeref( zSet1 );

			*nVars = Card1 + 1;
		}

		cuddCacheInsert1(dd, cacheOp, S, zRes);
		return zRes;
	}
} /* end of extraZddMaximum */


/**Function********************************************************************

  Synopsis    [Performs the reordering-sensitive step of Extra_zddMinimum().]

  Description [Generates in a bottom-up fashion ZDD for all combinations
  that have minimum cardinality in the cover.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * extraZddMinimum(
  DdManager * dd,   /* the DD manager */
  DdNode * S,       /* the set of combinations */
  int * nVars)      /* the pointer where cardinality goes */ 
{
	DdNode *zRes;
	DdNode*(*cacheOp)(DdManager*,DdNode*);
    statLine(dd); 

	/* terminal cases */
	if ( S == dd->zero )
	{
		*nVars = s_LargeNum;
		return S;
	}
	if ( S == dd->one )
	{
		*nVars = 0;
		return S;
	}

    /* check cache */
	cacheOp = (DdNode*(*)(DdManager*,DdNode*))extraZddMinimum;

    zRes = cuddCacheLookup1Zdd(dd, cacheOp, S);
    if (zRes)
	{
		*nVars = zddCountVars( dd, zRes );
		return(zRes);
	}
	else
	{
		DdNode *zSet0, *zSet1;
		int     Card0,  Card1;

		/* solve the else problem recursively */
		zSet0 = extraZddMinimum( dd, cuddE( S ), &Card0 );
		if ( zSet0 == NULL )
			return NULL;
		cuddRef( zSet0 );

		/* solve the then problem recursively */
		zSet1 = extraZddMinimum( dd, cuddT( S ), &Card1 );
		if ( zSet1 == NULL )
		{
			Cudd_RecursiveDerefZdd( dd, zSet0 );
			return NULL;
		}
		cuddRef( zSet1 );

		/* compare the solutions */
		if ( Card0 == Card1 + 1 ) 
		{  /* both sets are good */

			/* create new node with this variable */
			zRes = cuddZddGetNode( dd, S->index, zSet1, zSet0 );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zSet0 );
				Cudd_RecursiveDerefZdd( dd, zSet1 );
				return NULL;
			}
			cuddDeref( zSet0 );
			cuddDeref( zSet1 );

			*nVars = Card0;
		}
		else if ( Card0 < Card1 + 1 )
		{  /* take only zSet0 */

			Cudd_RecursiveDerefZdd( dd, zSet1 );
			zRes = zSet0;
			cuddDeref( zRes );

			*nVars = Card0;
		}
		else /* if ( Card0 > Card1 + 1 ) */
		{  /* take only zSet1 */

			Cudd_RecursiveDerefZdd( dd, zSet0 );

			/* create new node with this variable and empty else child */
			zRes = cuddZddGetNode( dd, S->index, zSet1, dd->zero );
			if ( zRes == NULL ) 
			{
				Cudd_RecursiveDerefZdd( dd, zSet1 );
				return NULL;
			}
			cuddDeref( zSet1 );

			*nVars = Card1 + 1;
		}

		cuddCacheInsert1(dd, cacheOp, S, zRes);
		return zRes;
	}
} /* end of extraZddMinimum */




/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Returns the number of elements in a randomly selected combination.]

  Description [Returns the number of elements in a randomly selected combination.
               This number may be useful if the set contains combinations of 
			   the same cardinality.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
static int 
zddCountVars( 
  DdManager *dd, /* the manager */
  DdNode* S)     /* the set */
{
	/* in a ZDD, positive edge never points to dd->zero */
	int Counter;
	assert( S != dd->zero );
	for ( Counter = 0; S != dd->one; Counter++, S = cuddT(S) );
	return Counter;
}
