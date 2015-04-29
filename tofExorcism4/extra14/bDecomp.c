/**CFile***********************************************************************

  FileName    [bDecomp.c]

  PackageName [extra]

  Synopsis    [Experimental version of some BDD operators.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_bddDecompose()
				</ul>
			Internal procedures included in this module:
				<ul>
				</ul>
			Static procedures included in this module:
				<ul>
				<li> ddBuildNodeFunction()
				<li> ddCountCutNodes()
				<li> ddClearFlag()
				</ul>
	      ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$bDecomp.c, v.1.2, December 8,11, 2000, alanmi $]

******************************************************************************/

#include "util.h"
#include "cuddInt.h"
#include "extra.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

#define COUNTER_MAX  10 

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

typedef struct 
{
	int      CutLevel;
	int      Counter;
	DdNode * Nodes[COUNTER_MAX];
} travdata;

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

static DdNode* ddBuildNodeFunction(DdManager *dd, DdNode *F, DdNode *Node, int CutLevel);

static void ddCountCutNodes(DdManager *dd, DdNode *f, travdata *p);
static void ddClearFlag(DdNode *f);

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis [Disjointly decomposes the function with the given variable set.]

  Description [Checks whether F is disjointly decomposable with variables Vars 
  (meaning that F can be represented as a composition of disjoint support 
  functions G and H, such that the only output of G feeds into H).
  If F is not disjointly decomposable using Vars, Extra_bddDecompose() 
  returns the number of columns n (n<COUNTER_MAX) in the decomposition table of F 
  with Vars as the bound set. In this case, FuncG and FuncH remain without changes. 
  If F is indeed disjointly decomposable, Extra_bddDecompose() returns 2 and 
  sets FuncG and FuncH to be equal to the BDDs of the decomposed blocks. 
  These BDDs are already referenced. If NewVar is not NULL, it is used to 
  compose FuncH. NewVar should not belong to the support of F. The result
  can be verified as follows: Cudd_bddCompose(dd,FuncH,FuncG,NewVar->index) 
  should be F. If NewVar is NULL, uses the top variable in the DD manager to 
  compose H and returns functions G and H remapped into the upper levels of the 
  manager. In this case, the result cannot be verified using Cudd_bddCompose().]

  SideEffects [Does not change variable ordering.]

  SeeAlso     []

