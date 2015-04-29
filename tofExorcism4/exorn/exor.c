////////////////////////////////////////////////////////////////////////
///                                                                  ///
///                  Implementation of EXORCISM - 4                  ///
///              An Exclusive Sum-of-Product Minimizer               ///
///               Alan Mishchenko  <alanmi@ee.pdx.edu>               ///
///                                                                  ///
////////////////////////////////////////////////////////////////////////
///                                                                  ///
///                         Main Module                              ///
///                ESOP Minimization Task Coordinator                ///
///                                                                  ///
///          1) interprets command line                              ///  
///          2) calls the approapriate reading procedure             ///
///          3) calls the minimization module                        ///
///                                                                  ///
///  Ver. 1.0. Started - July 18, 2000. Last update - July 20, 2000  ///
///  Ver. 1.1. Started - July 24, 2000. Last update - July 29, 2000  ///
///  Ver. 1.4. Started -  Aug 10, 2000. Last update -  Aug 26, 2000  ///
///  Ver. 1.6. Started -  Sep 11, 2000. Last update -  Sep 15, 2000  ///
///  Ver. 1.7. Started -  Sep 20, 2000. Last update -  Sep 23, 2000  ///
///                                                                  ///
////////////////////////////////////////////////////////////////////////
///   This software was tested with the BDD package "CUDD", v.2.3.0  ///
///                          by Fabio Somenzi                        ///
///                  http://vlsi.colorado.edu/~fabio/                ///
////////////////////////////////////////////////////////////////////////

#include "exor.h"
#include "extra.h"

///////////////////////////////////////////////////////////////////////
///                      GLOBAL VARIABLES                            ///
////////////////////////////////////////////////////////////////////////

float alphaC = 1.0 ;
float betaC = 1.0 ;
int hasNots = 1 ;
int costFunc = 0 ;
int removeNots = 1 ;

// the function
BFunc g_Func;

// information about the cube cover
cinfo g_CoverInfo;

////////////////////////////////////////////////////////////////////////
///                       EXTERNAL FUNCTIONS                         ///
////////////////////////////////////////////////////////////////////////

// command line parsing
void IntroduceYourself();
void ExplainCommandLine( char* Path );

// preparation
extern void PrepareBitSetModule();

// minimization
extern int Exorcism();


////////////////////////////////////////////////////////////////////////
///                        FUNCTION main()                           ///
////////////////////////////////////////////////////////////////////////

int main( int argc, char* argv[] )
{
	int c;
	FILE * pFile;
	DdManager * dd;
	int RetValue;
	long clk1;

	// set the defaults
	g_CoverInfo.Quality = 2;
	g_CoverInfo.Verbosity = 0;

    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "q:v:a:b:n:c:r:")) != EOF) 
	{
		switch(c) 
		{
		case 'c':
		  costFunc = atoi(util_optarg); //which cost function?
		  break;
		case 'a':
		  alphaC = atoi(util_optarg); //alpha
		  break;
		case 'b':
		  betaC = atoi(util_optarg); //beta
		  break;		  
		case 'n':
		  hasNots = atoi(util_optarg); //use insert not algorithm
		  break;
		case 'r':
		  removeNots = atoi(util_optarg); //remove nots
		  break;
		case 'q':
		  g_CoverInfo.Quality = atoi(util_optarg);
		  break;
		case 'v':
		  g_CoverInfo.Verbosity = atoi(util_optarg);
		  break;
		default:
		  goto usage;
		}
    }

	pFile = fopen( argv[util_optind], "r" );
	if ( pFile == NULL )
	{
		printf( "\nString <%s> does not look like a valid file name\n\n", argv[util_optind] );
		goto usage;
	}

	IntroduceYourself();

	///////////////////////////////////////////////////////////////////////
	// start the package
    dd = Cudd_Init( 0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
	Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );

	// prepare the data structure that stores the multi-output function
	// assign the initial parameters
	g_Func.FileInput = strsav( argv[util_optind] );
	g_Func.dd = dd;

	// call the input file reader	
	clk1 = clock();
	if ( g_CoverInfo.Verbosity )
		printf( "\nReading the input ! file...\n" );
 
	// read the total multi-output function
	if ( Extra_ReadFile( &g_Func ) == 0 )
	{
		printf( "\nSomething did not work out while reading the input file\n");
		goto usage;
	} 

	// find the node count and path count in the shared BDD
	if ( g_CoverInfo.Verbosity )
	{
		int i;
		double PathCount = 0;
		// find the node count and the path count in the shared BDD
		printf( "The number of inputs is %d.   The number of outputs is %d.\n", g_Func.nInputs, g_Func.nOutputs );
		for ( i = 0; i < g_Func.nOutputs; i++ )
			PathCount += Cudd_CountPath(g_Func.pOutputs[i]);
		printf( "Shared BDD node count after reading is %d\n", Cudd_SharingSize( g_Func.pOutputs, g_Func.nOutputs ) );
		printf( "Shared path count after reading is %d\n", (int) PathCount );
		printf( "The input file reading time is %.2f sec\n", TICKS_TO_SECONDS(clock() - clk1) );
	}


    // reorder variables
	if ( g_CoverInfo.Verbosity )
		printf( "Reordering variables...\n" );
	Cudd_ReduceHeap(dd,CUDD_REORDER_SYMM_SIFT,1);
	if ( g_CoverInfo.Verbosity )
	{
		// find the node count in the shared BDD
		printf( "Shared BDD node count after reordering is %d\n", Cudd_SharingSize( g_Func.pOutputs, g_Func.nOutputs ) );
		printf( "Variable reordering time is %.2f sec\n", TICKS_TO_SECONDS(clock() - clk1) );
	}
	g_CoverInfo.TimeRead = clock() - clk1;
	Cudd_AutodynDisable(dd);
	///////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////
	// ESOP MINIMIZATION
	///////////////////////////////////////////////////////////////////////
	// general preparation
	PrepareBitSetModule();
	if ( Exorcism() == 0 )
	{
		printf( "Something went wrong when minimizing the cover\n" );
		return 0;
	}
