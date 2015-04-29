/**CFile***********************************************************************

  FileName    [zEspresso.c]

  PackageName [extra]

  Synopsis    [DD interface to Espresso.]

  Description [External procedures included in this module:
		    <ul>
		    <li> Extra_bddEspresso();
		    <li> Extra_zddEspresso();
		    <li> Extra_zddEspressoMO();
		    </ul>
	       Internal procedures included in this module:
		    <ul>
		    </ul>
	       Static procedures included in this module:
		    <ul>
			<li> zddEspressoEnumeratePaths();
			<li> zddEspressoEnumeratePathsMO();
		    </ul>
	      ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$zEspresso.c, v.1.2, April 26, 2001, alanmi $]

******************************************************************************/

#include "util.h"
#include "time.h"
#include "cuddInt.h"
#include "st.h"
#include "extra.h"
#include "espresso.h"

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

//extern struct cube_struct cube;
static int  * s_pVarValues;
int s_nIns;
int s_nOuts;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static void zddEspressoEnumeratePaths ARGS((DdManager *dd, DdNode *zCover, int levPrev, pcover CubeSet ));
static void zddEspressoEnumeratePathsMO ARGS((DdManager *dd, DdNode *zCover, int levPrev, pcover CubeSet ));

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [A wrapper for the call to Espresso minimization of a single-output function.]

  Description [This function takes the bOn and the bOnDc represented as 
  BDDs in the manager dd and returns the ZDD of a SOP heuristically minimized 
  using the Espresso minimizer. If nCubes == NULL, the number of cubes in the 
  resulting cover is not returned. If called with pPhases == NULL, 
  no phase-assignment is performed.]

  SideEffects [Changes settings of the omnipresent Espresso "cube" structure.]

  SeeAlso     []

******************************************************************************/
DdNode * 
Extra_bddEspresso( 
  DdManager * dd, 
  DdNode * bOn, 
  DdNode * bOnDc, 
  int * nCubes, 
  int * Phase )
{
	DdNode * zOn, * bDc, * zDc, * zRes;
	zOn = Extra_zddIsopCover( dd, bOn, bOn );         Cudd_Ref( zOn );
	bDc = Cudd_bddAnd( dd, bOnDc, Cudd_Not(bOn) );   Cudd_Ref( bDc ); 
	zDc = Extra_zddIsopCover( dd, bDc, bDc );         Cudd_Ref( zDc );
	zRes= Extra_zddEspresso( dd, zOn, zDc, nCubes, Phase );  Cudd_Ref( zRes );
	Cudd_RecursiveDerefZdd( dd, zOn );
	Cudd_RecursiveDerefZdd( dd, zDc );
	Cudd_RecursiveDeref( dd, bDc );
	Cudd_Deref( zRes );
	return zRes;
}


