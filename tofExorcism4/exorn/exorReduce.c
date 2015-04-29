////////////////////////////////////////////////////////////////////////
///                                                                  ///
///                  Implementation of EXORCISM - 4                  ///
///              An Exclusive Sum-of-Product Minimizer               ///
///                                                                  ///
///               Alan Mishchenko  <alanmi@ee.pdx.edu>               ///
///                                                                  ///
////////////////////////////////////////////////////////////////////////
///                                                                  ///
///                 Minimization Module
///                                                                  ///
///      2) computes the starting Pseudo-KRO cover                   ///
///      3) allocates memory for and creating the ESOP cover         ///
///      4) calls the iterative simplification procedure             ///
///      5) writes the resultant cover into an ESOP PLA/BLIF file    ///
///      6) verifies the result against the original function        ///
///                                                                  ///
///  Ver. 1.0. Started - July 18, 2000. Last update - July 20, 2000  ///
///  Ver. 1.1. Started - July 24, 2000. Last update - July 29, 2000  ///
///  Ver. 1.2. Started - July 30, 2000. Last update - July 31, 2000  ///
///  Ver. 1.4. Started -  Aug 10, 2000. Last update -  Aug 26, 2000  ///
///  Ver. 1.5. Started -  Aug 30, 2000. Last update -  Aug 30, 2000  ///
///  Ver. 1.6. Started -  Sep 11, 2000. Last update -  Sep 15, 2000  ///
///                                                                  ///
////////////////////////////////////////////////////////////////////////
///   This software was tested with the BDD package "CUDD", v.2.3.0  ///
///                          by Fabio Somenzi                        ///
///                  http://vlsi.colorado.edu/~fabio/                ///
////////////////////////////////////////////////////////////////////////

#include "exor.h"
#include "extra.h"

////////////////////////////////////////////////////////////////////////
///                       EXTERNAL VARIABLES                         ///
////////////////////////////////////////////////////////////////////////

// information about the options, the function, and the cover
extern BFunc g_Func;
extern cinfo g_CoverInfo;

// the flag which tells whether the literal count is improved in ExorLink-2
extern int s_fDecreaseLiterals;

// the number of allocated places in the adjacency queques
extern int s_nPosAlloc;
// the peak number of occupied places in the adjacency queques
extern int s_nPosMax[3];

////////////////////////////////////////////////////////////////////////
///                      EXTERNAL FUNCTIONS                          ///
////////////////////////////////////////////////////////////////////////

// cube cover memory allocation/delocation
extern int AllocateCover( int nCubes, int nWordsIn, int nWordsOut );
extern void DelocateCover();

// cube storage allocation/delocation
extern int AllocateCubeSets( int nVarsIn, int nVarsOut );
extern void DelocateCubeSets();

// adjacency queque allocation/delocation procedures
extern int AllocateQueques( int nPlaces );
extern void DelocateQueques();

// Pseudo-Kronecker cover computation
extern int  CountTermsInPseudoKroneckerCover( DdManager *bddm, DdNode** OnSets );
extern void GeneratePseudoKroneckerCover();

// cover reduction
// iterative ExorLink
extern int IterativelyApplyExorLink2( char fDistEnable );   
extern int IterativelyApplyExorLink3( char fDistEnable );   
extern int IterativelyApplyExorLink4( char fDistEnable );   

// imported file writing procedures
extern int WriteResultIntoFile();

// cover/cube printing
/*
extern void PrintCube( ostream& DebugStream, Cube* pC );
extern void PrintCoverDebug( ostream& DebugStream );
extern void PrintCoverProfile( ostream& DebugStream );
extern void PrintQuequeStats();
extern int GetQuequeStats( cubedist Dist );
*/

extern int GetNumberOfCubes( DdManager * dd, DdNode ** pbFuncs, int nFuncs );
extern void AddCubesToStartingCover( DdManager * dd );


////////////////////////////////////////////////////////////////////////
///                    FUNCTIONS OF THIS MODULE                      ///
////////////////////////////////////////////////////////////////////////

// the main function of this module
int Exorcism();

// iterative reduction of the cover
int ReduceEsopCover();

////////////////////////////////////////////////////////////////////////
///                        STATIC VARIABLES                          ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      FUNCTION DEFINITIONS                        ///
////////////////////////////////////////////////////////////////////////


