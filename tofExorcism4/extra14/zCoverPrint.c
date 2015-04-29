/**CFile***********************************************************************

  FileName    [zCoverPrint.c]

  PackageName [extra]

  Synopsis    [The print out of the cube cover represented by a ZDD.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_WriteFunctionSop();
				<li> Extra_WriteFunctionMuxes();
				<li> Extra_PerformVerification();
				<li> Extra_VerifyRange();
				</ul>
			Internal procedures included in this module:
				<ul>
				<li> extraWriteFunctionSop();
				<li> extraWriteFunctionMuxes();
				</ul>
			Static procedures included in this module:
				<ul>
				</ul>
	      ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$zCoverPrint.c, v.1.2, September 4, 2001, alanmi $]

******************************************************************************/

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


/**Automaticstart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/**Automaticend***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Writes the BDD into a PLA file as one SOP block.]

  Description [Takes the manager, the function, the array of variable names, 
  the number of variables and the file name. This function is useful, when we need 
  to write into a file the function, for which it is impossible to derive the SOP.]

  SideEffects [None]

  SeeAlso     [Cudd_zddPrintCover]

******************************************************************************/
void Extra_WriteFunctionSop( 
  DdManager * dd, 
  DdNode * bFunc, 
  DdNode * bFuncDc, 
  char ** pNames, 
  char * OutputName, 
  char * FileName )
{
	static int s_pVarMask[MAXINPUTS];
	DdNode * bFuncs[2]; 
	FILE * pFile;
	int nInputCounter;
	int i;
	DdNode * zCover;

	bFuncs[0] = bFunc;
	bFuncs[1] = (bFuncDc)? bFuncDc: b0;

	// create the variable mask
//	Extra_SupportArray( dd, bFunc, s_pVarMask );
	Extra_VectorSupportArray( dd, bFuncs, 2, s_pVarMask );
	nInputCounter = 0;
	for ( i = 0; i < dd->size; i++ )
		if ( s_pVarMask[i] )
			nInputCounter++;


	// start the file
	pFile = fopen( FileName, "w" );
/*
	fprintf( pFile, ".model %s\n", FileName );
	fprintf( pFile, ".inputs" );
	if ( pNames )
	{
		for ( i = 0; i < dd->size; i++ ) // go through levels
			if ( s_pVarMask[ dd->invperm[i] ] ) // if this level's var is present
				fprintf( pFile, " %s", pNames[ dd->invperm[i] ] ); // print its name
	}
	else
	{
		for ( i = 0; i < nInputCounter; i++ )
			fprintf( pFile, " x%d", i );
	}
    fprintf( pFile, "\n" );
	fprintf( pFile, ".outputs %s", OutputName );
    fprintf( pFile, "\n" );
*/
	fprintf( pFile, ".i %d\n", nInputCounter );
	fprintf( pFile, ".o %d\n", 1 );

	// write the on-set into file
	zCover = Extra_zddIsopCover( dd, bFunc, bFunc );       Cudd_Ref( zCover );
	if ( zCover != z0 )
	extraWriteFunctionSop( pFile, dd, zCover, -1, dd->size, "1", s_pVarMask );
	Cudd_RecursiveDerefZdd( dd, zCover );

	// write the dc-set into file
	if ( bFuncDc )
	{
		zCover = Extra_zddIsopCover( dd, bFuncDc, bFuncDc );       Cudd_Ref( zCover );
		if ( zCover != z0 )
		extraWriteFunctionSop( pFile, dd, zCover, -1, dd->size, "-", s_pVarMask );
		Cudd_RecursiveDerefZdd( dd, zCover );
	}

//	fprintf( pFile, ".end\n" );
	fprintf( pFile, ".e\n" );
	fclose( pFile );
}

