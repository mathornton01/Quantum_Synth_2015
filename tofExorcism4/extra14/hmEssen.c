/**CFile***********************************************************************

  FileName    [hmEssen.c]

  PackageName [Rondo Heuristic]

  Synopsis    [Heuristic SOP minimizer implementing Espresso strategy using ZDDs.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_zddEssential();
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

  Revision    [$hmEssen.c, v.1.2, January 16, 2001, alanmi $]

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

  Synopsis    [Computes all essential cubes belonging to the cover.]

  Description [Returns a pointer to the BDD if successful; NULL otherwise.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode	*
Extra_zddEssential(
  DdManager * dd,
  DdNode * zC,
  DdNode * bFuncOn)
{
	DdNode * bSingleCoveredArea;
	DdNode * bOnSetPart;
	DdNode * zEssentials;

	/* compute the part of the b-space covered by one cube only */
	bSingleCoveredArea = Extra_zddSingleCoveredArea( dd, zC );
	Cudd_Ref( bSingleCoveredArea );

	/* find the part of the on-set covered by essentials only */
	bOnSetPart = Cudd_bddAnd( dd, bFuncOn, bSingleCoveredArea );
	Cudd_Ref( bOnSetPart );
	Cudd_RecursiveDeref( dd, bSingleCoveredArea );

	/* compute the primes that overlap with this area */
	zEssentials = Extra_zddOverlappingWithArea( dd, zC, bOnSetPart );
	Cudd_Ref( zEssentials );
	Cudd_RecursiveDeref( dd, bOnSetPart );

	Cudd_Deref( zEssentials );
    return zEssentials;

} /* end of Extra_zddEssential */

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/