int Exorcism()
// performs heuristic minimization of ESOPs
// returns 1 on success, 0 on failure
{
	long clk1;
	int RemainderBits;
	int TotalWords;
	int MemTemp, MemTotal;
	BFunc Func2;
	int fVerificationOkay;
	int i;

	///////////////////////////////////////////////////////////////////////
	// STEPS of HEURISTIC ESOP MINIMIZATION
	///////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////
	// STEP 1: determine the size of the starting cover
	///////////////////////////////////////////////////////////////////////
	assert( g_Func.nInputs > 0 );
	// inputs
	RemainderBits = (g_Func.nInputs*2)%(sizeof(unsigned)*8);
	TotalWords    = (g_Func.nInputs*2)/(sizeof(unsigned)*8) + (RemainderBits > 0);
	g_CoverInfo.nVarsIn  = g_Func.nInputs;
	g_CoverInfo.nWordsIn = TotalWords;
	// outputs
	RemainderBits = (g_Func.nOutputs)%(sizeof(unsigned)*8);
	TotalWords    = (g_Func.nOutputs)/(sizeof(unsigned)*8) + (RemainderBits > 0);
	g_CoverInfo.nVarsOut  = g_Func.nOutputs;
	g_CoverInfo.nWordsOut = TotalWords;
	g_CoverInfo.cIDs = 1;

	// cubes
	clk1 = clock();
//	g_CoverInfo.nCubesBefore = CountTermsInPseudoKroneckerCover( g_Func.dd, g_Func.pOutputs );
	g_CoverInfo.nCubesBefore = GetNumberOfCubes( g_Func.dd, g_Func.pOutputs, g_Func.nOutputs );
	g_CoverInfo.TimeStart = clock() - clk1;

	if ( g_CoverInfo.Verbosity )
	{
	printf( "Starting cover generation time is %.2f sec\n", TICKS_TO_SECONDS(g_CoverInfo.TimeStart) );
	printf( "The number of cubes in the starting cover is %d\n", g_CoverInfo.nCubesBefore );
	}

	if ( g_CoverInfo.nCubesBefore > 20000 )
	{
		printf( "\nThe size of the starting cover is more than 20000 cubes. Quitting...\n" );
		return 0;
	}

	///////////////////////////////////////////////////////////////////////
	// STEP 2: prepare internal data structures
	///////////////////////////////////////////////////////////////////////
	g_CoverInfo.nCubesAlloc = g_CoverInfo.nCubesBefore + ADDITIONAL_CUBES; 

	// allocate cube cover
	MemTotal = 0;
	MemTemp = AllocateCover( g_CoverInfo.nCubesAlloc, g_CoverInfo.nWordsIn, g_CoverInfo.nWordsOut );
	if ( MemTemp == 0 )
	{
		printf( "Unexpected memory allocation problem. Quitting...\n" );
		Cudd_Quit( g_Func.dd );
		return 0;
	}
	else 
		MemTotal += MemTemp;

	// allocate cube sets
	MemTemp = AllocateCubeSets( g_CoverInfo.nVarsIn, g_CoverInfo.nVarsOut );
	if ( MemTemp == 0 )
	{
		printf( "Unexpected memory allocation problem. Quitting...\n" );
		Cudd_Quit( g_Func.dd );
		return 0;
	}
	else 
		MemTotal += MemTemp;

	// allocate adjacency queques
	MemTemp = AllocateQueques( g_CoverInfo.nCubesAlloc*g_CoverInfo.nCubesAlloc/CUBE_PAIR_FACTOR );
	if ( MemTemp == 0 )
	{
		printf( "Unexpected memory allocation problem. Quitting...\n" );
		Cudd_Quit( g_Func.dd );
		return 0;
	}
	else 
		MemTotal += MemTemp;

	if ( g_CoverInfo.Verbosity )
	printf( "Dynamically allocated memory (excluding the BDD package) is %dK\n",  MemTotal/1000 );

	///////////////////////////////////////////////////////////////////////
	// STEP 3: write the cube cover into the allocated storage
	///////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////
	clk1 = clock();
	if ( g_CoverInfo.Verbosity )
	printf( "Generating the starting cover...\n" );
	AddCubesToStartingCover( g_Func.dd );
	///////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////
	// STEP 4: iteratively improve the cover 
	///////////////////////////////////////////////////////////////////////
	if ( g_CoverInfo.Verbosity )
	printf( "Performing minimization...\n" );
	clk1 = clock();
	ReduceEsopCover();
	g_CoverInfo.TimeMin = clock() - clk1;
//	g_Func.TimeMin = (float)(clock() - clk1)/(float)(CLOCKS_PER_SEC);
	if ( g_CoverInfo.Verbosity )
	{
	printf( "\nMinimization time is %.2f sec\n", TICKS_TO_SECONDS(g_CoverInfo.TimeMin) );
	printf( "\nThe number of cubes after minimization is %d\n", g_CoverInfo.nCubesInUse );
	}

	///////////////////////////////////////////////////////////////////////
	// STEP 5: save the cover into file 
	///////////////////////////////////////////////////////////////////////
	// if option is MULTI_OUTPUT, the output is written into the output file;
	// if option is SINGLE_NODE, the output is added to the input file
	// and written into the output file; in this case, the minimized nodes is
	// also stored in the temporary file "temp.blif" for verification

	// create the file name and write the output
	{
		char Buffer[1000];
		sprintf( Buffer, "%s.esop", g_Func.FileGeneric );
		g_Func.FileOutput  = strsav(Buffer );
	    WriteResultIntoFile();
	}

	///////////////////////////////////////////////////////////////////////
	// STEP 6: delocate memory
	///////////////////////////////////////////////////////////////////////
	DelocateCubeSets();
	DelocateCover();
	//	DelocateQueques();
	
	///////////////////////////////////////////////////////////////////////
	// STEP 7: perform the final verification
	///////////////////////////////////////////////////////////////////////
	  //	if ( g_CoverInfo.Verbosity )
	  //	printf( "Reading the output file(s) for verification...\n" );
	  //	// define the data structure which stores the multi-input multi-output function
	  //	// assign the initial parameters
	  //	{
	  //		// prepare to fill in the second function from the file <temp.blif>
	  //		Func2.FileInput = strsav( g_Func.FileOutput );
	  //		Func2.dd = g_Func.dd;
	  //		// read the second file
	  //		if ( Extra_ReadFile( &Func2 ) == 0 )
	  //		{
	  //			printf( "\nSomething did not work out while reading the output file for verification\n" );
	  //			return 1;
	  //		}
	  //	}
	  
	  // verify the result for every output
	  //	fVerificationOkay = 1;
	  // the verification is done using bdds, 
	  // even though the cover is built using ADDs
	  //	assert( Func2.nOutputs == g_Func.nOutputs );
	  //	for ( i = 0; i < g_Func.nOutputs; i++ )
	  //		if ( Func2.pOutputs[i] != g_Func.pOutputs[i] )
	  //		{
	  //			fVerificationOkay = 0;
	  //			printf( "Global Verification is not okay for output #%d\n", i );
	  //			printf( "The number of wrong minterms is %d\n" , (int)Cudd_CountMinterm( g_Func.dd, Cudd_bddXor( g_Func.dd, g_Func.pOutputs[i], Func2.pOutputs[i] ), Cudd_ReadSize( g_Func.dd ) ) );
	  ////			cout << Wrong minterms are " << ReadBdd( g_Func.dd, Cudd_bddXor( g_Func.dd, Func2.pOutputs[i], g_Func.pOutputs[i] ) ) << endl;
	  //			printf( "The original support is\n" );
	  //			Extra_bddPrint( g_Func.dd, Cudd_Support(g_Func.dd, g_Func.pOutputs[i]) );
	  //			printf( "\n" );
	  //			printf( "The resulting output is\n" );
	  //			Extra_bddPrint( g_Func.dd, Cudd_Support(g_Func.dd, Func2.pOutputs[i]) );
	  //			printf( "\n" );
	  //			printf( "The original output is\n" );
	  //			Extra_bddPrint( g_Func.dd, g_Func.pOutputs[i] );
	  //			printf( "\n" );
	  //			printf( "The resulting output is\n" );
	  //			Extra_bddPrint( g_Func.dd, Func2.pOutputs[i] );
	  //			printf( "\n" );
	  //		}
	  //	if ( g_CoverInfo.Verbosity )
	  //	if ( fVerificationOkay )
	  //		printf( "Global verification: okay for all outputs\n" );
	  /////////////////////////////////////////////////////
	  //
	  // // dispose of the temporary function
	  //	Extra_Dissolve( &Func2 );
	  // g_Func will be disposed of in main()

	// return success
	return 1;
}

