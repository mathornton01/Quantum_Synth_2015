/**CFile***********************************************************************

  FileName    [aMisc.c]

  PackageName [extra]

  Synopsis    [Experimental version of some ADD operators.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_addCountConst()
				<li> Extra_addCountConstArray()
				<li> Extra_addFindMinArray()
				<li> Extra_addFindMaxArray()
				<li> Extra_addDetectAdd()
				</ul>
			Internal procedures included in this module:
				<ul>
				</ul>
			Static procedures included in this module:
				<ul>
				</ul>
	      ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$aMisc.c, v.1.2, August 20, 2001, alanmi $]

******************************************************************************/

#include "util.h"
#include "cuddInt.h"
#include "time.h"
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

  Synopsis    [Counts the number of different constant nodes of the ADD.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Extra_addCountConst(
  DdManager * dd,
  DdNode * aFunc )
{
	DdGen * genDD;
	DdNode * node;
	int Count = 0;

	/* iterate through the nodes */
	Cudd_ForeachNode( dd, aFunc, genDD, node )
	{
		if ( Cudd_IsConstant(node) )
			Count++;
	}
	return Count;
}


/**Function********************************************************************

  Synopsis    [Counts the number of different constant nodes of the array of ADDs.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Extra_addCountConstArray(
  DdManager * dd,
  DdNode ** paFuncs,
  int nFuncs )
{
	st_table * Visited;
	st_generator * gen;
	DdNode * aNode;
	int i, Count = 0;
	assert( nFuncs > 0 );

	// start the table
	Visited = st_init_table( st_ptrcmp, st_ptrhash );

	// collect the unique nodes in the shared ADD
	for ( i = 0; i < nFuncs; i++ )
		cuddCollectNodes( Cudd_Regular( paFuncs[i] ), Visited );

	// count the unique terminals
	st_foreach_item( Visited, gen, (char**)&aNode, NULL) 
	{
		if ( Cudd_IsConstant(aNode) )
			Count++;
	}

	// deref the visited table
	st_free_table( Visited );
	return Count;
}

/**Function********************************************************************

  Synopsis    [Return the encoded set of absolute values of the constant nodes of an ADD.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * Extra_bddAddConstants(
  DdManager * dd,
  DdNode * aFunc )
{
	st_table * Visited;
	st_generator * gen;
	DdNode * aNode, * bRes, * bTemp, * bCube;
	int nVars;

	// start the table
	Visited = st_init_table( st_ptrcmp, st_ptrhash );

	// collect the unique nodes in the ADD
	cuddCollectNodes( Cudd_Regular(aFunc), Visited );

	nVars = Cudd_SupportSize( dd, aFunc );

	// start the set of encoded constants
	bRes = b0;  Cudd_Ref( bRes );
	st_foreach_item( Visited, gen, (char**)&aNode, NULL) 
	{
		if ( Cudd_IsConstant(aNode) )
		{
			bCube = Extra_bddBitsToCube( dd, (int)ddAbs(cuddV(aNode)), nVars, NULL );  Cudd_Ref( bCube );
			bRes  = Cudd_bddOr( dd, bCube, bTemp = bRes );                            Cudd_Ref( bRes );
			Cudd_RecursiveDeref( dd, bTemp );
			Cudd_RecursiveDeref( dd, bCube );
		}
	}

	// deref the visited table
	st_free_table( Visited );

	Cudd_Deref( bRes );
	return bRes;
} /* end of Extra_bddAddConstants */


/**Function********************************************************************

  Synopsis    [Restructure the ADD by replacing negative terminals with their abs value.]

  Description []

  SideEffects []

  SeeAlso     [Cudd_addMonadicApply]

******************************************************************************/
DdNode * Cudd_addAbs( DdManager * dd, DdNode * f )
{
	if ( cuddIsConstant( f ) )
	{
		CUDD_VALUE_TYPE value = cuddV( f );
		if ( value < 0 )	value = -value;
		return cuddUniqueConst( dd, value );
	}
	return ( NULL );

}								/* end of Cudd_addAbs */


/**Function********************************************************************

  Synopsis    [Finds the minimum discriminant of the array of ADDs.]

  Description [Returns a pointer to a constant ADD.]

  SideEffects [None]

******************************************************************************/
DdNode * Extra_addFindMinArray( DdManager * dd, DdNode ** paFuncs, int nFuncs )
{
	DdNode * aRes, * aCur;
	int i;
	assert( nFuncs > 0 );

	aRes = Cudd_addFindMin( dd, paFuncs[0] );
	for ( i = 1; i < nFuncs; i++ )
	{
		aCur = Cudd_addFindMin( dd, paFuncs[i] );
		aRes = ( cuddV(aRes) <= cuddV(aCur) ) ? aRes: aCur;
	}

	return aRes;
}  /* end of Extra_addFindMinArray */


/**Function********************************************************************

  Synopsis    [Finds the maximum discriminant of the array of ADDs.]

  Description [Returns a pointer to a constant ADD.]

  SideEffects [None]

******************************************************************************/
DdNode * Extra_addFindMaxArray( DdManager * dd, DdNode ** paFuncs, int nFuncs )
{
	DdNode * aRes, * aCur;
	int i;
	assert( nFuncs > 0 );

	aRes = Cudd_addFindMax( dd, paFuncs[0] );
	for ( i = 1; i < nFuncs; i++ )
	{
		aCur = Cudd_addFindMax( dd, paFuncs[i] );
		aRes = ( cuddV(aRes) >= cuddV(aCur) ) ? aRes: aCur;
	}

	return aRes;
}  /* end of Extra_addFindMinArray */


/**Function********************************************************************

  Synopsis    [Absolute value of an ADD.]

  Description [Absolute value of an ADD. Returns NULL if not a terminal case; 
  abs(f) otherwise.]

  SideEffects [None]

  SeeAlso     [Cudd_addMonadicApply]

******************************************************************************/
DdNode * Extra_addAbs( DdManager * dd, DdNode * f )
{
	if ( cuddIsConstant( f ) )
	{
		CUDD_VALUE_TYPE value = ( cuddV( f ) > 0 )? cuddV( f ): -cuddV( f );
		DdNode *res = cuddUniqueConst( dd, value );
		return ( res );
	}
	return ( NULL );

} /* end of Extra_addAbs */


/**Function********************************************************************

  Synopsis    [Determines whether this is an ADD or a BDD.]

  Description [Returns 1, if the function is an ADD; 0, if the function is a BDD; -1, if unknown.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Extra_addDetectAdd( DdManager * dd, DdNode * Func )
{
	int RetValue;

	if ( Cudd_IsComplement(Func) )
		return 0;
	if ( Func == dd->one )
		return -1;
	if ( Func == dd->zero )
		return 1;

	RetValue = Extra_addDetectAdd( dd, cuddE(Func) );
	if ( RetValue == 1 )
		return 1;
	if ( RetValue == 0 )
		return 0;
	return Extra_addDetectAdd( dd, cuddT(Func) );
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/
