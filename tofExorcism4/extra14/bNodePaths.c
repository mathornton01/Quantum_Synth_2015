/**CFile***********************************************************************

  FileName    [bMisc.c]

  PackageName [extra]

  Synopsis    [Experimental version of some BDD operators.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_bddNodePaths()
				<li> Extra_bddNodePathsUnderCut()
				</ul>
			Internal procedures included in this module:
				<ul>
				</ul>
			Static procedures included in this module:
				<ul>
				<li> CountNodeVisits_rec()
				<li> CollectNodesAndComputePaths_rec()
				</ul>
	      ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$bMisc.c, v.1.2, December 8, 2000, alanmi $]

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

typedef struct 
{
	int      nEdges;  // the number of in-coming edges of the node
	DdNode * bSum;    // the sum of paths of the incoming edges
} traventry;

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

int s_CutLevel;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

void CountNodeVisits_rec( DdManager * dd, DdNode * aFunc, st_table * Visited );
void CollectNodesAndComputePaths_rec( DdManager * dd, DdNode * aFunc, DdNode * bCube, st_table * Visited, st_table * CutNodes );

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [For each node in the ADD, computes the sum of paths leading to it from the root.]

  Description [The table returned contains the set of all nodes (including the terminals)
  and, for each ADD node, the BDD of the sum of paths leading to this node from the root
  The nodes are not referenced; the sums of paths in the table are referenced.]

  SideEffects []

  SeeAlso     [Extra_bddNodePathsUnderCut]

******************************************************************************/
st_table * Extra_addNodePaths( DdManager * dd, DdNode * aFunc )
{
	st_table * Visited;  // temporary table to remember the visited nodes
	st_table * CutNodes; // the result goes here
	st_table * Result;

	// the terminal cases
	assert( !Cudd_IsComplement(aFunc) );
	Result  = st_init_table(st_ptrcmp,st_ptrhash);
	if ( Cudd_IsConstant( aFunc ) )
	{
		if ( aFunc == a1 )
		{
			st_insert( Result, (char*)aFunc, (char*)b1 );
			Cudd_Ref( b1 );
		}
		else // if ( aFunc == a0 )
		{
			st_insert( Result, (char*)aFunc, (char*)b0 );
			Cudd_Ref( b0 );
		}
		return Result;
	}

	// Step 1: Start the table and collect information about the nodes above the cut 
	// this information tells how many edges point to each node
	CutNodes = st_init_table(st_ptrcmp,st_ptrhash);
	Visited  = st_init_table(st_ptrcmp,st_ptrhash);

	// visit the nodes
	s_CutLevel = CUDD_MAXINDEX;
	CountNodeVisits_rec( dd, aFunc, Visited );

	// Step 2: Traverse the BDD using the visited table and compute the sum of paths
	CollectNodesAndComputePaths_rec( dd, aFunc, b1, Visited, CutNodes );

	// the function is not a constant
	// the table of CutNodes contains only two nodes - terminals
	assert( st_count(CutNodes) == 2 );
	{
		st_generator * gen;
		DdNode * aNode;
		DdNode * bNode;
		st_foreach_item( CutNodes, gen, (char**)&aNode, (char**)&bNode) 
		{
			Cudd_RecursiveDeref( dd, bNode );
		}
	}
	st_free_table( CutNodes );


	// move the nodes from the table Visited to the table Result
	{
		st_generator * gen;
		DdNode * aNode;
		traventry * p;
		st_foreach_item( Visited, gen, (char**)&aNode, (char**)&p) 
		{
			st_insert( Result, (char*)aNode, (char*) p->bSum );
			// takes ref of p->bSum
			free( p );
		}
	}
	st_free_table( Visited );

	// return the table
	return Result;
}