//	if ( g_CoverInfo.Verbosity )
//	printf( "Minimum cover has been written into file <%s>\n", g_Func.FileOutput );
	///////////////////////////////////////////////////////////////////////

	Extra_Dissolve( &g_Func );

	// debugging
	// check that there are no referenced BDDs in the package
	if ( g_CoverInfo.Verbosity )
	{
		RetValue = Cudd_CheckZeroRef( dd );
		printf( "\nThe number of referenced nodes = %d\n", RetValue );
	}

	// shut down the package
	Cudd_Quit( dd );
	return 1;

usage:
	ExplainCommandLine(argv[0]);
	return 0;
}




void IntroduceYourself()
{
	printf( "\nToffoli Cascade Gen, Ver.0.1" );
	printf( "\nby Kenneth Fazel" ) ;
	printf( "\nbased on EXORCISM, Ver.4.7" );
	printf( "\nby Alan Mishchenko\n" );
}

void ExplainCommandLine( char * ProgName )
{
	fprintf( stderr, "\n" );
	fprintf( stderr, "Usage: %s [-q n] [-v n] file1\n", ProgName );
	fprintf( stderr, "\n" );
	fprintf( stderr, "       Performs heuristic exclusive sum-of-project minimization and tfc cascade generation\n" );
	fprintf( stderr, "\n" );
	fprintf( stderr, "        -n {0,1} : insert nots = 1\n") ;
	fprintf( stderr, "        -r {0,1} : remove nots = 1\n") ;
	fprintf( stderr, "        -a f : alpha\n") ;
	fprintf( stderr, "        -b f : beta\n") ;
	fprintf( stderr, "        -c {0,1} : cost function = 1\n") ;
	fprintf( stderr, "        -q n : minimization quality [default = 0]\n");
	fprintf( stderr, "               increasing this number improves quality and adds to runtime\n");
	fprintf( stderr, "        -v n : verbosity level [default = 0]\n");
	fprintf( stderr, "               0 = no output; 1 = outline; 2 = verbose\n");
	fprintf( stderr, "        file1: the input file in PLA (*.pla), ESOP (*.esop) or BLIF (*.blif)\n");
	fprintf( stderr, "               the program detects the format from the file extension\n");
	fprintf( stderr, "               in case of BLIF, minimization is applied the flattened network\n");
	fprintf( stderr, "\n" );
}

	//////////////////////////////////////////////////////////////////////////
	// write the shard bdds into the blif file
//    FILE* pFileDot = fopen( "g_FuncB.dot", "w" );
//    Cudd_DumpDot( g_Func.ddman, g_Func.nOutputs, g_Func.pOutputs, NULL, NULL, pFileDot );
//    fclose( pFileDot );
	//////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
////////////              End of File             /////////////////
///////////////////////////////////////////////////////////////////