int ReduceEsopCover()
{
//	printf( "\nThe reduced Pseudo-Kronecker cover is \n" );
//	PrintCoverProfile( cout );
//	PrintCoverDebug( cout );
//	PrintQuequeStats();

//	return 0;

	///////////////////////////////////////////////////////////////
	// SIMPLIFICATION
	////////////////////////////////////////////////////////////////////

  	long clk1 = clock();
	int nIterWithoutImprovement = 0;
	int nIterCount = 0;
	int GainTotal;
	int z;

	do
	{
//START:
		if ( g_CoverInfo.Verbosity == 2 )
			printf( "\nITERATION #%d\n\n", ++nIterCount );
		else if ( g_CoverInfo.Verbosity == 1 )
			printf( "." );

		GainTotal  = 0;
		GainTotal += IterativelyApplyExorLink2( 1|2|0 );
		GainTotal += IterativelyApplyExorLink3( 1|2|0 );

		GainTotal += IterativelyApplyExorLink2( 1|2|0 );
		GainTotal += IterativelyApplyExorLink3( 1|2|0 );

		GainTotal += IterativelyApplyExorLink2( 1|2|0 );
		GainTotal += IterativelyApplyExorLink3( 1|2|0 );

		GainTotal += IterativelyApplyExorLink2( 1|2|0 );
		GainTotal += IterativelyApplyExorLink3( 1|2|0 );

		GainTotal += IterativelyApplyExorLink2( 1|2|0 );
		GainTotal += IterativelyApplyExorLink3( 1|2|0 );

		GainTotal += IterativelyApplyExorLink2( 1|2|0 );
		GainTotal += IterativelyApplyExorLink3( 1|2|0 );

		if ( nIterWithoutImprovement > (int)(g_CoverInfo.Quality>0) )
		{
			GainTotal += IterativelyApplyExorLink2( 1|2|0 );
			GainTotal += IterativelyApplyExorLink3( 1|2|0 );
			GainTotal += IterativelyApplyExorLink2( 1|2|4 );
			GainTotal += IterativelyApplyExorLink3( 1|2|4 );
			GainTotal += IterativelyApplyExorLink2( 1|2|4 );
			GainTotal += IterativelyApplyExorLink4( 1|2|4 );
			GainTotal += IterativelyApplyExorLink2( 1|2|4 );
			GainTotal += IterativelyApplyExorLink4( 1|2|0 );

			GainTotal += IterativelyApplyExorLink2( 1|2|0 );
			GainTotal += IterativelyApplyExorLink3( 1|2|0 );
			GainTotal += IterativelyApplyExorLink2( 1|2|4 );
			GainTotal += IterativelyApplyExorLink3( 1|2|4 );
			GainTotal += IterativelyApplyExorLink2( 1|2|4 );
			GainTotal += IterativelyApplyExorLink4( 1|2|4 );
			GainTotal += IterativelyApplyExorLink2( 1|2|4 );
			GainTotal += IterativelyApplyExorLink4( 1|2|0 );
		}

		if ( GainTotal )
			nIterWithoutImprovement = 0;
		else
			nIterWithoutImprovement++;

//		if ( g_CoverInfo.Quality >= 2 && nIterWithoutImprovement == 2 )
//			s_fDecreaseLiterals = 1;
	}
	while ( nIterWithoutImprovement < 1 + g_CoverInfo.Quality );


	// improve the literal count
	s_fDecreaseLiterals = 1;
	for ( z = 0; z < 1; z++ )
	{
		if ( g_CoverInfo.Verbosity == 2 )
			printf( "\nITERATION #%d\n\n", ++nIterCount );
		else if ( g_CoverInfo.Verbosity == 1 )
			printf( "." );

		GainTotal  = 0;
		GainTotal += IterativelyApplyExorLink2( 1|2|0 );
		GainTotal += IterativelyApplyExorLink3( 1|2|0 );

		GainTotal += IterativelyApplyExorLink2( 1|2|0 );
		GainTotal += IterativelyApplyExorLink3( 1|2|0 );

		GainTotal += IterativelyApplyExorLink2( 1|2|0 );
		GainTotal += IterativelyApplyExorLink3( 1|2|0 );

		GainTotal += IterativelyApplyExorLink2( 1|2|0 );
		GainTotal += IterativelyApplyExorLink3( 1|2|0 );

		GainTotal += IterativelyApplyExorLink2( 1|2|0 );
		GainTotal += IterativelyApplyExorLink3( 1|2|0 );

//		if ( GainTotal )
//		{
//			nIterWithoutImprovement = 0;
//			goto START;
//		}			
	}


/*	////////////////////////////////////////////////////////////////////
	// Print statistics
	printf( "\nShallow simplification time is ";
	cout << (float)(clk2 - clk1)/(float)(CLOCKS_PER_SEC) << " sec\n" );
	printf( "Deep simplification time is ";
	cout << (float)(clock() - clk2)/(float)(CLOCKS_PER_SEC) << " sec\n" );
	printf( "Cover after iterative simplification = " << s_nCubesInUse << endl;
	printf( "Reduced by initial cube writing      = " << g_CoverInfo.nCubesBefore-nCubesAfterWriting << endl;
	printf( "Reduced by shallow simplification    = " << nCubesAfterWriting-nCubesAfterShallow << endl;
	printf( "Reduced by deep simplification       = " << nCubesAfterWriting-s_nCubesInUse << endl;

//	printf( "\nThe total number of cubes created = " << g_CoverInfo.cIDs << endl;
//	printf( "Total number of places in a queque = " << s_nPosAlloc << endl;
//	printf( "Minimum free places in queque-2 = " << s_nPosMax[0] << endl;
//	printf( "Minimum free places in queque-3 = " << s_nPosMax[1] << endl;
//	printf( "Minimum free places in queque-4 = " << s_nPosMax[2] << endl;
*/	////////////////////////////////////////////////////////////////////

	// write the number of cubes into cover information 
	assert ( g_CoverInfo.nCubesInUse + g_CoverInfo.nCubesFree == g_CoverInfo.nCubesAlloc );

//	printf( "\nThe output cover is\n" );
//	PrintCoverDebug( cout );

	return 0;
}