/**Function********************************************************************

  Synopsis    [Writes the {A,B}DD into a BLIF file as a network of MUXes.]

  Description [Takes the manager, the function, the array of variable names, 
  the number of variables, the output name and the file name. This function is useful, when we need 
  to write into a file the function, for which it is impossible to derive the SOP.]

  SideEffects [None]

  SeeAlso     [Cudd_zddPrintCover]

******************************************************************************/
void Extra_WriteFunctionMuxes( 
  DdManager * dd, 
  DdNode * Func, 
  char ** pNames, 
  char * OutputName, 
  char * FileName )
{
	int i;
	FILE * pFile;
	int nInputCounter;
	static int s_pVarMask[MAXINPUTS];

	// create the variable mask
	Extra_SupportArray( dd, Func, s_pVarMask );
//	Extra_VectorSupportArray( dd, bFuncs, 2, s_pVarMask );
	nInputCounter = 0;
	for ( i = 0; i < dd->size; i++ )
		if ( s_pVarMask[i] )
			nInputCounter++;

	// start the file
	pFile = fopen( FileName, "w" );
	fprintf( pFile, ".model %s\n", FileName );

	fprintf( pFile, ".inputs" );
	if ( pNames )
	{
		for ( i = 0; i < dd->size; i++ ) // go through vars
			if ( s_pVarMask[i] ) // if this var is present
				fprintf( pFile, " %s", pNames[i] ); // print its name
	}
	else
	{
		for ( i = 0; i < dd->size; i++ )
			if ( s_pVarMask[i] ) // if this var is present
				fprintf( pFile, " x%d", i );
	}

	fprintf( pFile, "\n" );
	fprintf( pFile, ".outputs %s", OutputName );
    fprintf( pFile, "\n" );

	// write the DD into the file
	extraWriteFunctionMuxes( pFile, Func, "F", "", pNames );

	fprintf( pFile, ".end\n" );
	fclose( pFile );
}

//#define PRB(f)    printf("%s = ", #f); Extra_bddPrint(dd,f); printf("\n")