******************************************************************************/
int
Extra_bddDecompose(
  DdManager * dd,
  DdNode * F,
  DdNode * Vars,
  DdNode * NewVar,
  DdNode ** FuncH,
  DdNode ** FuncG)
{
	int     *MapOld2New, *MapNew2Old;
	int      nVarsFuncG, nVarsTotal, v, l, nColumns;
	int      UseCofactorOne = 1; /* tells what Node ([0] or [1]) is used for FuncG */
	DdNode  *RegF = Cudd_Regular(F), *PermutedF, *Temp, *ResG, *ResH;
	travdata Data;
    int      autoDynZ;

	/* disable reordering */
    autoDynZ = dd->autoDynZ;
    dd->autoDynZ = 0;

	/* skip top variable(s) in Vars that are not in the support of F */
	while ( 1 )
	{
		/* if there are no variables in F or Vars, quit */
		if ( RegF == dd->one || Vars == dd->one )
			return -1;
		/* if Vars is not a cube, quit */
		if ( Cudd_IsComplement( Vars ) || Cudd_E(Vars) != Cudd_Not( dd->one ) )
			return -1;
		/* reduce the top level of Vars until it is equal or below F */
		if ( cuddI( dd, RegF->index ) > cuddI( dd, Vars->index ) )
			Vars = Cudd_T( Vars );
		else
			break;
	}


	/* allocate memory to hold variable permutations */
	MapOld2New  = ALLOC( int, dd->size );
	MapNew2Old  = ALLOC( int, dd->size );
	if ( MapOld2New == NULL || MapNew2Old == NULL ) 
	{
		dd->errorCode = CUDD_MEMORY_OUT;
		return -1;
	}

	/* set the array of support vars to zero */
	for ( v = 0; v < dd->size; v++ )
		MapOld2New[v] = -1;

	/* define direct/inverse permutation of variables in F with Vars on top */
	for ( nVarsFuncG = 0; Vars != dd->one; nVarsFuncG++, Vars = Cudd_T( Vars ) )
	{
		/* check that Vars is a cube */
		if ( Cudd_IsComplement( Vars ) || Cudd_E(Vars) != Cudd_Not( dd->one ) )
			return -1;
		MapOld2New[ Vars->index ] = dd->invperm[nVarsFuncG];
		MapNew2Old[ dd->invperm[nVarsFuncG] ] = Vars->index;
	}
	/* go through the remaining variables in MapOld2New and map them */
	/* !!! do not go through variables, go through levels - 
	   this will remap variables evenly, in one continuous range */
	nVarsTotal = nVarsFuncG;
	for ( l = 0; l < dd->size; l++ )
		if ( MapOld2New[ dd->invperm[l] ] == -1 )
		{
			 MapOld2New[ dd->invperm[l] ] = dd->invperm[nVarsTotal];
			 MapNew2Old[dd->invperm[nVarsTotal]] =  dd->invperm[l];
			 nVarsTotal++;
		}
	assert( nVarsTotal == dd->size );


	/* permute variables in the function */
	PermutedF = Cudd_bddPermute( dd, F, MapOld2New );
	if ( PermutedF == NULL )
		goto cleanup;
	Cudd_Ref( PermutedF );	
	

	/* convert the permuted function into ADD */
	PermutedF = Cudd_BddToAdd( dd, Temp = PermutedF );
	if ( PermutedF == NULL )
	{
		Cudd_RecursiveDeref( dd, Temp );
		goto cleanup;
	}
	Cudd_Ref( PermutedF );
	Cudd_RecursiveDeref( dd, Temp );


	/* find the number of columns in the table */
	/* this function finds two nodes: Data->Nodes[0] and Data->Nodes[1] */
	Data.CutLevel = nVarsFuncG;
	Data.Counter  = 0;
	ddCountCutNodes( dd, PermutedF, &Data );
	ddClearFlag( Cudd_Regular(PermutedF) );
	nColumns = Data.Counter;
	assert( nColumns >= 2 );

	if ( nColumns > 2 )
	{
		Cudd_RecursiveDeref( dd, PermutedF );
		goto cleanup;
	}


	/* build the BDDs for the sum of cofactors leading to Node[0] */
	ResG = ddBuildNodeFunction( dd, PermutedF, Data.Nodes[UseCofactorOne], nVarsFuncG );
	if ( ResG == NULL )
	{
		Cudd_RecursiveDeref( dd, PermutedF );
		nColumns = 0;
		goto cleanup;
	}
	Cudd_Ref( ResG );


	/* convert Node[0] and Node[1] to BDDs */
	/* originally, the nodes are not referenced, they are held by PermuteF ADD */
	Data.Nodes[0] = Cudd_addBddPattern( dd, Data.Nodes[0] );
	if ( Data.Nodes[0] == NULL )
	{
		Cudd_RecursiveDeref( dd, PermutedF );
		goto cleanup;
	}
	Cudd_Ref( Data.Nodes[0] );
	Data.Nodes[1] = Cudd_addBddPattern( dd, Data.Nodes[1] );
	if ( Data.Nodes[1] == NULL )
	{
		Cudd_RecursiveDeref( dd, PermutedF );
		goto cleanup;
	}
	Cudd_Ref( Data.Nodes[1] );
	Cudd_RecursiveDeref( dd, PermutedF );

	/*////////////////////////////////////////////////////////*/
	/* decide how to return the result */
	if ( NewVar == NULL )
	{ /* return functions mapped to the upper part of the manager */

		DdNode *SuppH, *SuppHCopy;
		int nVarsFuncH;

		/* use the top-most var to compose FuncH */
		NewVar = Cudd_NotCond( Cudd_bddIthVar(dd,dd->invperm[0]), UseCofactorOne );
		ResH = Cudd_bddIte( dd, NewVar, Data.Nodes[0], Data.Nodes[1] );
		if ( ResH == NULL )
		{
			Cudd_RecursiveDeref( dd, Data.Nodes[0] );
			Cudd_RecursiveDeref( dd, Data.Nodes[1] );
			Cudd_RecursiveDeref( dd, ResG );
			nColumns = 0;
			goto cleanup;
		}
		Cudd_Ref( ResH );

		/* find the support of H */
		SuppH = Cudd_Support( dd, ResH );
		if ( SuppH == NULL )
		{
			Cudd_RecursiveDeref( dd, Data.Nodes[0] );
			Cudd_RecursiveDeref( dd, Data.Nodes[1] );
			Cudd_RecursiveDeref( dd, ResG );
			Cudd_RecursiveDeref( dd, ResH );
			nColumns = 0;
			goto cleanup;
		}
		Cudd_Ref( SuppH );

		SuppHCopy = SuppH;

		/* set the array of support vars to zero */
		for ( v = 0; v < dd->size; v++ )
			MapOld2New[v] = -1;

		/* create permutation to remap variables in H to the top of the manager */
		for ( nVarsFuncH = 0; SuppH != dd->one; nVarsFuncH++, SuppH = Cudd_T( SuppH ) )
			MapOld2New[ SuppH->index ] = dd->invperm[nVarsFuncH];

		/* go through the remaining variables in MapOld2New and map them */
		nVarsTotal = nVarsFuncH;
		for ( v = 0; v < dd->size; v++ )
			if ( MapOld2New[v] == -1 )
				 MapOld2New[v] = dd->invperm[nVarsTotal++];
		assert( nVarsTotal == dd->size );

		/* deference support, now useless */
		Cudd_RecursiveDeref( dd, SuppHCopy );

		/* remap function H */
		ResH = Cudd_bddPermute( dd, Temp = ResH, MapOld2New );
		if ( ResH == NULL )
		{
			Cudd_RecursiveDeref( dd, Data.Nodes[0] );
			Cudd_RecursiveDeref( dd, Data.Nodes[1] );
			Cudd_RecursiveDeref( dd, ResG );
			Cudd_RecursiveDeref( dd, Temp );
			nColumns = 0;
			goto cleanup;
		}
		Cudd_Ref( ResH );
		Cudd_RecursiveDeref( dd, Temp );

		*FuncG = ResG;
		*FuncH = ResH;
	}
	else /* use variable NewVar to compose H */
	{
		/* remap NewVar to the place from which it will became NewVar again */
		NewVar = Cudd_NotCond( Cudd_bddIthVar( dd, MapOld2New[NewVar->index] ), UseCofactorOne );

		/* build BDD for FuncH as an ITE using the new variable */
		ResH = Cudd_bddIte( dd, NewVar, Data.Nodes[0], Data.Nodes[1] );
		if ( ResH == NULL )
		{
			Cudd_RecursiveDeref( dd, Data.Nodes[0] );
			Cudd_RecursiveDeref( dd, Data.Nodes[1] );
			Cudd_RecursiveDeref( dd, ResG );
			nColumns = 0;
			goto cleanup;
		}
		Cudd_Ref( ResH );


		/* remap FuncG into the old variables */
		ResG = Cudd_bddPermute( dd, Temp = ResG, MapNew2Old );
		if ( ResG == NULL )
		{
			Cudd_RecursiveDeref( dd, Data.Nodes[0] );
			Cudd_RecursiveDeref( dd, Data.Nodes[1] );
			Cudd_RecursiveDeref( dd, Temp );
			Cudd_RecursiveDeref( dd, ResH );
			nColumns = 0;
			goto cleanup;
		}
		Cudd_Ref( ResG );
		Cudd_RecursiveDeref( dd, Temp );

		/* remap FuncH into the old variables */
		ResH = Cudd_bddPermute( dd, Temp = ResH, MapNew2Old );
		if ( ResH == NULL )
		{
			Cudd_RecursiveDeref( dd, Data.Nodes[0] );
			Cudd_RecursiveDeref( dd, Data.Nodes[1] );
			Cudd_RecursiveDeref( dd, Temp );
			Cudd_RecursiveDeref( dd, ResG );
			nColumns = 0;
			goto cleanup;
		}
		Cudd_Ref( ResH );
		Cudd_RecursiveDeref( dd, Temp );

		*FuncG = ResG;
		*FuncH = ResH;

	}
	Cudd_RecursiveDeref( dd, Data.Nodes[0] );
	Cudd_RecursiveDeref( dd, Data.Nodes[1] );


cleanup:

	FREE( MapOld2New );
	FREE( MapNew2Old );

	/* restore reordering parameter */
	dd->autoDynZ = autoDynZ;

    return nColumns;

} /* end of Cudd_bddDecompose */


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Builds the BDD of all paths in F pointing to Node.]

  Description [Takes F represented by the non-complemented ADD/BDD node and 
  returns an OR of all paths that lead from F to Node. In computing the sum, 
  replaces all the nodes below the cut by ZERO and the Node itself by ONE. 
  Node may be complemented. ]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode *
ddBuildNodeFunction(
  DdManager * dd,
  DdNode * F,
  DdNode * Node,
  int CutLevel)
{
	DdNode *Res;

//  IMPORTANT: CutLevel should be cached along with F and Node
//  otherwise, unexpected bugs show up (very rarely) - January 7, 2001

//	DdNode*(*cacheOp)(DdManager*,DdNode*,DdNode*);
    /* statLine(dd); */

	/* F is not complemented */
	assert( !Cudd_IsComplement(F) );

    /* Check cache. */
//	cacheOp = (DdNode*(*)(DdManager*,DdNode*,DdNode*))ddBuildNodeFunction;
//  Res = cuddCacheLookup2(dd, cacheOp, F, Node);
//  if (Res != NULL) 
//        return Res;
//	else
	{
		DdNode *Res0, *Res1;
		DdNode *Fe = cuddE(F), *Ft = cuddT(F);
	
		if ( Cudd_IsComplement(Fe) )
		{ /* the else child is complemented */

			if ( cuddI( dd, Cudd_Regular(Fe)->index ) >= CutLevel ) /* below the cut level */
/*				Res0 = ( Fe == Node )? dd->one: dd->zero; */
				Res0 = ( Fe == Node )? dd->one: Cudd_Not( dd->one );
			else /* above the cut level */
			{
				Res0 = ddBuildNodeFunction(dd, Cudd_Not(Fe), Node, CutLevel);
				if ( Res0 == NULL ) return NULL;
				Res0 = Cudd_Not( Res0 );
			}
		}
		else
		{
			if ( cuddI( dd, Fe->index ) >= CutLevel ) /* below the cut level */
/*				Res0 = ( Fe == Node )? dd->one: dd->zero; */
				Res0 = ( Fe == Node )? dd->one: Cudd_Not( dd->one );
			else /* above the cut level */
			{
				Res0 = ddBuildNodeFunction(dd, Fe, Node, CutLevel);
				if ( Res0 == NULL ) return NULL;
			}
		}
		Cudd_Ref( Res0 );

		/* the then child is never complemented */
		if ( cuddI( dd, Ft->index ) >= CutLevel )
/*			Res1 = ( Ft == Node )? dd->one: dd->zero; */
			Res1 = ( Ft == Node )? dd->one: Cudd_Not( dd->one );
		else
		{
			Res1 = ddBuildNodeFunction(dd, Ft, Node, CutLevel);
			if ( Res1 == NULL ) return NULL;
		}
		Cudd_Ref( Res1 );

		/* consider the case when Res0 and Res1 are the same node */
		if ( Res0 == Res1 )
			Res = Res1;
		/* consider the case when Res1 is complemented */
		else if ( Cudd_IsComplement(Res1) ) 
		{
            Res = cuddUniqueInter(dd, F->index, Cudd_Not(Res1),Cudd_Not(Res0));
            if ( Res == NULL ) 
			{
				Cudd_RecursiveDeref(dd,Res0);
				Cudd_RecursiveDeref(dd,Res1);
                return NULL;
            }
            Res = Cudd_Not(Res);
        } 
		else 
		{
			Res = cuddUniqueInter( dd, F->index, Res1, Res0 );
            if ( Res == NULL ) 
			{
				Cudd_RecursiveDeref(dd,Res0);
				Cudd_RecursiveDeref(dd,Res1);
                return NULL;
            }
        }
		cuddDeref( Res0 );
		cuddDeref( Res1 );

		/* insert the result into cache */
//	    cuddCacheInsert2(dd, cacheOp, F, Node, Res);
		return Res;
	}
} /* end of ddBuildNodeFunction */


