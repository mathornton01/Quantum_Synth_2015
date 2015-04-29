/**CFile***********************************************************************

  FileName    [bTransfer.c]

  PackageName [extra]

  Synopsis    [Cross-manager transfer functions.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_TransferPermute();
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

  Revision    [$bTransfer.c, v.1.2, September 4, 2001, alanmi $]

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

static s_fAdd;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**Automaticstart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static DdNode * extraTransferPermuteRecur
ARGS((DdManager * ddS, DdManager * ddD, DdNode * f, st_table * table, int * Permute ));

static DdNode * extraTransferPermute
ARGS((DdManager * ddS, DdManager * ddD, DdNode * f, int * Permute));

/**Automaticend***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Convert a {A,B}DD from a manager to another with variable remapping.]

  Description [Convert a {A,B}DD from a manager to another one. The orders of the
  variables in the two managers may be different. Returns a
  pointer to the {A,B}DD in the destination manager if successful; NULL
  otherwise. The i-th entry in the array Permute tells what is the index
  of the i-th variable from the old manager in the new manager.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode * Extra_TransferPermute( DdManager * ddSource, DdManager * ddDestination, DdNode * f, int * Permute )
{
	DdNode *res;
	// determine whether this function is a BDD or an ADD
	s_fAdd = Extra_addDetectAdd( ddSource, f );
	do
	{
		ddDestination->reordered = 0;
		res = extraTransferPermute( ddSource, ddDestination, f, Permute );
	}
	while ( ddDestination->reordered == 1 );
	return ( res );

}								/* end of Extra_TransferPermute */


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Convert a BDD from a manager to another one.]

  Description [Convert a BDD from a manager to another one. Returns a
  pointer to the BDD in the destination manager if successful; NULL
  otherwise.]

  SideEffects [None]

  SeeAlso     [Extra_TransferPermute]

******************************************************************************/
DdNode * extraTransferPermute( DdManager * ddS, DdManager * ddD, DdNode * f, int * Permute )
{
	DdNode *res;
	st_table *table = NULL;
	st_generator *gen = NULL;
	DdNode *key, *value;

	table = st_init_table( st_ptrcmp, st_ptrhash );
	if ( table == NULL )
		goto failure;
	res = extraTransferPermuteRecur( ddS, ddD, f, table, Permute );
	if ( res != NULL )
		cuddRef( res );

	/* Dereference all elements in the table and dispose of the table.
	   ** This must be done also if res is NULL to avoid leaks in case of
	   ** reordering. */
	gen = st_init_gen( table );
	if ( gen == NULL )
		goto failure;
	while ( st_gen( gen, ( char ** ) &key, ( char ** ) &value ) )
	{
		Cudd_RecursiveDeref( ddD, value );
	}
	st_free_gen( gen );
	gen = NULL;
	st_free_table( table );
	table = NULL;

	if ( res != NULL )
		cuddDeref( res );
	return ( res );

  failure:
	if ( table != NULL )
		st_free_table( table );
	if ( gen != NULL )
		st_free_gen( gen );
	return ( NULL );

}								/* end of extraTransferPermute */


/**Function********************************************************************

  Synopsis    [Performs the recursive step of Extra_TransferPermute.]

  Description [Performs the recursive step of Extra_TransferPermute.
  Returns a pointer to the result if successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [extraTransferPermute]

******************************************************************************/
static DdNode * 
extraTransferPermuteRecur( 
  DdManager * ddS, 
  DdManager * ddD, 
  DdNode * f, 
  st_table * table, 
  int * Permute )
{
	DdNode *ft, *fe, *t, *e, *var, *res;
	DdNode *one, *zero;
	int index;
	int comple = 0;

	statLine( ddD );
	one = DD_ONE( ddD );
	comple = Cudd_IsComplement( f );

	/* Trivial cases. */
//	if ( Cudd_IsConstant( f ) )
//		return ( Cudd_NotCond( one, comple ) );


	// modifies to work with ADDs, too
	if ( f == ddS->zero )
		return ddD->zero;
	if ( f == ddS->one )
		return ddD->one;
	if ( f == Cudd_Not(ddS->one) )
		return Cudd_Not(ddD->one);


	/* Make canonical to increase the utilization of the cache. */
	f = Cudd_NotCond( f, comple );
	/* Now f is a regular pointer to a non-constant node. */

	/* Check the cache. */
	if ( st_lookup( table, ( char * ) f, ( char ** ) &res ) )
		return ( Cudd_NotCond( res, comple ) );

	/* Recursive step. */
	if ( Permute )
		index = Permute[f->index];
	else
		index = f->index;

	ft = cuddT( f );
	fe = cuddE( f );

	t = extraTransferPermuteRecur( ddS, ddD, ft, table, Permute );
	if ( t == NULL )
	{
		return ( NULL );
	}
	cuddRef( t );

	e = extraTransferPermuteRecur( ddS, ddD, fe, table, Permute );
	if ( e == NULL )
	{
		Cudd_RecursiveDeref( ddD, t );
		return ( NULL );
	}
	cuddRef( e );

	zero = Cudd_Not(ddD->one);
	var = cuddUniqueInter( ddD, index, one, zero );
	if ( var == NULL )
	{
		Cudd_RecursiveDeref( ddD, t );
		Cudd_RecursiveDeref( ddD, e );
		return ( NULL );
	}

	// if it is an ADD, call a specialized function
	if ( s_fAdd == 1 )
		res = extraAddIteRecurGeneral( ddD, var, t, e );
	else // it is a BDD
		res = cuddBddIteRecur( ddD, var, t, e );

	if ( res == NULL )
	{
		Cudd_RecursiveDeref( ddD, t );
		Cudd_RecursiveDeref( ddD, e );
		return ( NULL );
	}
	cuddRef( res );
	Cudd_RecursiveDeref( ddD, t );
	Cudd_RecursiveDeref( ddD, e );

	if ( st_add_direct( table, ( char * ) f, ( char * ) res ) ==
		 ST_OUT_OF_MEM )
	{
		Cudd_RecursiveDeref( ddD, res );
		return ( NULL );
	}
	return ( Cudd_NotCond( res, comple ) );

}								/* end of extraTransferPermuteRecur */

/*---------------------------------------------------------------------------*/
/* Definition of static Functions                                            */
/*---------------------------------------------------------------------------*/