/**Function********************************************************************

  Synopsis    [For each node in the BDD, computes the sum of paths leading to it from the root.]

  Description [The table returned contains the set of all nodes (including the terminals)
  and, for each node, the BDD of the sum of paths leading to this node from the root
  The nodes and the sums of paths in the table are both referenced.]

  SideEffects []

  SeeAlso     [Extra_bddNodePathsUnderCut]

******************************************************************************/
st_table * Extra_bddNodePaths( DdManager * dd, DdNode * bFunc )
{
	st_table * Visited;  // temporary table to remember the visited nodes
	st_table * CutNodes; // the result goes here
	st_table * Result;
	DdNode * aFunc;


	// the terminal cases
	Result  = st_init_table(st_ptrcmp,st_ptrhash);
	if ( Cudd_IsConstant( bFunc ) )
	{
		if ( bFunc == b1 )
		{
			st_insert( Result, (char*)bFunc, (char*)b1 );
			Cudd_Ref( b1 );
			Cudd_Ref( b1 );
		}
		else //if ( bFunc == b0 )
		{
			st_insert( Result, (char*)bFunc, (char*)b0 );
			Cudd_Ref( b0 );
			Cudd_Ref( b0 );
		}
		return Result;
	}

	// Step 1: Start the table and collect information about the nodes above the cut 
	// this information tells how many edges point to each node
	CutNodes = st_init_table(st_ptrcmp,st_ptrhash);
	Visited  = st_init_table(st_ptrcmp,st_ptrhash);

	// create the ADD to simplify processing (no complemented edges)
	aFunc = Cudd_BddToAdd( dd, bFunc );   Cudd_Ref( aFunc );

	// visit the nodes
	s_CutLevel = CUDD_MAXINDEX;
	CountNodeVisits_rec( dd, aFunc, Visited );

	// Step 2: Traverse the BDD using the visited table and compute the sum of paths
	CollectNodesAndComputePaths_rec( dd, aFunc, b1, Visited, CutNodes );

	// the function is not a constant
	// the table of CutNodes contains only two nodes - terminals
	assert( st_count(CutNodes) == 2 );
	{
		st_generator * gen;
		DdNode * aNode;
		DdNode * bNode;
		st_foreach_item( CutNodes, gen, (char**)&aNode, (char**)&bNode) 
		{
			Cudd_RecursiveDeref( dd, bNode );
		}
	}
	st_free_table( CutNodes );

	// move the nodes from the table Visited to the table Result
	{
		st_generator * gen;
		DdNode * aNode, * bNode;
		traventry * p;
		st_foreach_item( Visited, gen, (char**)&aNode, (char**)&p) 
		{
			bNode = Cudd_addBddPattern( dd, aNode );  Cudd_Ref( bNode );
			st_insert( Result, (char*)bNode, (char*) p->bSum );
			// takes both references, bNode and p->bSum
			free( p );
		}
		st_free_table( Visited );
	}

	// dereference the ADD
	Cudd_RecursiveDeref( dd, aFunc );

	// return the table
	return Result;
}

/**Function********************************************************************

  Synopsis    [Collects the nodes under the cut and, for each node, computes the sum of paths leading to it from the root.]

  Description [The table returned contains the set of BDD nodes pointed to under the cut
  and, for each node, the BDD of the sum of paths leading to this node from the root
  The sums of paths in the table are referenced. CutLevel is the first DD level 
  considered to be under the cut.]

  SideEffects []

  SeeAlso     [Extra_bddNodePaths]

******************************************************************************/
st_table * Extra_bddNodePathsUnderCut( DdManager * dd, DdNode * bFunc, int CutLevel )
{
	st_table * Visited;  // temporary table to remember the visited nodes
	st_table * CutNodes; // the result goes here
	st_table * Result; // the result goes here
	DdNode * aFunc;

	s_CutLevel = CutLevel;

	Result  = st_init_table(st_ptrcmp,st_ptrhash);
	// the terminal cases
	if ( Cudd_IsConstant( bFunc ) )
	{
		if ( bFunc == b1 )
		{
			st_insert( Result, (char*)b1, (char*)b1 );
			Cudd_Ref( b1 );
			Cudd_Ref( b1 );
		}
		else
		{
			st_insert( Result, (char*)b0, (char*)b0 );
			Cudd_Ref( b0 );
			Cudd_Ref( b0 );
		}
		return Result;
	}

	// create the ADD to simplify processing (no complemented edges)
	aFunc = Cudd_BddToAdd( dd, bFunc );   Cudd_Ref( aFunc );

	// Step 1: Start the tables and collect information about the nodes above the cut 
	// this information tells how many edges point to each node
	Visited  = st_init_table(st_ptrcmp,st_ptrhash);
	CutNodes = st_init_table(st_ptrcmp,st_ptrhash);

	CountNodeVisits_rec( dd, aFunc, Visited );

	// Step 2: Traverse the BDD using the visited table and compute the sum of paths
	CollectNodesAndComputePaths_rec( dd, aFunc, b1, Visited, CutNodes );

	// at this point the table of cut nodes is ready and the table of visited is useless
	{
		st_generator * gen;
		DdNode * aNode;
		traventry * p;
		st_foreach_item( Visited, gen, (char**)&aNode, (char**)&p ) 
		{
			Cudd_RecursiveDeref( dd, p->bSum );
			free( p );
		}
		st_free_table( Visited );
	}

	// go through the table CutNodes and create the BDD and the path to be returned
	{
		st_generator * gen;
		DdNode * aNode, * bNode, * bSum;
		st_foreach_item( CutNodes, gen, (char**)&aNode, (char**)&bSum) 
		{
			// aNode is not referenced, because aFunc is holding it
			bNode = Cudd_addBddPattern( dd, aNode );  Cudd_Ref( bNode ); 
			st_insert( Result, (char*)bNode, (char*)bSum );
			// the new table takes both refs
		}
		st_free_table( CutNodes );
	}

	// dereference the ADD
	Cudd_RecursiveDeref( dd, aFunc );

	// return the table
	return Result;

} /* end of Extra_bddNodePathsUnderCut */