/**Function********************************************************************

  Synopsis    [Counts the number of nodes pointed to under the cut level.]

  Description [Counts the number of nodes pointed to under the cut level. 
  Performs a DFS from f. The nodes are accumulated in p->Nodes[]. 
  Uses the LSB of the then pointer as visited flag.]

  SideEffects [None]

  SeeAlso     [ddClearFlag]

******************************************************************************/
static void
ddCountCutNodes(
  DdManager *dd,   /* the manager */
  DdNode    *f,    /* the BDD */
  travdata  *p)    /* data structure to store nodes under the cut level */
{
	DdNode *RegF = Cudd_Regular(f);

	if ( cuddI( dd, RegF->index ) < p->CutLevel ) 
	{ /* this node is above the cut level */

		/* already visited node */
		if ( Cudd_IsComplement(RegF->next) ) 
			return;

		/* call recursively - pointer are not necessaryly regular */
		ddCountCutNodes(dd,cuddE(RegF),p);
		ddCountCutNodes(dd,cuddT(RegF),p);

		/* Mark as visited. */
		RegF->next = Cudd_Not(RegF->next);
	}
	else
	{ /* this node is below the cut level */

		/* check whether this node has already been collected */
		int i;
		for ( i = 0; i < p->Counter; i++ )
			if ( f == p->Nodes[i] )
				return;
		/* this is a new node */

		/* check how many nodes can be stored */
		if ( p->Counter == COUNTER_MAX )
			return;

		/* store the node */
		p->Nodes[ p->Counter++ ] = f;
		return;
	}
    return;

} /* end of ddCountCutNodes */


/**Function********************************************************************

  Synopsis    [Performs a DFS from f, clearing the LSB of the next pointers.]

  Description [Should be called for a regular pointer!]

  SideEffects [None]

  SeeAlso     [ddSupportStep ddDagInt]

******************************************************************************/
static void
ddClearFlag(
  DdNode * f)
{
    if (!Cudd_IsComplement(f->next)) 
		return;

    /* Clear visited flag. */
    f->next = Cudd_Regular(f->next);
    if (cuddIsConstant(f)) 
		return;

    ddClearFlag(cuddT(f));
    ddClearFlag(Cudd_Regular(cuddE(f)));
    return;

} /* end of ddClearFlag */