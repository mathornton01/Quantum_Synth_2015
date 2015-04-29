////////////////////////////////////////////////////////////////////////
///                                                                  ///
///                    Improved Image Computation                    ///
///                                                                  ///
///               Alan Mishchenko  <alanmi@ee.pdx.edu>               ///
///                                                                  ///
///   Ver. 1.0. Started - May 07, 2001  Last update - May 09, 2001   ///
///   Ver. 1.1. Started - Sep 20, 2001  Last update - Sep 20, 2001   ///
///   Ver. 2.0. Started - Nov 27, 2001  Last update - Nov 27, 2001   ///
///                                                                  ///
////////////////////////////////////////////////////////////////////////

#include "util.h"
#include "extra.h"

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

// data structure storing the partitioned transition relation and other relevant data
// notice that all the BDDs in this structure are not referenced in this module
// because the user "holds" them while the image computation proceeds
typedef struct partTR 
{
	///////////////////////////////////////////////////////////////////////
	// the image computation problem is specified using these parameters
	///////////////////////////////////////////////////////////////////////
	DdManager * dd;      // the manager
	// the input variables that appear in the support of partitions (bPFuncs)
	// these inputs should be quantified unlike other variables (parameters)
    int       nXVars;    // the number of the input variables
	DdNode ** bXVars;    // the elementary bdds of the input variables
	// the output variables that appear in the image after computation
	// notice that there should be as many output variables as there are partitions
    int       nPVars;    // the number of output variables (may be different from nParts)
	DdNode ** bPVars;    // the bdd variables on which the computed image depends
	// the partions and the care set
    int       nParts;    // the number of partions (bPFuncs)
    DdNode ** bPFuncs;	 // the bdds of the partitions to be conjoined
	DdNode *  bPart0;    // the care set, which is imaged
	// additional flags
	int fUsePVars;       // set to 1 if the variables R should be introduced during image computation
	                     // alternatively, these variables may be already present in the partitions
	int fReplacePbyX;    // set to 1 if the replacement P->X is needed after computation
	int fMonolitic;      // set to 1 to use the naive image computation algorithm
	///////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////
	// intermediate data (used for partitioned TR)
	int *     pOrder;    // the ordering of parts used for partitined image computation
	DdNode *  bXCube0;   // the cube of variables appearing in bPart0 and not in bPFuncs
	DdNode ** bXCubes;   // the cubes of variables quantified in each iteration
	char **   pSupps;    // the square-sized array [nXVars x nXVars] containing 1's 
	                     // if the given part has the corresponding var in its support
	///////////////////////////////////////////////////////////////////////
	// intermediate data (used for monolithic TR)
	DdNode *  bTransRel; // the product transition relation
	///////////////////////////////////////////////////////////////////////
} partTR;


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

static int s_LargestTRBddSize;
static int s_fVerbose = 0;

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

// preparing one image computation
void extraImagePrepareMonolithic( partTR * p );
void extraImagePreparePartitioned( partTR * p );
// clearing the results of one image computation
void extraImageCleanMonolithic( partTR * p );
void extraImageClearPartitioned( partTR * p );
// image computation itself
DdNode * extraImageMonolithic_Internal( partTR * p );
DdNode * extraImagePartitioned_Internal( partTR * p );


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Performs one instance of image computation.]

  Description [The function takes the following arguments. The manager (dd);
  the number of input variables (nXVars), which should be quantified during image 
  computation; the input varibles (bXVars); the number of output variables (nPVars),
  (this number may be different from nParts if partition are the products of 
  several bit-functions AND[pi==Pi(x)]); the output variables (bPVars) to be 
  introduced during image computation (may be NULL when the variables are already 
  introduced into bPFuncs and there is no need to do final variable replacement - 
  in the latter case bPVars are needed); the number of partitions (nParts); the 
  partitions (bPFuncs); the care set to be imaged (bPart0); the flag to show that 
  the output variables should be introduced during the image computation; the 
  flag to show that the P vars should be replaced by X var in the computed image;
  the flag to perform the monolitic image computation (maybe useful for comparison 
  and debugging).]

  SideEffects []

  SeeAlso     []