/**Function********************************************************************

  Synopsis    [Verifies the multi-output function against the one written into the file.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Extra_PerformVerification( DdManager * dd, DdNode ** pOutputs, DdNode ** pOutputOffs, int nOutputs, char * FileName )
{
	int i;
	int fVerification = 1;
	int fThisOutputVer;
	BFunc g_Func;

	// read the total multi-output function
	g_Func.dd = dd;
	g_Func.FileInput = util_strsav( FileName );

	if ( Extra_ReadFile( &g_Func ) == 0 )
	{
		printf( "\nSomething did not work out while reading the decomposed file\n" );
		Extra_Dissolve( &g_Func );
		return 0;
	} 

	// compare all outputs
	for ( i = 0; i < nOutputs; i++ )
	{
		fThisOutputVer = 1;
		if ( !Cudd_bddLeq( dd, pOutputs[i], g_Func.pOutputs[i] ) )
		{
			fVerification  = 0;
			fThisOutputVer = 0;
			printf( "The on-set is not contained in the solution\n", i );
//			printf( "Original = " );
//			Extra_zddIsopPrintCover( dd, pOutputs[i], pOutputs[i] );
//			printf( "Derived = " );
//			Extra_zddIsopPrintCover( dd, g_Func.pOutputs[i], g_Func.pOutputs[i] );
		}
		if ( !Cudd_bddLeq( dd, g_Func.pOutputs[i], Cudd_Not(pOutputOffs[i]) ) )
		{
			fVerification  = 0;
			fThisOutputVer = 0;
			printf( "The solution is not contained in the on-set plus dc-set\n", i );
//			printf( "Original = " );
//			Extra_zddIsopPrintCover( dd, pOutputs[i], pOutputs[i] );
//			printf( "Derived = " );
//			Extra_zddIsopPrintCover( dd, g_Func.pOutputs[i], g_Func.pOutputs[i] );
		}
		if ( !fThisOutputVer )
		{
			if ( g_Func.pOutputs[i] == Cudd_Not(pOutputs[i]) )
				printf( "The resulting function is complemented\n" );
			printf( "Verification for output #%d: FAILED\n\n", i );
		}
	}

	// delocate
	Extra_Dissolve( &g_Func );
	return fVerification;
}

/**Function********************************************************************

  Synopsis    [Verify that the function falls within the given range.]

  Description [Returns 1 if the function falls within the specified range. bLower is the on-set.
  bUpper is the complement of the off-set.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Extra_VerifyRange( DdManager * dd, DdNode * bFunc, DdNode * bLower, DdNode * bUpper )
{
	int fVerificationOkey = 1;
	
	// make sure that the function belongs to the range [bLower, bUpper]
	if ( !Cudd_bddLeq( dd, bLower, bFunc ) )
	{
		fVerificationOkey = 0;
		printf( "\nVerification not okay: Function does not cover the on-set\n" );
	}
	if ( !Cudd_bddLeq( dd, bFunc, bUpper ) )
	{
		fVerificationOkey = 0;
		printf( "\nVerification not okay: Function overlaps with the off-set\n" );
	}

	return fVerificationOkey;
}


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Prints the cover represented by a ZDD.]

  Description [Takes the pointer to the output file stream, the manager, the cover 
  represented by a ZDD, the level from which this function was called (initially,
  this level should be -1), the number of levels that are considered when traversing 
  the cover, the suffix to append to the end of each line, and the variable mask 
  showing variables appearing in the cover. In the cubes that are printed, 
  the variables follow in the order in which they appear in the levels starting 
  from the topmost one.]

  SideEffects [None]

  SeeAlso     [Cudd_zddPrintCover]

******************************************************************************/
void extraWriteFunctionSop( 
	FILE * pFile, 
	DdManager * dd, 
	DdNode * zCover, 
	int levPrev,         // the previous level, from which this function has been called
	int nLevels,         // the number of levels traversed while printing starting from the top one
	const char * AddOn,  // the additional string to attache at the end
	int * VarMask )      // the mask showing which variables should be printed
{
	static char s_VarValueAtLevel[MAXINPUTS];

	if ( zCover == z1 )
	{
		int lev;
		// fill in the remaining variables
		for ( lev = levPrev + 1; lev < nLevels; lev++ )
//			s_VarValueAtLevel[ dd->invpermZ[ 2*lev ] / 2 ] = '-';
			s_VarValueAtLevel[ lev ] = '-';

		// write zero-delimiter
		s_VarValueAtLevel[nLevels] = 0;

		// print the cube
		if ( VarMask == NULL )
			fprintf( pFile, "%s %s\n", s_VarValueAtLevel, AddOn );
		else
		{ 
			for ( lev = 0; lev < nLevels; lev++ )
				if ( VarMask[ dd->invperm[lev] ] ) // the variable on this level
					fprintf( pFile, "%c", s_VarValueAtLevel[lev] );
            fprintf( pFile, " %s\n", AddOn );
		}
	}
	else
	{
		// find the level of the top variable
		int TopLevel = dd->permZ[zCover->index] / 2;
		int TopPol   = zCover->index % 2;
		int lev;

		// fill in the remaining variables
		for ( lev = levPrev + 1; lev < TopLevel; lev++ )
	//		s_VarValueAtLevel[ dd->invpermZ[ 2*lev ] / 2 ] = '-';
			s_VarValueAtLevel[ lev ] = '-';

		// fill in this variable and call for the low and high cofactors
		if ( TopPol == 0 ) 
		{  // the given var has positive polarity
	//		s_VarValueAtLevel[ dd->invpermZ[ 2*TopLevel ] / 2 ] = '1';
			s_VarValueAtLevel[ TopLevel ] = '1';
			extraWriteFunctionSop( pFile, dd, cuddT(zCover), TopLevel, nLevels, AddOn, VarMask );

			if ( cuddE(zCover) != z0 )
			{
	//			s_VarValueAtLevel[ dd->invpermZ[ 2*TopLevel ] / 2 ] = '-';
				s_VarValueAtLevel[ TopLevel ] = '-';
				extraWriteFunctionSop( pFile, dd, cuddE(zCover), TopLevel, nLevels, AddOn, VarMask );
			}
		}
		else
		{  // the given var has negative polarity
	//		s_VarValueAtLevel[ dd->invpermZ[ 2*TopLevel ] / 2 ] = '0';
			s_VarValueAtLevel[ TopLevel ] = '0';
			extraWriteFunctionSop( pFile, dd, cuddT(zCover), TopLevel, nLevels, AddOn, VarMask );

			if ( cuddE(zCover) != z0 )
			{
	//			s_VarValueAtLevel[ dd->invpermZ[ 2*TopLevel ] / 2 ] = '-';
				s_VarValueAtLevel[ TopLevel ] = '-';
				extraWriteFunctionSop( pFile, dd, cuddE(zCover), TopLevel, nLevels, AddOn, VarMask );
			}
		}
	}
}


