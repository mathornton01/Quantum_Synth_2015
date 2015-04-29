////////////////////////////////////////////////////////////////////////
///                                                                  ///
///                  Implementation of EXORCISM - 4                  ///
///              An Exclusive Sum-of-Product Minimizer               ///
///                                                                  ///
///               Alan Mishchenko  <alanmi@ee.pdx.edu>               ///
///                                                                  ///
////////////////////////////////////////////////////////////////////////
///                                                                  ///
///                    Starting Cover Computation                    ///
///                                                                  ///
////////////////////////////////////////////////////////////////////////
///   This software was tested with the BDD package "CUDD", v.2.3.1  ///
///                          by Fabio Somenzi                        ///
///                  http://vlsi.colorado.edu/~fabio/                ///
////////////////////////////////////////////////////////////////////////

#include "exor.h"
#include "extra.h"

////////////////////////////////////////////////////////////////////////
///                       EXTERNAL VARIABLES                         ///
////////////////////////////////////////////////////////////////////////

// information about the options, the function, and the cover
extern cinfo g_CoverInfo;

////////////////////////////////////////////////////////////////////////
///                      EXTERNAL FUNCTIONS                          ///
////////////////////////////////////////////////////////////////////////
extern void InsertVarsWithoutClearing( Cube* pC, int* pVars, int nVarsIn, int* pVarValues, int Output );
extern Cube* GetFreeCube();
extern int CheckForCloseCubes( Cube* p, int fAddCube );

////////////////////////////////////////////////////////////////////////
///                    FUNCTIONS OF THIS MODULE                      ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                        STATIC VARIABLES                          ///
////////////////////////////////////////////////////////////////////////

// temporary storage for starting ISOP covers
static DdNode ** s_pzCovers;

////////////////////////////////////////////////////////////////////////
///                      FUNCTION DEFINITIONS                        ///
////////////////////////////////////////////////////////////////////////

int GetNumberOfCubes( DdManager * dd, DdNode ** pbFuncs, int nFuncs )
{
	int nCubes;
	// alloc ZDD variables
	Cudd_zddVarsFromBddVars( dd, 2 );
	s_pzCovers = ALLOC( DdNode *, nFuncs );
	nCubes = Extra_zddFastEsopCoverArray( dd, pbFuncs, s_pzCovers, nFuncs );
	return nCubes;
}

void AddCubesToStartingCover( DdManager * dd )
{
	int Out;
	DdNode * zCover, * zCube, * zTemp;
	Cube* pNew;

	int * s_Level2Var;
	int * s_LevelValues;
	int i;

	int nLiterals;

	s_Level2Var = ALLOC( int, g_CoverInfo.nVarsIn );
	s_LevelValues = ALLOC( int, g_CoverInfo.nVarsIn );

	for ( i = 0; i < g_CoverInfo.nVarsIn; i++ )
		s_Level2Var[i] = i;

	g_CoverInfo.nLiteralsBefore = 0;

	for ( Out = 0; Out < g_CoverInfo.nVarsOut; Out++ )
	{
		zCover = s_pzCovers[Out];  Cudd_Ref( zCover );
		while ( zCover != dd->zero )
		{
			nLiterals = 0;
			//////////////////////////////////////////////////////////////////////////////
			// get the cube
			zCube  = Extra_zddSelectOneSubset( dd, zCover );    Cudd_Ref( zCube );
			//////////////////////////////////////////////////////////////////////////////

			// fill in the cube with blanks
			for ( i = 0; i < g_CoverInfo.nVarsIn; i++ )
				s_LevelValues[i] = VAR_ABS;
			for ( zTemp = zCube; zTemp != dd->one; zTemp = cuddT(zTemp) )
			{
				nLiterals++;
				if ( zTemp->index % 2 == 0 )
					s_LevelValues[zTemp->index/2] = VAR_POS;
				else
					s_LevelValues[zTemp->index/2] = VAR_NEG;
			}


			// get the new cube
			pNew = GetFreeCube();
			// consider the need to clear the cube
			if ( pNew->pCubeDataIn[0] ) // this is a recycled cube
			{
				for ( i = 0; i < g_CoverInfo.nWordsIn; i++ )
					pNew->pCubeDataIn[i] = 0;
				for ( i = 0; i < g_CoverInfo.nWordsOut; i++ )
					pNew->pCubeDataOut[i] = 0;
			}

			InsertVarsWithoutClearing( pNew, s_Level2Var, g_CoverInfo.nVarsIn, s_LevelValues, Out );
			// set literal counts
			pNew->a = nLiterals;
			pNew->z = 1;
			// set the ID
			pNew->ID = g_CoverInfo.cIDs++;
			// skip through zero-ID
			if ( g_CoverInfo.cIDs == 256 )
				g_CoverInfo.cIDs = 1;

		//	cout << "Kro" << endl;
		//	PrintCube( cout, pNew );
		//	cout << endl;

			// add this cube to storage
			CheckForCloseCubes( pNew, 1 );

			g_CoverInfo.nLiteralsBefore += nLiterals;

			//////////////////////////////////////////////////////////////////////////////
			// update the cover and deref the cube
			zCover = Cudd_zddDiff( dd, zTemp = zCover, zCube ); Cudd_Ref( zCover );
			Cudd_RecursiveDerefZdd( dd, zTemp );
			Cudd_RecursiveDerefZdd( dd, zCube );
			//////////////////////////////////////////////////////////////////////////////
		}
		Cudd_RecursiveDerefZdd( dd, zCover );
	}
	FREE( s_Level2Var );
	FREE( s_LevelValues );

	assert ( g_CoverInfo.nCubesInUse + g_CoverInfo.nCubesFree == g_CoverInfo.nCubesAlloc );


	for ( Out = 0; Out < g_CoverInfo.nVarsOut; Out++ )
		Cudd_RecursiveDerefZdd( dd, s_pzCovers[Out] );
	FREE( s_pzCovers );
}