/**Function********************************************************************

  Synopsis    [A wrapper for the call to Espresso minimization of a single-output function.]

  Description [This function takes the On-Set and the Dc-Set represented as 
  ZDDs in the manager dd and returns the ZDD of a SOP heuristically minimized 
  using the Espresso minimizer. If Dc-Set is NULL, only the on-set is considered. 
  If nCubes == NULL, the number of cubes in the resulting cover is not returned. 
  If called with pPhases == NULL, no phase-assignment is performed.]

  SideEffects [Changes settings of the omnipresent Espresso "cube" structure.]

  SeeAlso     []

******************************************************************************/
DdNode * 
Extra_zddEspresso( 
  DdManager * dd, 
  DdNode * zCover, 
  DdNode * zCoverDC, 
  int * nCubes, 
  int * Phase )
{
	pPLA PLA;
	long clk1;
	DdNode * zTemp, * zCube, * zResult;

	// set the number of input varibles 
	int nInputs = dd->sizeZ/2;
	// allocate storage for variable values
	s_pVarValues = (int*) malloc( dd->sizeZ * sizeof(int) );

	// prepare the cube (storing information about the cover)
//	define_cube_size(nInputs);
//	s_pLocalCube = get_cube();

	cube.num_binary_vars = nInputs;
	cube.num_vars = cube.num_binary_vars + 1;
	cube.part_size = ALLOC(int, cube.num_vars);
	cube.part_size[cube.num_vars-1] = 1;  // the number of outputs
	cube_setup();

    // allocate and initialize the PLA structure 
	PLA = new_PLA();
	PLA->pla_type = FD_type;  //  #define FD_type 3
	PLA->F = sf_new(10, cube.size);
	PLA->D = sf_new(10, cube.size);

	clk1 = clock();
	// create the on-set
	zddEspressoEnumeratePaths( dd, zCover, -1, PLA->F );
	// create the dc-set
	if ( zCoverDC )
	zddEspressoEnumeratePaths( dd, zCoverDC, -1, PLA->D );
//	printf("\nPath enumeration time = %.2f sec\n", (float)(clock() - clk1)/(float)(CLOCKS_PER_SEC) );

	// create the off-set
	PLA->R = complement(cube2list(PLA->F, PLA->D));
	assert(PLA->R);

	// at this point, the PLA is ready for minimization

	// call the minimizer
	if ( Phase == NULL )
		PLA->F = espresso(PLA->F, PLA->D, PLA->R);
	else
	{
		int first;
		int strategy = 0; // 0=heuristic, 1=exact
		so_both_espresso(PLA, strategy);

		// borrowed from "cvrout.c"
		first = cube.first_part[cube.output];
		*Phase = is_in_set(PLA->phase,first) ? 1: 0;
	}

	// set the number of cubes in the minimized PLA
	if ( nCubes )
	*nCubes = (PLA->F)->count;

	// convert the PLA back into ZDD
	zResult = dd->zero;
	Cudd_Ref( zResult );

	{
	int var, ch;
    register pcube last, p;
	clk1 = clock();
    foreach_set(PLA->F, last, p) 
	{ 
	    for(var = 0; var < nInputs; var++) 
		{
			ch = "?01-" [GETINPUT(p, var)];
			// set the var values
			if ( ch == '0' )
			{
				s_pVarValues[2*var+0] = 0;
				s_pVarValues[2*var+1] = 1;
			}
			else if ( ch == '1' )
			{
				s_pVarValues[2*var+0] = 1;
				s_pVarValues[2*var+1] = 0;
			}
			else if ( ch == '-' )
			{
				s_pVarValues[2*var+0] = 0;
				s_pVarValues[2*var+1] = 0;
			}
			else
				assert( 0 );
		}

		// create the combination
		zCube = Extra_zddCombination( dd, s_pVarValues, dd->sizeZ );
		Cudd_Ref( zCube );
		// add this combination to the result
		zResult = Cudd_zddUnion( dd, zTemp = zResult, zCube );
		Cudd_Ref( zResult );
		Cudd_RecursiveDerefZdd( dd, zTemp );
		Cudd_RecursiveDerefZdd( dd, zCube );		
	}
// 	printf("\nZDD generation time = %.2f sec\n", (float)(clock() - clk1)/(float)(CLOCKS_PER_SEC) );
	}

	free_PLA(PLA);
	free(s_pVarValues);
//    sf_cleanup();   /* free unused set structures */
//    sm_cleanup();   /* sparse matrix cleanup      */

	// free the cube/cdata structure data 
    FREE(cube.part_size);
    setdown_cube();             

	// dereference the result and return
	Cudd_Deref( zResult );
	return zResult;
}