//////////////////////////////////////////////////////////////////
// quite a good script
//////////////////////////////////////////////////////////////////
/*
	long clk1 = clock();
	int nIterWithoutImprovement = 0;
	do
	{
		PrintQuequeStats();
		GainTotal  = 0;
		GainTotal += IterativelyApplyExorLink( DIST2, 0|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST3, 0|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

		if ( nIterWithoutImprovement > 2 )
		{
			GainTotal += IterativelyApplyExorLink( DIST2, 0|0|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 0|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST3, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 1|2|0 );
		}

		if ( nIterWithoutImprovement > 6 )
		{
			GainTotal += IterativelyApplyExorLink( DIST2, 0|0|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 0|0|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 0|0|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 1|2|0 );
		}

		if ( GainTotal )
			nIterWithoutImprovement = 0;
		else
			nIterWithoutImprovement++;
	}
	while ( nIterWithoutImprovement < 12 );

	nCubesAfterShallow = s_nCubesInUse;

*/

/*
	// alu4 - 439
  	long clk1 = clock();
	int nIterWithoutImprovement = 0;
	do
	{
		PrintQuequeStats();
		GainTotal  = 0;
		GainTotal += IterativelyApplyExorLink( DIST2, 1|0|0 );
		GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

		if ( nIterWithoutImprovement > 2 )
		{
			GainTotal += IterativelyApplyExorLink( DIST2, 0|0|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 0|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST3, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 1|2|0 );
		}

		if ( nIterWithoutImprovement > 6 )
		{
			GainTotal += IterativelyApplyExorLink( DIST2, 0|0|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 0|0|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 0|0|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 1|2|0 );
		}

		if ( GainTotal )
			nIterWithoutImprovement = 0;
		else
			nIterWithoutImprovement++;
	}
	while ( nIterWithoutImprovement < 12 );
*/