/**Function********************************************************************

  Synopsis    [Performs the recursive step of Extra_WriteFunctionMuxes().]

  Description [Takes the output file stream, the {A,B}DD function, the name of the output, 
  the prefix attached to each intermediate signal to make it unique, and the array of input 
  variable names. This function is useful, when we need to write into a file the function, 
  for which it is impossible to derive the SOP. Some part of the code is borrowed 
  from Cudd_DumpDot()]

  SideEffects [None]

  SeeAlso     [Cudd_zddPrintCover]

******************************************************************************/
void extraWriteFunctionMuxes( 
  FILE * pFile, 
  DdNode * Func, 
  char * OutputName, 
  char * Prefix, 
  char ** InputNames )
{
	int i;
	st_table * visited;
	st_table * invertors;
	st_generator * gen = NULL;
	long refAddr, diff, mask;
	DdNode * Node, * Else, * ElseR, * Then;

	/* Initialize symbol table for visited nodes. */
	visited = st_init_table( st_ptrcmp, st_ptrhash );

	/* Collect all the nodes of this DD in the symbol table. */
	cuddCollectNodes( Cudd_Regular(Func), visited );

	/* Find how many most significant hex digits are identical
	   ** in the addresses of all the nodes. Build a mask based
	   ** on this knowledge, so that digits that carry no information
	   ** will not be printed. This is done in two steps.
	   **  1. We scan the symbol table to find the bits that differ
	   **     in at least 2 addresses.
	   **  2. We choose one of the possible masks. There are 8 possible
	   **     masks for 32-bit integer, and 16 possible masks for 64-bit
	   **     integers.
	 */

	/* Find the bits that are different. */
	refAddr = ( long )Cudd_Regular(Func);
	diff = 0;
	gen = st_init_gen( visited );
	while ( st_gen( gen, ( char ** ) &Node, NULL ) )
	{
		diff |= refAddr ^ ( long ) Node;
	}
	st_free_gen( gen );
	gen = NULL;

	/* Choose the mask. */
	for ( i = 0; ( unsigned ) i < 8 * sizeof( long ); i += 4 )
	{
		mask = ( 1 << i ) - 1;
		if ( diff <= mask )
			break;
	}


	// write the buffer for the output
	fprintf( pFile, ".names %s%lx %s\n", Prefix, ( mask & (long)Cudd_Regular(Func) ) / sizeof(DdNode), OutputName ); 
	fprintf( pFile, "%s 1\n", (Cudd_IsComplement(Func))? "0": "1" );


	invertors = st_init_table( st_ptrcmp, st_ptrhash );

	gen = st_init_gen( visited );
	while ( st_gen( gen, ( char ** ) &Node, NULL ) )
	{
		if ( Node->index == CUDD_MAXINDEX )
		{
			// write the terminal node
			fprintf( pFile, ".names %s%lx\n", Prefix, ( mask & (long)Node ) / sizeof(DdNode) );
			fprintf( pFile, " %s\n", (cuddV(Node) == 0.0)? "0": "1" );
			continue;
		}

		Else  = cuddE(Node);
		ElseR = Cudd_Regular(Else);
		Then  = cuddT(Node);

		if ( InputNames )
		{
			assert( InputNames[Node->index] );
		}
		if ( Else == ElseR )
		{ // no inverter
			if ( InputNames )
				fprintf( pFile, ".names %s", InputNames[Node->index] );
			else
				fprintf( pFile, ".names x%d", Node->index );

			fprintf( pFile, " %s%lx %s%lx %s%lx\n",
				              Prefix, ( mask & (long)ElseR ) / sizeof(DdNode),
				              Prefix, ( mask & (long)Then  ) / sizeof(DdNode),
						      Prefix, ( mask & (long)Node  ) / sizeof(DdNode)   );
			fprintf( pFile, "01- 1\n" );
			fprintf( pFile, "1-1 1\n" );
		}
		else
		{ // inverter
			if ( InputNames )
				fprintf( pFile, ".names %s", InputNames[Node->index] );
			else
				fprintf( pFile, ".names x%d", Node->index );

			fprintf( pFile, " %s%lx_i %s%lx %s%lx\n", 
				              Prefix, ( mask & (long)ElseR ) / sizeof(DdNode),
				              Prefix, ( mask & (long)Then  ) / sizeof(DdNode),
						      Prefix, ( mask & (long)Node  ) / sizeof(DdNode)   );
			fprintf( pFile, "01- 1\n" );
			fprintf( pFile, "1-1 1\n" );

			if ( !st_find_or_add( invertors, (char*)ElseR, NULL ) )
			{
				// write the intertor
				fprintf( pFile, ".names %s%lx %s%lx_i\n",  
								  Prefix, ( mask & (long)ElseR  ) / sizeof(DdNode),
								  Prefix, ( mask & (long)ElseR  ) / sizeof(DdNode)   );
				fprintf( pFile, "0 1\n" );
			}
		}
	}
	st_free_gen( gen );
	gen = NULL;
	st_free_table( visited );
	st_free_table( invertors );
}

/*---------------------------------------------------------------------------*/
/* Definition of static Functions                                            */
/*---------------------------------------------------------------------------*/