/**Function********************************************************************

  Synopsis    [A wrapper for the call to Espresso minimization of a MO function.]

  Description [This function takes the On-Set and the Dc-Set represented as 
  ZDDs in the manager dd and returns the ZDD of a SOP heuristically minimized 
  using the Espresso minimizer. If Dc-Set is NULL, only the on-set is considered. 
  If nCubes == NULL, the number of cubes in the resulting cover is not returned. 
  If called with pPhases == NULL, no phase-assignment is performed.]

  SideEffects [Changes settings of the omnipresent Espresso "cube" structure.]

  SeeAlso     []

******************************************************************************/
DdNode * 
Extra_zddEspressoMO( 
  DdManager * dd,    /* the manager */
  DdNode * zCover,   /* the cover of on-set cubes */
  DdNode * zCoverDC, /* the cover of dc-set cubes */
  int nIns, 
  int nOuts, 
  int * nCubes, 
  int * pPhases )
{
	pPLA PLA;
	long clk1;
	DdNode * zTemp, * zCube, * zResult;

	s_nIns  = nIns;
	s_nOuts = nOuts;

	assert( dd->sizeZ/2 == nIns + nOuts );
	// allocate storage for variable values
	s_pVarValues = (int*) malloc( dd->sizeZ * sizeof(int) );

	// prepare the cube (storing information about the cover)

//	define_cube_size(nInputs);
//	s_pLocalCube = get_cube();

	cube.num_binary_vars = nIns;
	cube.num_vars = cube.num_binary_vars + 1;
	cube.part_size = ALLOC(int, cube.num_vars);
	cube.part_size[cube.num_vars-1] = nOuts; // the number of outputs
	cube_setup();

    // allocate and initialize the PLA structure 
	PLA = new_PLA();
	PLA->pla_type = FD_type;  //  #define FD_type 3
	PLA->F = sf_new(10, cube.size);
	PLA->D = sf_new(10, cube.size);

	clk1 = clock();
	// create the on-set
	zddEspressoEnumeratePathsMO( dd, zCover, -1, PLA->F );
	// create the dc-set
	if ( zCoverDC )
	zddEspressoEnumeratePathsMO( dd, zCoverDC, -1, PLA->D );
//	printf("\nPath enumeration time = %.2f sec\n", (float)(clock() - clk1)/(float)(CLOCKS_PER_SEC) );

	// create the off-set
	PLA->R = complement(cube2list(PLA->F, PLA->D));
	assert(PLA->R);

	// at this point, the PLA is ready for minimization

	//////////////////////////////////////////////////////////////
	// call the minimizer
	if ( pPhases == NULL )
		PLA->F = espresso(PLA->F, PLA->D, PLA->R);
	else
	{
		int first, last, i;

		int opo_strategy = 0;
		phase_assignment( PLA, opo_strategy );

		assert( PLA->phase != (pcube)NULL );

		first = cube.first_part[cube.output];
		last  = cube.last_part[cube.output];
		for(i = first; i <= last; i++)
			if ( is_in_set(PLA->phase,i) )
				pPhases[i-first] = 1;
			else 
				pPhases[i-first] = 0;

		// the source of some of this tricks and hacks can be 
		// traced down to read_pla() and other functions 
		// in "cvrin.cpp" belonging to the Espresso package)
	}
	//////////////////////////////////////////////////////////////


	// set the number of cubes in the minimized PLA
	if ( nCubes )
		*nCubes = (PLA->F)->count;

	// convert the PLA back into ZDD
	zResult = dd->zero;
	Cudd_Ref( zResult );

	{
	int var, ch, start, stop, i;
    register pcube last, p;

 	clk1 = clock();
    foreach_set(PLA->F, last, p) 
	{ 
	    for(var = 0; var < nIns; var++) 
		{
			ch = "?01-" [GETINPUT(p, var)];
			// set the var values
			if ( ch == '0' )
			{
				s_pVarValues[2*var+0] = 0;
				s_pVarValues[2*var+1] = 1;
			}
			else if ( ch == '1' )
			{
				s_pVarValues[2*var+0] = 1;
				s_pVarValues[2*var+1] = 0;
			}
			else if ( ch == '-' )
			{
				s_pVarValues[2*var+0] = 0;
				s_pVarValues[2*var+1] = 0;
			}
			else
				assert( 0 );
		}

		assert( cube.output != -1 );

		start = cube.first_part[cube.output];
		stop  = cube.last_part [cube.output];
		for(i = start; i <= stop; i++) 
		{
			ch = "01" [is_in_set(p, i) != 0];
			// set the var values
			if ( ch == '0' )
			{ // "0" in PLA = "-" in the cover

//				s_pVarValues[start + 2*(i-start)+0] = 0;
//				s_pVarValues[start + 2*(i-start)+1] = 1;

				s_pVarValues[start + 2*(i-start)+0] = 0;
				s_pVarValues[start + 2*(i-start)+1] = 0;
			}
			else if ( ch == '1' )
			{ // "1" in PLA = "1" in the cover

//				s_pVarValues[start + 2*(i-start)+0] = 0;
//				s_pVarValues[start + 2*(i-start)+1] = 0;

				s_pVarValues[start + 2*(i-start)+0] = 1;
				s_pVarValues[start + 2*(i-start)+1] = 0;
			}
			else
				assert( 0 );
		}


		// create the combination
		zCube = Extra_zddCombination( dd, s_pVarValues, dd->sizeZ );
		Cudd_Ref( zCube );
		// add this combination to the result
		zResult = Cudd_zddUnion( dd, zTemp = zResult, zCube );
		Cudd_Ref( zResult );
		Cudd_RecursiveDerefZdd( dd, zTemp );
		Cudd_RecursiveDerefZdd( dd, zCube );		
	}
// 	printf("\nZDD generation time = %.2f sec\n", (float)(clock() - clk1)/(float)(CLOCKS_PER_SEC) );
	}

	free_PLA(PLA);
	free(s_pVarValues);
//    sf_cleanup();   //free unused set structures 
//    sm_cleanup();   //sparse matrix cleanup      

	// free the cube/cdata structure data 
    FREE(cube.part_size);
    setdown_cube();             

	// dereference the result and return
	Cudd_Deref( zResult );
	return zResult;

} /* end of zddEspressoEnumeratePathsMO */


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [ZDD traversal function, which fills Espresso PLA structure.]

  Description [Enumerates the cubes of zCover and writes them into s_pVerValues.
  Upon reaching the bottom level, adds the cube to the PLA.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void 
zddEspressoEnumeratePaths( 
  DdManager *dd, 
  DdNode *zCover, 
  int levPrev, 
  pcover CubeSet 
)
{
	int lev;
	if ( zCover == dd->zero )
		return;
	if ( zCover == dd->one )
	{
		int var;
		pcube cf;
		/* fill in the remaining variables */
		for ( lev = levPrev + 1; lev < dd->sizeZ; lev++ )
			s_pVarValues[ dd->invpermZ[ lev ] ] = 0;

		/* add the cube to the PLA */
		cf = cube.temp[0];
		set_clear(cf, cube.size);
	    for(var = 0; var < dd->sizeZ/2; var++)
			if ( s_pVarValues[var*2] ) 
				// the value in ZDD is 1 - set 1 in the cube
				set_insert(cf, var*2+1);
			else if	( s_pVarValues[var*2+1] ) 
				// the value in ZDD is 0 - set 0 in the cube
				set_insert(cf, var*2);
			else 
			{   // the value in ZDD is DC - set both(0,1) in the cube
				set_insert(cf, var*2);
				set_insert(cf, var*2+1);
			}
		// set the output value
		set_insert(cf, cube.first_part[var]);
		// add the cube
		CubeSet = sf_addset(CubeSet, cf);
		return;
	}
	else
	{
		/* find the level of the top variable */
		int TopLevel = dd->permZ[ zCover->index ];

		/* fill in the remaining variables */
		for ( lev = levPrev + 1; lev < TopLevel; lev++ )
			s_pVarValues[ dd->invpermZ[ lev ] ] = 0;

		/* fill in this variable */
		/* the given var has negative polarity */
		s_pVarValues[ zCover->index ] = 0;
		// iterate through the else branch
		zddEspressoEnumeratePaths( dd, cuddE(zCover), TopLevel, CubeSet );
		
		// the given var has positive polarity
		s_pVarValues[ zCover->index ] = 1;
		// iterate through the then branch
		zddEspressoEnumeratePaths( dd, cuddT(zCover), TopLevel, CubeSet );
	}
} /* end of zddEspressoEnumeratePaths */


/**Function********************************************************************

  Synopsis    [ZDD traversal function, which fills Espresso PLA structure.]

  Description [Enumerates the cubes of zCover and writes them into s_pVerValues.
  Upon reaching the bottom level, adds the cube to the PLA.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
void 
zddEspressoEnumeratePathsMO( 
  DdManager *dd, 
  DdNode *zCover, 
  int levPrev, 
  pcover CubeSet 
)
{
	int lev;
	if ( zCover == dd->zero )
		return;
	if ( zCover == dd->one )
	{
		int var, i;
		int start = cube.first_part[cube.output];
		int stop  = cube.last_part [cube.output];

		/* prepare the cube to be added to the PLA */
		pcube cf = cube.temp[0];
		set_clear(cf, cube.size);

		/* fill in the remaining variables */
		for ( lev = levPrev + 1; lev < dd->sizeZ; lev++ )
			s_pVarValues[ dd->invpermZ[ lev ] ] = 0;

		/* write the input variables */
	    for(var = 0; var < s_nIns; var++)
			if ( s_pVarValues[var*2] ) 
				// the value in ZDD is 1 - set 1 in the cube
				set_insert(cf, var*2+1);
			else if	( s_pVarValues[var*2+1] ) 
				// the value in ZDD is 0 - set 0 in the cube
				set_insert(cf, var*2);
			else 
			{   // the value in ZDD is DC - set both(0,1) in the cube
				set_insert(cf, var*2);
				set_insert(cf, var*2+1);
			}
		// set the output value
//		set_insert(cf, cube.first_part[var]);

		/* write the output variables */
		for(i = start; i <= stop; i++) 
			if ( s_pVarValues[start + 2*(i-start)+1] )
			{	// "0" in ZDD = "0" in the output part of the PLA
			}
			else if ( s_pVarValues[start + 2*(i-start)+0] )
			{	// "1" in ZDD - cannot be
				assert(0);
			}
			else
			{   // "-" in ZDD = "1" in the output part of the PLA
				set_insert(cf, i);
			}

		// add the cube
		CubeSet = sf_addset(CubeSet, cf);
		return;
	}
	else
	{
		/* find the level of the top variable */
		int TopLevel = dd->permZ[ zCover->index ];

		/* fill in the remaining variables */
		for ( lev = levPrev + 1; lev < TopLevel; lev++ )
			s_pVarValues[ dd->invpermZ[ lev ] ] = 0;

		/* fill in this variable */
		/* the given var has negative polarity */
		s_pVarValues[ zCover->index ] = 0;
		// iterate through the else branch
		zddEspressoEnumeratePathsMO( dd, cuddE(zCover), TopLevel, CubeSet );
		
		// the given var has positive polarity
		s_pVarValues[ zCover->index ] = 1;
		// iterate through the then branch
		zddEspressoEnumeratePathsMO( dd, cuddT(zCover), TopLevel, CubeSet );
	}
} /* end of zddEspressoEnumeratePathsMO */