/*
// alu4 - 412 cubes, 700 sec

  	long clk1 = clock();
	int nIterWithoutImprovement = 0;
	int nIterCount = 0;
	do
	{
		printf( "\nITERATION #" << ++nIterCount << endl << endl;

		GainTotal  = 0;
		GainTotal += IterativelyApplyExorLink2( 1|2|0 );
		GainTotal += IterativelyApplyExorLink2( 1|2|0 );
		GainTotal += IterativelyApplyExorLink3( 1|2|0 );
		GainTotal += IterativelyApplyExorLink3( 1|2|0 );
		GainTotal += IterativelyApplyExorLink3( 1|2|0 );
		GainTotal += IterativelyApplyExorLink3( 1|2|0 );

		if ( nIterWithoutImprovement > 3 )
		{
			GainTotal += IterativelyApplyExorLink2( 1|2|0 );
			GainTotal += IterativelyApplyExorLink3( 1|2|0 );
			GainTotal += IterativelyApplyExorLink3( 1|2|4 );
			GainTotal += IterativelyApplyExorLink2( 1|2|4 );
			GainTotal += IterativelyApplyExorLink3( 1|2|4 );
			GainTotal += IterativelyApplyExorLink4( 1|2|4 );
			GainTotal += IterativelyApplyExorLink3( 1|2|4 );
			GainTotal += IterativelyApplyExorLink4( 1|2|4 );
		}

		if ( nIterWithoutImprovement > 7 )
		{
			GainTotal += IterativelyApplyExorLink2( 1|2|0 );
			GainTotal += IterativelyApplyExorLink3( 1|2|0 );
			GainTotal += IterativelyApplyExorLink2( 1|2|4 );
			GainTotal += IterativelyApplyExorLink3( 1|2|4 );
			GainTotal += IterativelyApplyExorLink3( 1|2|4 );
			GainTotal += IterativelyApplyExorLink4( 1|2|4 );
			GainTotal += IterativelyApplyExorLink4( 1|2|4 );
			GainTotal += IterativelyApplyExorLink4( 1|2|4 );
			GainTotal += IterativelyApplyExorLink3( 1|2|4 );
			GainTotal += IterativelyApplyExorLink4( 1|2|4 );
			GainTotal += IterativelyApplyExorLink4( 1|2|4 );
			GainTotal += IterativelyApplyExorLink4( 1|2|4 );
		}

		if ( GainTotal )
			nIterWithoutImprovement = 0;
		else
			nIterWithoutImprovement++;
	}
	while ( nIterWithoutImprovement < 12 );
*/