/**Function********************************************************************

  Synopsis    [Collects the nodes under the cut in the ADD starting from the given set of ADD nodes.]

  Description [Takes the array, paNodes, of ADD nodes to start the traversal,
  the array, pbCubes, of BDD cubes to start the traversal with in each node, 
  and the number, nNodes, of ADD nodes and BDD cubes in paNodes and pbCubes. 
  Returns the number of columns found. Fills in paNodesRes (pbCubesRes) 
  with the set of ADD columns (BDD paths). These arrays should be allocated 
  by the user.]

  SideEffects []

  SeeAlso     [Extra_bddNodePaths]

******************************************************************************/
int Extra_bddNodePathsUnderCutArray( DdManager * dd, DdNode ** paNodes, DdNode ** pbCubes, int nNodes, DdNode ** paNodesRes, DdNode ** pbCubesRes, int CutLevel )
{
	st_table * Visited;  // temporary table to remember the visited nodes
	st_table * CutNodes; // the nodes under the cut go here
	int i, Counter;

	s_CutLevel = CutLevel;

	// there should be some nodes
	assert( nNodes > 0 );
	if ( nNodes == 1 && Cudd_IsConstant( paNodes[0] ) )
	{
		if ( paNodes[0] == a1 )
		{
			paNodesRes[0] = a1;          Cudd_Ref( a1 );
			pbCubesRes[0] = pbCubes[0];  Cudd_Ref( pbCubes[0] );
		}
		else
		{
			paNodesRes[0] = a0;          Cudd_Ref( a0 );
			pbCubesRes[0] = pbCubes[0];  Cudd_Ref( pbCubes[0] );
		}
		return 1;
	}

	// Step 1: Start the table and collect information about the nodes above the cut 
	// this information tells how many edges point to each node
	CutNodes = st_init_table(st_ptrcmp,st_ptrhash);
	Visited  = st_init_table(st_ptrcmp,st_ptrhash);

	for ( i = 0; i < nNodes; i++ )
		CountNodeVisits_rec( dd, paNodes[i], Visited );

	// Step 2: Traverse the BDD using the visited table and compute the sum of paths
	for ( i = 0; i < nNodes; i++ )
		CollectNodesAndComputePaths_rec( dd, paNodes[i], pbCubes[i], Visited, CutNodes );

	// at this point, the table of cut nodes is ready and the table of visited is useless
	{
		st_generator * gen;
		DdNode * aNode;
		traventry * p;
		st_foreach_item( Visited, gen, (char**)&aNode, (char**)&p ) 
		{
			Cudd_RecursiveDeref( dd, p->bSum );
			free( p );
		}
		st_free_table( Visited );
	}

	// go through the table CutNodes and create the BDD and the path to be returned
	{
		st_generator * gen;
		DdNode * aNode, * bSum;
		Counter = 0;
		st_foreach_item( CutNodes, gen, (char**)&aNode, (char**)&bSum) 
		{
			paNodesRes[Counter] = aNode;   Cudd_Ref( aNode ); 
			pbCubesRes[Counter] = bSum;
			Counter++;
		}
		st_free_table( CutNodes );
	}

	// return the number of cofactors found
	return Counter;

} /* end of Extra_bddNodePathsUnderCutArray */


 
/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Visits the nodes.]

  Description [Visits the nodes above the cut and the nodes pointed to below the cut;
  collects the visited nodes, counts how many times each node is visited, and sets 
  the path-sum to be the constant zero BDD.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void CountNodeVisits_rec( DdManager * dd, DdNode * aFunc, st_table * Visited )

{
	traventry * p;
	char **slot;
	if ( st_find_or_add(Visited, (char*)aFunc, &slot) )
	{ // the entry already exists
		p = (traventry*) *slot;
		// increment the counter of incoming edges
		p->nEdges++;
		return; 
	}
	// this node has not been visited
	assert( !Cudd_IsComplement(aFunc) );

	// create the new traversal entry
	p = (traventry *) malloc( sizeof(traventry) );
	// set the initial sum of edges to zero BDD
	p->bSum = b0;   Cudd_Ref( b0 );
	// set the starting number of incoming edges
	p->nEdges = 1;
	// set this entry into the slot
	*slot = (char*)p;

	// recur if the node is above the cut
	if ( cuddI(dd,aFunc->index) < s_CutLevel )
	{
		CountNodeVisits_rec( dd, cuddE(aFunc), Visited );
		CountNodeVisits_rec( dd, cuddT(aFunc), Visited );
	}
} /* end of CountNodeVisits_rec */


/**Function********************************************************************

  Synopsis    [Revisits the nodes and computes the paths.]

  Description [This function visits the nodes above the cut having the goal of 
  summing all the incomming BDD edges; when this function comes across the node 
  below the cut, it saves this node in the CutNode table.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void CollectNodesAndComputePaths_rec( DdManager * dd, DdNode * aFunc, DdNode * bCube, st_table * Visited, st_table * CutNodes )
{	
	// find the node in the visited table
	DdNode * bTemp;
	traventry * p;
	char **slot;
	if ( st_find_or_add(Visited, (char*)aFunc, &slot) )
	{ // the node is found
		// get the pointer to the traversal entry
		p = (traventry*) *slot;

		// make sure that the counter of incoming edges is positive
		assert( p->nEdges > 0 );

		// add the cube to the currently accumulated cubes
		p->bSum = Cudd_bddOr( dd, bTemp = p->bSum, bCube );  Cudd_Ref( p->bSum );
		Cudd_RecursiveDeref( dd, bTemp );

		// decrement the number of visits
		p->nEdges--;

		// if more visits to this node are expected, return
		if ( p->nEdges ) 
			return;
		else // if ( p->nEdges == 0 )
		{ // this is the last visit - propagate the cube

			// check where this node is
			if ( cuddI(dd,aFunc->index) < s_CutLevel )
			{ // the node is above the cut
				DdNode * bCube0, * bCube1;

				// get the top-most variable
				DdNode * bVarTop = dd->vars[aFunc->index];

				// compute the propagated cubes
				bCube0 = Cudd_bddAnd( dd, p->bSum, Cudd_Not( bVarTop ) );   Cudd_Ref( bCube0 );
				bCube1 = Cudd_bddAnd( dd, p->bSum,           bVarTop   );   Cudd_Ref( bCube1 );

				// call recursively
				CollectNodesAndComputePaths_rec( dd, cuddE(aFunc), bCube0, Visited, CutNodes );
				CollectNodesAndComputePaths_rec( dd, cuddT(aFunc), bCube1, Visited, CutNodes );

				// dereference the cubes
				Cudd_RecursiveDeref( dd, bCube0 );
				Cudd_RecursiveDeref( dd, bCube1 );
				return;
			}
			else
			{ // the node is below the cut
				// add this node to the cut node table, if it is not yet there

//				DdNode * bNode;
//				bNode = Cudd_addBddPattern( dd, aFunc );  Cudd_Ref( bNode );
				if ( st_find_or_add(CutNodes, (char*)aFunc, &slot) )
				{ // the node exists - should never happen
					assert( 0 );
				}
				*slot = (char*) p->bSum;   Cudd_Ref( p->bSum );
				// the table takes the reference of bNode
				return;
			}
		}
	}

	// the node does not exist in the visited table - should never happen
	assert(0);

} /* end of CollectNodesAndComputePaths_rec */