******************************************************************************/
DdNode * 
Extra_bddImageCompute( 
  DdManager * dd,    // the manager
  int nXVars,          // the number of input variables (excluding parameter variables)
  DdNode ** bXVars,     // the elementary BDDs for the input variables
  int nPVars,          // the number of output variables (may be different from nParts)
  DdNode ** bPVars,     // the output variables introduced during the image computation
  int nParts,          // the number of partitions
  DdNode ** bPFuncs,    // the partitions  
  DdNode * bPart0,      // the care set to be imaged
  int fUsePVars,       // set to 1 if bPVars should be introduced during computation
  int fReplacePbyX,    // replace bPVars by bXVars after the image is computed
  int fMonolitic )     // use the naive image computation algorithm
{
	partTR * p;
	int Size;
	DdNode * bImage;
	int i;

	// if the variables should be introduced, it is assumed 
	// that there are at least as many of them as there are partitions
	if ( fUsePVars )
		assert( nParts <= nPVars );

	// if the use of P vars or var replacement is requested, 
	// then the PVars should be given
	if ( fUsePVars || fReplacePbyX )
		assert( bPVars != NULL );

	// allocate the structure to store the arguments
	p = ALLOC( partTR, 1 );
	memset( p, 0, sizeof( partTR ) );

	// copy the arguments
	p->dd           = dd;
	p->nXVars       = nXVars;
	p->bXVars       = bXVars;
	p->nPVars       = nPVars;
	p->bPVars       = bPVars;
	p->nParts       = nParts;
	p->bPFuncs      = bPFuncs;
	p->bPart0       = bPart0;
	p->fUsePVars    = fUsePVars;
	p->fReplacePbyX = fReplacePbyX;
	p->fMonolitic   = fMonolitic;

	// allocate temprary data structures
	// the order of partitions and the quantified cubes
	p->pOrder     = ALLOC( int, nParts );
	for ( i = 0; i < nParts; i++ )
		p->pOrder[i] = 0;
	p->bXCubes    = ALLOC( DdNode *, nParts );
	for ( i = 0; i < nParts; i++ )
		p->bXCubes[i] = NULL;
	// the temporary storage for the support of partitions
	Size = MAX(nXVars,nParts);
	p->pSupps     = ALLOC( char *, Size );
	p->pSupps[0]  = ALLOC( char  , Size * Size );
	memset( p->pSupps[0], 0, Size * Size * sizeof(char) );
	for ( i = 1; i < Size; i++ )
		p->pSupps[i] = p->pSupps[i-1] + Size;
	s_LargestTRBddSize = 0;

	// perform the computation
	if ( fMonolitic )
	{
		extraImagePrepareMonolithic(p);
		bImage = extraImageMonolithic_Internal(p); // bImage is already referenced
		extraImageCleanMonolithic(p);
	}
	else
	{
		extraImagePreparePartitioned(p);
		bImage = extraImagePartitioned_Internal(p); // bImage is already referenced 
		extraImageClearPartitioned(p);
	}

	// delocate the data structures
	FREE( p->pOrder );
	FREE( p->bXCubes );
	FREE( p->pSupps[0] );
	FREE( p->pSupps );
	FREE( p );

	Cudd_Deref( bImage );
	return bImage;
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static Functions                                            */
/*---------------------------------------------------------------------------*/

void extraImageCleanMonolithic( partTR * p )
// recycles the data structure by derefing temp parts and leaving the perm parts unchanged
{
	Cudd_RecursiveDeref( p->dd, p->bTransRel );
}

void extraImageClearPartitioned( partTR * p )
// recycles the data structure by derefing temp parts and leaving the perm parts unchanged
{
	int i;
	Cudd_RecursiveDeref( p->dd, p->bXCube0 );
	for ( i = 0; i < p->nParts; i++ )
		Cudd_RecursiveDeref( p->dd, p->bXCubes[i] );
}

void extraImagePrepareMonolithic( partTR * p )
// this function prepares the data structure for the monolithic image computation
{
	int i, nNodes = 0;
	DdNode * bComp, * bTemp;
	DdManager * dd = p->dd;
	assert( p->nParts > 0 );

	// build the transition relation mapping X space into P space using bPFuncs
	p->bTransRel = b1;  Cudd_Ref( b1 );
	for ( i = 0; i < p->nParts; i++ )
	{
		// get the component
		if ( p->fUsePVars )
			bComp = Cudd_bddXnor( p->dd, p->bPFuncs[i], p->bPVars[i] );
		else
			bComp = p->bPFuncs[i];
		Cudd_Ref( bComp );

		// multiply this component with the transitive relation
		p->bTransRel = Cudd_bddAnd( p->dd, bTemp = p->bTransRel, bComp );  Cudd_Ref( p->bTransRel );
		Cudd_RecursiveDeref( p->dd, bTemp );
		Cudd_RecursiveDeref( p->dd, bComp );
	}
	// now the transition relation is ready
	nNodes = Cudd_DagSize( p->bTransRel );
//	printf( "The transitive relation node count = %d\n", nNodes );
	if ( s_LargestTRBddSize < nNodes )
	     s_LargestTRBddSize = nNodes;
}

void extraImagePreparePartitioned( partTR * p )
// this function prepares the data structure for the paritioned image computation
// by selecting the order, in which partitions should be conjoined,
// and the variables, which should be quantified during each conjoining
{
	DdNode * bTemp;
	int c, v, Part;
	int PartBest, CostBest;
	int TopVar;
	DdManager * dd = p->dd;

	// allocate the book-keeping data structure
	int * Support   = ALLOC( int, MAX(p->dd->size,p->dd->sizeZ) );  // the temporary storage for supports
	int * nPartsupp = ALLOC( int, p->nParts );  // how many vars are in the part's support
	int * NVarOccur = ALLOC( int, p->nXVars );  // how many times each var occurs in supports
	int * NPartUVar = ALLOC( int, p->nParts );  // the number of unique vars in each part
	int * fSelected = ALLOC( int, p->nParts );  // the flag showing that the partition is selected

	// set the arrays to zero
	for ( c = 0; c < p->nParts; c++ )
		nPartsupp[c] = fSelected[c] = 0;
	for ( v = 0; v < p->nXVars; v++ )
		NVarOccur[v] = 0;

	// compute supports for all functions in the array
	for ( c = 0; c < p->nParts; c++ )
	{
		// compute the support for this component
		Extra_SupportArray( p->dd, p->bPFuncs[c], Support );
//		support = bdd_get_support( p->bPFuncs[c] );

		if ( s_fVerbose )
		printf( "Part #%2d: ", c );

		// copy the support into the internal data structure
		for ( v = 0; v < p->nXVars; v++ )
		{
			TopVar = p->bXVars[v]->index;
			p->pSupps[c][v] = Support[ TopVar ];

			if ( s_fVerbose )
			printf( "%d", p->pSupps[c][v] );
		}

		if ( s_fVerbose )
		printf( "\n" );
	}
	FREE( Support );

	if ( s_fVerbose )
	printf( "\n" );


	// now supports are ready - prepare to count 
	// how many unique variables belong to each part

	// set the counters of how-many-vars-in-parts and how-many-parts-with-vars
	for ( c = 0; c < p->nParts;  c++ )
	for ( v = 0; v < p->nXVars; v++ )
	{
		nPartsupp[c] += p->pSupps[c][v];
		NVarOccur[v] += p->pSupps[c][v];
	}

	////////////////////////////////////////////////////////////
	// find those variables that do not appear in the partitions
	if ( s_fVerbose )
		printf( "Care Set: " );
	p->bXCube0 = b1;  Cudd_Ref( b1 );
	for ( v = 0; v < p->nXVars; v++ )
	{
		if ( s_fVerbose )
			printf( "%d", (int)(NVarOccur[v] == 0) );

		if ( NVarOccur[v] == 0 ) 
		{ // this var belongs to the support of the care set and does not occur in partitions
			// add this var to the care set vars to be quantified at the beginning
			p->bXCube0 = Cudd_bddAnd( p->dd, bTemp = p->bXCube0, p->bXVars[v] ); Cudd_Ref( p->bXCube0 );
			Cudd_RecursiveDeref( p->dd, bTemp );
		}
	}
	////////////////////////////////////////////////////////////
	if ( s_fVerbose )
		printf( "\n" );


	// go through the partitions
	for ( Part = 0; Part < p->nParts; Part++ )
	{
		/////////////////////////////////////////////////
		// set the numbers of unique vars to zero
		for ( c = 0; c < p->nParts; c++ )
			NPartUVar[c] = 0;

		/////////////////////////////////////////////////
		// count how many unique vars each component has
		for ( v = 0; v < p->nXVars; v++ )
		if ( NVarOccur[v] == 1 ) // unique var
		{
			// find the corresponding component
			for ( c = 0; c < p->nParts; c++ )
				if ( p->pSupps[c][v] )
				{
					NPartUVar[c]++;
					break;
				}
		}
		/////////////////////////////////////////////////
		if ( s_fVerbose )
		for ( c = 0; c < p->nParts; c++ )
			printf( "%d", NPartUVar[c] );

		/////////////////////////////////////////////////
		// select the component with the max number of unique vars
		// in case of the tie, select the component with a larger support
		PartBest = -1;
		CostBest =  0;
		for ( c = 0; c < p->nParts; c++ )
		if ( !fSelected[c] )
		{
			if ( CostBest < NPartUVar[c] )
			{
				CostBest = NPartUVar[c];
				PartBest = c;
			}
			else if ( CostBest == NPartUVar[c] )
			{ // select the component with a larger support
				if ( PartBest == -1 )
					PartBest = c;
				else if ( nPartsupp[ PartBest ] < nPartsupp[ c ] )
					 PartBest = c;
			}
		}
		assert( PartBest != -1 );
		/////////////////////////////////////////////////
		if ( s_fVerbose )
		printf( "  Select part #%d: ", PartBest );

		/////////////////////////////////////////////////
		// create the cube of the unique vars of the best component
		p->bXCubes[PartBest] = b1;  Cudd_Ref( b1 );
		// go through all the vars of best component
		if ( s_fVerbose )
		printf( " Adding vars to the cube:" );
		for ( v = 0; v < p->nXVars; v++ )
		if ( p->pSupps[PartBest][v] ) // belongs to this component
		{
			if ( NVarOccur[v] == 1 ) // unique var
			{
				// add this var to the set of unique vars of best component
				p->bXCubes[PartBest] = Cudd_bddAnd( p->dd, bTemp = p->bXCubes[PartBest], p->bXVars[v] );  Cudd_Ref( p->bXCubes[PartBest] );
				Cudd_RecursiveDeref( p->dd, bTemp );
				if ( s_fVerbose )
				printf( " %d", v );
			}

			// remove this var from the book-keeping structure
			p->pSupps[PartBest][v] = 0;

			// remove this var from the counters of vars
			NVarOccur[v]--;

			// we do not remove this component from the support of the function
			// but perhaps we should
			nPartsupp[PartBest]--;
		}
		if ( s_fVerbose )
		printf( "\n" );

		// assign the next component in the order to be the best one
		p->pOrder[Part] = PartBest;

		// mark the partition as selected
		fSelected[PartBest] = 1;
	}
	if ( s_fVerbose )
	printf( "\n" );

	FREE( nPartsupp );
	FREE( NVarOccur );
	FREE( NPartUVar );
	FREE( fSelected );
}


/*//////////////////////////////////////////////////////////////////////
///                     IMAGE COMPUTATION FUNCTIONS                  ///
//////////////////////////////////////////////////////////////////////*/

DdNode * extraImageMonolithic_Internal( partTR * p )
// should be called after extraImageIntroduce() and 
// extraImagePrepareMonolithic()
// returns a referenced image
{
	DdNode * bImage;
	DdNode * bTemp;
	DdNode * bXVarsAll;
	int i;
	DdManager * dd = p->dd;

	// get the set of all variables bXVars
	bXVarsAll = b1;  Cudd_Ref( b1 );
	for ( i = 0; i < p->nXVars; i++ )
	{
		bXVarsAll = Cudd_bddAnd( p->dd, bTemp = bXVarsAll, p->bXVars[i] );  Cudd_Ref( bXVarsAll );
		Cudd_RecursiveDeref( p->dd, bTemp );
	}

	// image X-space into the Y-space
	bImage = Cudd_bddAndAbstract( p->dd, p->bTransRel, p->bPart0, bXVarsAll );    Cudd_Ref( bImage );
	Cudd_RecursiveDeref( p->dd, bXVarsAll );

	// replace vars from P-space into X-space
	if ( p->fReplacePbyX )
	{
/*		
		array_t * OldVars;   // the array of old variables (for bdd_substitute)
		array_t * NewVars;   // the array of new variables

		OldVars = array_alloc( DdNode *, p->nPVars );
		NewVars = array_alloc( DdNode *, p->nPVars );
		for ( i = 0; i < p->nPVars; i++ )
		{
			array_insert( DdNode *, OldVars, i, p->bPVars[i] );
			array_insert( DdNode *, NewVars, i, p->bXVars[i] );
		}
*/
		int * Permute;
		Permute = ALLOC( int, p->dd->size );
		for ( i = 0; i < p->dd->size; i++ )
			Permute[i] = i;
		for ( i = 0; i < p->nPVars; i++ )
			Permute[p->bPVars[i]->index] = p->bXVars[i]->index;


//		bImage = Cudd_bddVarMap( p->dd, bTemp = bImage );  Cudd_Ref( bImage );
		bImage = Cudd_bddPermute( p->dd, bTemp = bImage, Permute );   Cudd_Ref( bImage );
		Cudd_RecursiveDeref( p->dd, bTemp );

//		array_free( OldVars );
//		array_free( NewVars );

		FREE( Permute );
	}

	return bImage;
}

DdNode * extraImagePartitioned_Internal( partTR * p )
// should be called after extraImageIntroduce() and 
// extraImagePreparePartitioned()
// returns a referenced image
{
	int i, PartCur;
	DdNode * bImage, * bTemp, * bComp;
	int nNodeMax = 0;
//	int nNodeCur;
	// perform initial quantification
	bImage = Cudd_bddExistAbstract( p->dd, p->bPart0, p->bXCube0 );   Cudd_Ref( bImage );
	for ( i = 0; i < p->nParts; i++ )
	{
		PartCur = p->pOrder[i];
		// get the component
//		bComp  = Cudd_bddXnor( p->dd, p->bPFuncs[PartCur], p->bPVars[PartCur] );
		if ( p->fUsePVars )
			bComp = Cudd_bddXnor( p->dd, p->bPFuncs[PartCur], p->bPVars[PartCur] );
		else
			bComp = p->bPFuncs[PartCur]; 
		Cudd_Ref( bComp );

		bImage = Cudd_bddAndAbstract( p->dd, bTemp = bImage, bComp, p->bXCubes[PartCur] ); Cudd_Ref( bImage );
		// parts and cubes will be dereferenced later
		Cudd_RecursiveDeref( p->dd, bTemp );
		Cudd_RecursiveDeref( p->dd, bComp );

//		nNodeCur = Cudd_DagSize(bImage);
//		if ( nNodeMax < nNodeCur )
//			 nNodeMax = nNodeCur;
	}
//	printf( "\nThe largest size of image during computation using partitions = %d\n", nNodeMax );

	// replace vars from P-space into X-space
	if ( p->fReplacePbyX )
	{
/*		
		array_t * OldVars;   // the array of old variables (for bdd_substitute)
		array_t * NewVars;   // the array of new variables

		OldVars = array_alloc( DdNode *, p->nPVars );
		NewVars = array_alloc( DdNode *, p->nPVars );
		for ( i = 0; i < p->nPVars; i++ )
		{
			array_insert( DdNode *, OldVars, i, p->bPVars[i] );
			array_insert( DdNode *, NewVars, i, p->bXVars[i] );
		}
*/
		int * Permute;
		Permute = ALLOC( int, p->dd->size );
		for ( i = 0; i < p->dd->size; i++ )
			Permute[i] = i;
		for ( i = 0; i < p->nPVars; i++ )
			Permute[p->bPVars[i]->index] = p->bXVars[i]->index;


//		bImage = Cudd_bddVarMap( p->dd, bTemp = bImage );  Cudd_Ref( bImage );
		bImage = Cudd_bddPermute( p->dd, bTemp = bImage, Permute );   Cudd_Ref( bImage );
		Cudd_RecursiveDeref( p->dd, bTemp );

//		array_free( OldVars );
//		array_free( NewVars );

		FREE( Permute );
	}

	return bImage;
}



/*//////////////////////////////////////////////////////////////////////
///                         TEST BENCH                               ///
//////////////////////////////////////////////////////////////////////*/

/*
void Experiment( BFunc * pFunc )
{ 
	int i;
	long clk1;
	DdManager * dd = pFunc->dd;
	DdNode * bImage1;
	DdNode * bImage2;
	int nXVars;

	DdNode * bXVars[MAXINPUTS];
	DdNode * bPVars[MAXINPUTS];

	DdNode * bPart0;

	nXVars = ddMax(pFunc->nInputs,pFunc->nOutputs);

	for ( i = 0; i < nXVars; i++ )
		bXVars[i] = Cudd_bddIthVar( dd, i );

	for ( i = 0; i < nXVars; i++ )
		bPVars[i] = Cudd_bddIthVar( dd, nXVars + i );

	printf( "\n" );
	printf( "\n" );


//	bPart0 = Extra_bddRandomFunc( dd, pFunc->nXVars, 0.2 );  Cudd_Ref( bPart0 );
	bPart0 = pFunc->pOutputs[0];  Cudd_Ref( bPart0 );

	// compute image partitioned
	clk1 = clock();

//	bImage2 = Extra_bddImageCompute( pFunc->pOutputs, pFunc->nOutputs, bPart0, 1 );   Cudd_Ref( bImage2 );

	bImage2 = Extra_bddImageCompute( dd, nXVars, bXVars, pFunc->nOutputs, bPVars,
	                                 pFunc->nOutputs, pFunc->pOutputs, bPart0, 
									 1, 0, 0 );    Cudd_Ref( bImage2 );

	printf( "The number of nodes in the image = %d\n", Cudd_DagSize(bImage2) );
	PRT("Partitioned image computation time", clock() - clk1 );
	fflush(stdout);


	// compute image monolithic
	clk1 = clock();
//	bImage1 = Extra_bddImageCompute( pFunc->pOutputs, pFunc->nOutputs, bPart0, 0 );   Cudd_Ref( bImage1 );
  	bImage1 = Extra_bddImageCompute( dd, nXVars, bXVars, pFunc->nOutputs, bPVars,
	                                 pFunc->nOutputs, pFunc->pOutputs, bPart0, 
									 1, 0, 1 );  Cudd_Ref( bImage1 );

	printf( "The number of nodes in the image = %d\n", Cudd_DagSize(bImage1) );
	PRT("Monolithic image computation time", clock() - clk1 );
	fflush(stdout);

	Cudd_RecursiveDeref( dd, bImage1 );
	Cudd_RecursiveDeref( dd, bImage2 );
	Cudd_RecursiveDeref( dd, bPart0 );

	Extra_Dissolve( pFunc );
}

*/

/*//////////////////////////////////////////////////////////////////////
///                            END OF FILE                           ///
//////////////////////////////////////////////////////////////////////*/