/*
// pretty good script
// alu4 = 424   in 250 sec

   	long clk1 = clock();
	int nIterWithoutImprovement = 0;
	int nIterCount = 0;
	do
	{
		printf( "\nITERATION #" << ++nIterCount << "   |";
		for ( int k = 0; k < nIterWithoutImprovement; k++ )
			printf( "*";
		for ( ; k < 11; k++ )
			printf( "_";
		printf( "|" << endl << endl;

		GainTotal  = 0;
		GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

		if ( nIterWithoutImprovement > 2 )
		{
			GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
			GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );
			GainTotal += IterativelyApplyExorLink( DIST2, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST3, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST3, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
		}

		if ( nIterWithoutImprovement > 4 )
		{
			GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
			GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );
			GainTotal += IterativelyApplyExorLink( DIST2, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST3, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST2, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST3, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
		}

		if ( GainTotal )
			nIterWithoutImprovement = 0;
		else
			nIterWithoutImprovement++;
	}
	while ( nIterWithoutImprovement < 7 );
*/

/*
alu4 = 435   70 secs

  	long clk1 = clock();
	int nIterWithoutImprovement = 0;
	int nIterCount = 0;

	do
	{
		printf( "\nITERATION #" << ++nIterCount << endl << endl;

		GainTotal  = 0;
		GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

		GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

		GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

		GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

		GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

		GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );


		if ( GainTotal )
			nIterWithoutImprovement = 0;
		else
			nIterWithoutImprovement++;
	}
	while ( nIterWithoutImprovement < 4 );
*/

/*
  // the best previous
  
  	long clk1 = clock();
	int nIterWithoutImprovement = 0;
	int nIterCount = 0;
	int GainTotal;
	do
	{
		if ( g_CoverInfo.Verbosity == 2 )
		printf( "\nITERATION #" << ++nIterCount << endl << endl;
		else if ( g_CoverInfo.Verbosity == 1 )
		cout << '.';

		GainTotal  = 0;
		GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

		GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

		GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

		GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

		GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

		GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
		GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );

		if ( nIterWithoutImprovement > 1 )
		{
			GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
			GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );
			GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
			GainTotal += IterativelyApplyExorLink( DIST3, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST2, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST2, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 1|2|0 );

			GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
			GainTotal += IterativelyApplyExorLink( DIST3, 1|2|0 );
			GainTotal += IterativelyApplyExorLink( DIST2, 1|2|0 );
			GainTotal += IterativelyApplyExorLink( DIST3, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST2, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST2, 1|2|4 );
			GainTotal += IterativelyApplyExorLink( DIST4, 1|2|0 );
		}

		if ( GainTotal )
			nIterWithoutImprovement = 0;
		else
			nIterWithoutImprovement++;
	}
//	while ( nIterWithoutImprovement < 20 );
//	while ( nIterWithoutImprovement < 7 );
	while ( nIterWithoutImprovement < 1 + g_CoverInfo.Quality );

*/

/*
// the last tried

  	long clk1 = clock();
	int nIterWithoutImprovement = 0;
	int nIterCount = 0;
	int GainTotal;
	do
	{
		if ( g_CoverInfo.Verbosity == 2 )
		printf( "\nITERATION #" << ++nIterCount << endl << endl;
		else if ( g_CoverInfo.Verbosity == 1 )
		cout << '.';

		GainTotal  = 0;
		GainTotal += IterativelyApplyExorLink2( 1|2|0 );
		GainTotal += IterativelyApplyExorLink3( 1|2|0 );

		GainTotal += IterativelyApplyExorLink2( 1|2|0 );
		GainTotal += IterativelyApplyExorLink3( 1|2|0 );

		GainTotal += IterativelyApplyExorLink2( 1|2|0 );
		GainTotal += IterativelyApplyExorLink3( 1|2|0 );

		GainTotal += IterativelyApplyExorLink2( 1|2|0 );
		GainTotal += IterativelyApplyExorLink3( 1|2|0 );

		GainTotal += IterativelyApplyExorLink2( 1|2|0 );
		GainTotal += IterativelyApplyExorLink3( 1|2|0 );

		if ( nIterWithoutImprovement > (int)(g_CoverInfo.Quality>0) )
		{
			GainTotal += IterativelyApplyExorLink2( 1|2|0 );
			GainTotal += IterativelyApplyExorLink3( 1|2|0 );
			GainTotal += IterativelyApplyExorLink2( 1|2|0 );
			GainTotal += IterativelyApplyExorLink3( 1|2|4 );
			GainTotal += IterativelyApplyExorLink2( 1|2|4 );
			GainTotal += IterativelyApplyExorLink4( 1|2|4 );
			GainTotal += IterativelyApplyExorLink2( 1|2|4 );
			GainTotal += IterativelyApplyExorLink4( 1|2|0 );

			GainTotal += IterativelyApplyExorLink2( 1|2|0 );
			GainTotal += IterativelyApplyExorLink3( 1|2|0 );
			GainTotal += IterativelyApplyExorLink2( 1|2|0 );
			GainTotal += IterativelyApplyExorLink3( 1|2|4 );
			GainTotal += IterativelyApplyExorLink2( 1|2|4 );
			GainTotal += IterativelyApplyExorLink4( 1|2|4 );
			GainTotal += IterativelyApplyExorLink2( 1|2|4 );
			GainTotal += IterativelyApplyExorLink4( 1|2|0 );
		}

		if ( GainTotal )
			nIterWithoutImprovement = 0;
		else
			nIterWithoutImprovement++;
	}
	while ( nIterWithoutImprovement < 1 + g_CoverInfo.Quality );
*/

///////////////////////////////////////////////////////////////////
////////////              End of File             /////////////////
///////////////////////////////////////////////////////////////////
