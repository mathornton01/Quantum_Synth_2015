/**CFile***********************************************************************

  FileName    [bNetRead.c]

  PackageName [EXTRA]

  Synopsis    [PLA and BLIF file reader.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_ReadFile();
				<li> Extra_Dissolve();
				</ul>
			   Internal procedures included in this module:
				<ul>
				</ul>
			   Static procedures included in this module:
				<ul>
				<li> ReadPla();
				<li> ReadBlif();
				<li> ntrInitializeCount();
				<li> ntrCountDFS();
				</ul>
	          ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$bNetRead.c, v.1.2, July 27, 2001, alanmi $]

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

// local functions
static int ReadPla( BFunc * pFunc );
static int ReadBlif( BFunc * pFunc );
// these functions read pla/blif file <pFunc->FileInput> 
// return 1 on success, 0 on failure

// utility functions borrowed from NANOTRAV Project
static void ntrInitializeCount( BnetNetwork * net, int stateOnly );
static void ntrCountDFS( BnetNetwork * net, BnetNode * node );

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Derived the BFunc structure]

  Description [This function opens the input file <pFunc->FileInput>, reads the 
  contents of input file using the BFunc reader, and derives BDDs for the primary 
  outputs using the manager <pFunc->dd> and returns them in <pFunc->pOutputs> 
  together with other information. The function returns 1 on success, 0 on failure.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
int
Extra_ReadFile( BFunc * pFunc )
{
	// find the pointer to the "." symbol in the file name
	char *pDot = strstr( pFunc->FileInput, "." );
	// find the generic name of the file
	pFunc->FileGeneric = util_strsav( pFunc->FileInput );
	if ( pDot )
		pFunc->FileGeneric[pDot - pFunc->FileInput] = 0;

	// determine what kind of file is it
	if ( strstr( pFunc->FileInput, ".pla" )
		 || strstr( pFunc->FileInput, ".esop" ) )
		return ReadPla( pFunc );
	else if ( strstr( pFunc->FileInput, ".blif" ) )
		return ReadBlif( pFunc );
	else
	{
		fprintf( stderr, "The input file is neither PLA nor BLIF\n" );
		return 0;
	}
}

/**Function********************************************************************

  Synopsis    [Delocates the BFunc object when out of use]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void
Extra_Dissolve( BFunc * pFunc )
// deletes the BFunc object when out of use
{
	// dereference BDDs
	int i;
	if ( pFunc->pOutputs )
		for ( i = 0; i < pFunc->nOutputs; i++ )
			Cudd_IterDerefBdd( pFunc->dd, pFunc->pOutputs[i] );
	if ( pFunc->pOutputDcs )
		for ( i = 0; i < pFunc->nOutputs; i++ )
			Cudd_IterDerefBdd( pFunc->dd, pFunc->pOutputDcs[i] );
	if ( pFunc->pOutputOffs )
		for ( i = 0; i < pFunc->nOutputs; i++ )
			Cudd_IterDerefBdd( pFunc->dd, pFunc->pOutputOffs[i] );

	// delete allocated memory
	if ( pFunc->pInputs )
		free( pFunc->pInputs );
	if ( pFunc->pInputNames )
		free( pFunc->pInputNames );

	if ( pFunc->pOutputs )
		free( pFunc->pOutputs );
	if ( pFunc->pOutputDcs )
		free( pFunc->pOutputDcs );
	if ( pFunc->pOutputOffs )
		free( pFunc->pOutputOffs );
	if ( pFunc->pOutputNames )
		free( pFunc->pOutputNames );
}

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int
ReadPla( BFunc * pFunc )
{
	int i, k, nTerms;
	FILE *Input;
	char Str[MAXLINE];
	char Str2[MAXLINE];
	char *pBuff1, *pBuff2, *pTemp;

	int LineCounter = 0;
	int LiteralCounter = 0;
	DdNode *bTemp, *bInputCube;
	DdManager *dd = pFunc->dd;


	// open the input file
	Input = fopen( pFunc->FileInput, "r" );
	if ( !Input )
	{
		fprintf( stderr, "ReadPla(): Cannot open input file <%s>",
				 pFunc->FileInput );
		return 0;
	}

	// start writing information about the network
	pFunc->Model = pFunc->FileGeneric;
	pFunc->nLatches = 0;

	// set the defaults
	pFunc->FileType = fd;
	pFunc->nInputs = -1;
	pFunc->nOutputs = -1;
	nTerms = -1;

	// read the header
	while ( 1 )
	{
		// scan the next line from the input stream (skip the empty and comment lines)
		while ( ( pTemp = fgets( Str, MAXLINE, Input ) )
				&& ( Str[0] == '\r' || Str[0] == '\n' || Str[0] == '#' ) );

		if ( !pTemp )
		{
			fprintf( stderr,
					 "ReadPla(): The header of the Pla file ends abruptly (cannot read the next line from the input stream)" );
			return 0;
		}

		// scan the first part of this line
		pBuff1 = strtok( Str, " \t\n" );

		// determine what line of the Pla header is this
		if ( strcmp( pBuff1, ".i" ) == 0 )
		{
			pTemp = strtok( NULL, " \t\n" );
			if ( !pTemp )
			{
				fprintf( stderr,
						 "ReadPla(): Cannot read the number of inputs" );
				return 0;
			}
			pFunc->nInputs = atoi( pTemp );

			if ( pFunc->nInputs < 1 || pFunc->nInputs > MAXINPUTS )
			{
				fprintf( stderr,
						 "ReadPla(): The number of inputs (%d) is less than 1 or more than MAXINPUTS (%d)",
						 pFunc->nInputs, MAXINPUTS );
				return 0;
			}

			pBuff1 = strtok( NULL, " \t\n" );
			if ( pBuff1 && strlen( pBuff1 ) != 0 )
			{
				fprintf( stderr,
						 "ReadPla(): The following extra character(s) <%s> appear at the end of .i line",
						 pBuff1 );
				return 0;
			}

			// allocate memory
			pFunc->pInputs =
				( DdNode ** ) malloc( pFunc->nInputs * sizeof( int ) );
			pFunc->pInputNames =
				( char ** ) malloc( pFunc->nInputs * sizeof( char * ) );
			pFunc->pInputNames[0] = NULL;

			// create elementary variables in the manager and write them into the array
			for ( i = 0; i < pFunc->nInputs; i++ )
				pFunc->pInputs[i] = Cudd_bddIthVar( dd, i );
		}
		else if ( strcmp( pBuff1, ".o" ) == 0 )
		{
			pTemp = strtok( NULL, " \t\n" );
			if ( !pTemp )
			{
				fprintf( stderr,
						 "ReadPla(): Cannot read the number of outputs" );
				return 0;
			}
			pFunc->nOutputs = atoi( pTemp );

			if ( pFunc->nOutputs <= 0 || pFunc->nOutputs > MAXOUTPUTS )
			{
				fprintf( stderr,
						 "ReadPla(): The number of outputs (%d) is less than 1 or more than MAXOUTPUTS (%d)",
						 pFunc->nOutputs, MAXOUTPUTS );
				return 0;
			}

			pBuff1 = strtok( NULL, " \t\n" );
			if ( pBuff1 && strlen( pBuff1 ) != 0 )
			{
				fprintf( stderr,
						 "ReadPla(): The following extra character(s) <%s> appear at the end of .o line",
						 pBuff1 );
				return 0;
			}

			// allocate memory
			pFunc->pOutputs =
				( DdNode ** ) malloc( pFunc->nOutputs * sizeof( int ) );
			pFunc->pOutputDcs =
				( DdNode ** ) malloc( pFunc->nOutputs * sizeof( int ) );
			pFunc->pOutputOffs =
				( DdNode ** ) malloc( pFunc->nOutputs * sizeof( int ) );
			pFunc->pOutputNames =
				( char ** ) malloc( pFunc->nOutputs * sizeof( char * ) );
			pFunc->pOutputNames[0] = NULL;
		}
		else if ( strcmp( pBuff1, ".p" ) == 0 )
		{
			pTemp = strtok( NULL, " \t\n" );
			if ( !pTemp )
			{
				fprintf( stderr,
						 "ReadPla(): Cannot read the number of product terms" );
				return 0;
			}
			nTerms = atoi( pTemp );

			if ( nTerms < 1 || nTerms > MAXLINES )
			{
				fprintf( stderr,
						 "ReadPla(): The number of prodect terms (%d) is more than MAXLINES (%d)",
						 nTerms, MAXLINES );
				return 0;
			}

			pBuff1 = strtok( NULL, " \t\n" );
			if ( pBuff1 && strlen( pBuff1 ) != 0 )
			{
				fprintf( stderr,
						 "ReadPla(): The following extra character(s) <%s> appear at the end of .p line",
						 pBuff1 );
				return 0;
			}
		}
		else if ( strcmp( pBuff1, ".type" ) == 0 )
		{
			pBuff1 = strtok( NULL, " \t\n" );

			if ( !pBuff1 || strlen( pBuff1 ) == 0 )
			{
				fprintf( stderr, "ReadPla(): Cannot read the file type" );
				return 0;
			}

			if ( strcmp( pBuff1, "fd" ) == 0 )
				pFunc->FileType = fd;
			else if ( strcmp( pBuff1, "esop" ) == 0 )
				pFunc->FileType = esop;
			else if ( strcmp( pBuff1, "fr" ) == 0 )
				pFunc->FileType = fr;
			else if ( strcmp( pBuff1, "f" ) == 0 )
				pFunc->FileType = f;
			else if ( strcmp( pBuff1, "fdr" ) == 0 )
				pFunc->FileType = fdr;
			else
			{
				fprintf( stderr,
						 "ReadPla(): The Pla file is of the unknown type <%s>",
						 pBuff1 );
				return 0;
			}

			pBuff1 = strtok( NULL, " \t\n" );
			if ( pBuff1 && strlen( pBuff1 ) != 0 )
			{
				fprintf( stderr,
						 "ReadPla(): The following extra character(s) <%s> appear at the end of .type line",
						 pBuff1 );
				return 0;
			}
		}
		else if ( strcmp( pBuff1, ".ilb" ) == 0 )
		{
			for ( i = 0; i < pFunc->nInputs; i++ )
			{
				pBuff1 = strtok( NULL, " \t\n" );
				if ( !pBuff1 )
				{
					fprintf( stderr,
							 "ReadPla(): The line .ilb has less inputs (%d) than is specified in .i line (%d)",
							 pBuff1, pFunc->nInputs );
					return 0;
				}
				pFunc->pInputNames[i] = util_strsav( pBuff1 );
			}

			pBuff1 = strtok( NULL, " \t\n" );
			if ( pBuff1 && strlen( pBuff1 ) != 0 )
			{
				fprintf( stderr,
						 "ReadPla(): The following extra character(s) <%s> appear at the end of .ilb line",
						 pBuff1 );
				return 0;
			}
		}
		else if ( strcmp( pBuff1, ".ob" ) == 0 )
		{
			for ( i = 0; i < pFunc->nOutputs; i++ )
			{
				pBuff1 = strtok( NULL, " \t\n" );
				if ( !pBuff1 )
				{
					fprintf( stderr,
							 "ReadPla(): The line .ob has less outputs (%d) than is specified in .o line (%d)",
							 i, pFunc->nOutputs );
					return 0;
				}
				pFunc->pOutputNames[i] = util_strsav( pBuff1 );
			}

			pBuff1 = strtok( NULL, " \t\n" );
			if ( pBuff1 && strlen( pBuff1 ) != 0 )
			{
				fprintf( stderr,
						 "ReadPla(): The following extra character(s) <%s> appear at the end of .ob line",
						 pBuff1 );
				return 0;
			}
		}
		else if ( strlen( pBuff1 ) == 0 || pBuff1[0] == '#' )
		{						// skip empty lines and lines containing comments
		}
		else if ( pBuff1[0] == '0' || pBuff1[0] == '1' || pBuff1[0] == '-' )
		{
			// insert space instead of the one that has been deleted
			pBuff1[strlen( pBuff1 )] = ' ';
			break;				// the only way out of the header reading loop
		}
		else
		{
			fprintf( stderr,
					 "ReadPla(): Unexpected string is encountered in the hearder: <%s>",
					 pBuff1 );
			return 0;
		}
	}

	if ( pFunc->pInputNames[0] == NULL )
	{
		for ( i = 0; i < pFunc->nInputs; i++ )
		{
			pFunc->pInputNames[i] = util_strsav( "xxxxxxx" );
			sprintf( pFunc->pInputNames[i] + 1, "%d", i );
		}

		// print out the default input variable names
//      cout << "ReadPla(): The default input variable names have been assumed" << endl;
//      for ( v = 0; v < pFunc->nInputs; v++ )
//          cout << pFunc->pInputNames[v] << " ";
//      cout << endl;
	}

	if ( pFunc->pOutputNames[0] == NULL )
	{
		for ( i = 0; i < pFunc->nOutputs; i++ )
		{
			pFunc->pOutputNames[i] = util_strsav( "yyyyyyy" );
			sprintf( pFunc->pOutputNames[i] + 1, "%d", i );
		}

		// print out the default output variable names
//      cout << "ReadPla(): The default output variable names have been assumed" << endl;
//      for ( v = 0; v < pFunc->nOutputs; v++ )
//          cout << pFunc->pOutputNames[v] << " ";
//      cout << endl;
	}

	// set the output bdds to zero
	for ( i = 0; i < pFunc->nOutputs; i++ )
	{
		pFunc->pOutputs[i] = Cudd_Not( dd->one );
		Cudd_Ref( pFunc->pOutputs[i] );
		pFunc->pOutputOffs[i] = Cudd_Not( dd->one );
		Cudd_Ref( pFunc->pOutputOffs[i] );
		pFunc->pOutputDcs[i] = Cudd_Not( dd->one );
		Cudd_Ref( pFunc->pOutputDcs[i] );
	}

	// read the body
	while ( 1 )
	{
		++LineCounter;

		// read the input cube
		pBuff1 = strtok( Str, " \t\n" );
		if ( !pBuff1 )
		{
			fprintf( stderr,
					 "ReadPla(): Cannot read the input cube in line #%d of the ESPRESSO table",
					 LineCounter );
			return 0;
		}

		// check the length of the input cube
		if ( strlen( pBuff1 ) != ( unsigned ) pFunc->nInputs )
		{
			fprintf( stderr,
					 "ReadPla(): The size of the input cube <%s> in line #%d of the Pla table does not correspond to the number of inputs specified in .i line (%d)",
					 pBuff1, LineCounter, pFunc->nInputs );
			return 0;
		}

		// read the output cube
		pBuff2 = strtok( NULL, " \t\n" );
		if ( !pBuff2 )			// it is possible that the output cube is specified in the next line
		{
			if ( !fgets( Str2, MAXLINE, Input ) )
			{
				fprintf( stderr,
						 "ReadPla(): Cannot read the output cube in line #%d of the ESPRESSO table",
						 LineCounter );
				return 0;
			}

			++LineCounter;

			pBuff2 = strtok( Str2, " \t\n" );
		}
		if ( strlen( pBuff2 ) != ( unsigned ) pFunc->nOutputs )
		{
			fprintf( stderr,
					 "ReadPla(): The size of the output cube <%s> in line #%d of the Pla table does not correspond to the number of outputs specified in .o line (%d)",
					 pBuff2, LineCounter, pFunc->nOutputs );
			return 0;
		}

		// check the remaining part of this line
		pTemp = strtok( NULL, " \t\n" );
		if ( pTemp && strlen( pTemp ) != 0 )
		{
			fprintf( stderr,
					 "ReadPla(): The following extra character(s) <%s> appear at the end of line #%d of the Pla table",
					 pTemp, LineCounter );
			return 0;
		}

		// create the bdd representing the input cube;
		// in CUDD, constant nodes are reference counted
		// while projection (elementary) variables are not!
		bInputCube = dd->one;	// this is the constant node
		Cudd_Ref( bInputCube );

		for ( i = 0; i < pFunc->nInputs; i++ )
			if ( pBuff1[i] == '0' )
			{
//              bInputCube &= !InputVars[i];
				bInputCube = Cudd_bddAnd( dd, bTemp =
										  bInputCube,
										  Cudd_Not( pFunc->pInputs[i] ) );
				Cudd_Ref( bInputCube );
				Cudd_IterDerefBdd( dd, bTemp );

				LiteralCounter++;
			}
			else if ( pBuff1[i] == '1' )
			{
//              bInputCube &=  InputVars[i];
				bInputCube = Cudd_bddAnd( dd, bTemp =
										  bInputCube, pFunc->pInputs[i] );
				Cudd_Ref( bInputCube );
				Cudd_IterDerefBdd( dd, bTemp );

				LiteralCounter++;
			}
			else if ( pBuff1[i] != '-' )
			{
				fprintf( stderr,
						 "ReadPla(): Unexpected symbol <%c> appears in the input cube in line #%d of the Pla table",
						 pBuff1[i], LineCounter );
				return 0;
			}

/*
        // this is an attempt to build the bdd for each cube in such a way
		// that, whatever is the variable order, the cube is build bottom up;
		// tests have shown that this version is at best only 5% faster...

		do 
		{   // allow for reordering to happen
			dd->reordered = 0;

			// start the cube
			bInputCube = dd->one; 
			Cudd_Ref( bInputCube );

			// derive the BDD for the cube in bottom up manner over the levels 
			for ( l = pFunc->nInputs - 1; l >= 0 ; l-- )
			{
				Index = dd->invperm[l];
				if ( pBuff1[Index] == '0' )
				{
					ThisVar = Cudd_Not( cuddUniqueInter(dd,dd->invperm[l],dd->one,Cudd_Not(dd->one)) );
					bInputCube = cuddBddAndRecur( dd, bTemp = bInputCube, ThisVar );
					if ( !bInputCube )
					{
						Cudd_IterDerefBdd( dd, bTemp );
						break;
					}
					Cudd_Ref( bInputCube );
					Cudd_Deref( bTemp );
				}
				else if ( pBuff1[Index] == '1' )
				{
					ThisVar = cuddUniqueInter(dd,dd->invperm[l],dd->one,Cudd_Not(dd->one));
					bInputCube = cuddBddAndRecur( dd, bTemp = bInputCube, ThisVar );
					if ( !bInputCube )
					{
						Cudd_IterDerefBdd( dd, bTemp );
						break;
					}
					Cudd_Ref( bInputCube );
					Cudd_Deref( bTemp );
				}
				else if ( pBuff1[Index] != '-' )
				{
					fprintf( stderr, "ReadPla(): Unexpected symbol <")+string(1,pBuff1[Index])+string("> appears in the input cube in line #%d###LineCounter)+string(" of the Pla table");
					return 0;
				}
			}

			if (dd->reordered == 1) 
				cout << endl << "ReadPla(): Reordering have happened." << endl;

		} while (dd->reordered == 1);
*/


		// add the input cube bdd to the bdds representing outputs
		if ( pFunc->FileType == fd )	// DEFAULT
		{
			// EXPRESSO MANUAL PAGE:
			//
			// With type fd (the default), for each  output,  a  1  means
			// this  product  term  belongs to the ON-set, a 0 means this
			// product term has no meaning for the value  of  this  func¡
			// tion, and a - implies this product term belongs to the DC-
			// set.
			for ( k = 0; k < pFunc->nOutputs; k++ )
				if ( pBuff2[k] == '1' )
//              pFunc->pOutputs[k] |= bInputCube;
				{
					pFunc->pOutputs[k] = Cudd_bddOr( dd, bTemp =
													 pFunc->pOutputs[k],
													 bInputCube );
					Cudd_Ref( pFunc->pOutputs[k] );
					Cudd_IterDerefBdd( dd, bTemp );
				}
				else if ( pBuff2[k] == '0' )
				{
				}
				else if ( pBuff2[k] == '-' || pBuff2[k] == '2' )
//              pFunc->pOutputDcs[k] |= bInputCube;
				{
					pFunc->pOutputDcs[k] = Cudd_bddOr( dd, bTemp =
													   pFunc->pOutputDcs[k],
													   bInputCube );
					Cudd_Ref( pFunc->pOutputDcs[k] );
					Cudd_IterDerefBdd( dd, bTemp );
				}
				else if ( pBuff2[k] == '~' )
				{
				}
				else			// some other symbol
				{
					fprintf( stderr,
							 "ReadPla(): Unexpected symbol <%c> appears in the output cube in line #%d of the Pla table",
							 pBuff2[k], LineCounter );
					return 0;
				}
		}
		else if ( pFunc->FileType == esop )
		{
			for ( k = 0; k < pFunc->nOutputs; k++ )
				if ( pBuff2[k] == '1' )
//              pFunc->pOutputs[k] ^= bInputCube;
				{
					pFunc->pOutputs[k] = Cudd_bddXor( dd, bTemp =
													  pFunc->pOutputs[k],
													  bInputCube );
					Cudd_Ref( pFunc->pOutputs[k] );
					Cudd_IterDerefBdd( dd, bTemp );
				}
				else if ( pBuff2[k] == '0' )
				{
				}
				else if ( pBuff2[k] == '-' || pBuff2[k] == '2' )
//              pFunc->pOutputDcs[k] ^= bInputCube;
				{
					pFunc->pOutputDcs[k] = Cudd_bddXor( dd, bTemp =
														pFunc->pOutputDcs[k],
														bInputCube );
					Cudd_Ref( pFunc->pOutputDcs[k] );
					Cudd_IterDerefBdd( dd, bTemp );
				}
				else if ( pBuff2[k] == '~' )
				{
				}
				else			// some other symbol
				{
					fprintf( stderr,
							 "ReadPla(): Unexpected symbol <%c> appears in the output cube in line #%d of the Pla table",
							 pBuff2[k], LineCounter );
					return 0;
				}
		}
		else if ( pFunc->FileType == fr )
		{
			// With type fr, for each output, a 1 means this product term
			// belongs to the ON-set, a 0 means this product term belongs
			// to the OFF-set, and a - means this  product  term  has  no
			// meaning for the value of this function.
			for ( k = 0; k < pFunc->nOutputs; k++ )
				if ( pBuff2[k] == '1' )
//              pFunc->pOutputs[k] |= bInputCube;
				{
					pFunc->pOutputs[k] = Cudd_bddOr( dd, bTemp =
													 pFunc->pOutputs[k],
													 bInputCube );
					Cudd_Ref( pFunc->pOutputs[k] );
					Cudd_IterDerefBdd( dd, bTemp );
				}
				else if ( pBuff2[k] == '0' )
//              pFunc->pOutputOffs[k] |= bInputCube;
				{
					pFunc->pOutputOffs[k] = Cudd_bddOr( dd, bTemp =
														pFunc->pOutputOffs[k],
														bInputCube );
					Cudd_Ref( pFunc->pOutputOffs[k] );
					Cudd_IterDerefBdd( dd, bTemp );
				}
				else if ( pBuff2[k] == '-' || pBuff2[k] == '2' )
				{
				}
				else if ( pBuff2[k] == '~' )
				{
				}
				else			// some other symbol
				{
					fprintf( stderr,
							 "ReadPla(): Unexpected symbol <%c> appears in the output cube in line #%d of the Pla table",
							 pBuff2[k], LineCounter );
					return 0;
				}
		}
		else if ( pFunc->FileType == f )
		{
			// With type f, for each output, a 1 means this product  term
			// belongs  to  the  ON-set,  and a 0 or - means this product
			// term has no meaning for the value of this function.   This
			// type corresponds to an actual PLA where only the ON-set is
			// actually implemented.
			for ( k = 0; k < pFunc->nOutputs; k++ )
				if ( pBuff2[k] == '1' )
//              pFunc->pOutputs[k] |= bInputCube;
				{
					pFunc->pOutputs[k] = Cudd_bddOr( dd, bTemp =
													 pFunc->pOutputs[k],
													 bInputCube );
					Cudd_Ref( pFunc->pOutputs[k] );
					Cudd_IterDerefBdd( dd, bTemp );
				}
				else if ( pBuff2[k] == '0' )
				{
				}
				else if ( pBuff2[k] == '-' || pBuff2[k] == '2' )
				{
				}
				else if ( pBuff2[k] == '~' )
				{
				}
				else			// some other symbol
				{
					fprintf( stderr,
							 "ReadPla(): Unexpected symbol <%c> appears in the output cube in line #%d of the Pla table",
							 pBuff2[k], LineCounter );
					return 0;
				}
		}
		else if ( pFunc->FileType == fdr )
		{
			// With  type  fdr,  for  each output, a 1 means this product
			// term belongs to the ON-set, a 0 means  this  product  term
			// belongs  to  the  OFF-set,  a  -  means  this product term
			// belongs to the DC-set, and a ~ implies this  product  term
			// has no meaning for the value of this function.
			for ( k = 0; k < pFunc->nOutputs; k++ )
				if ( pBuff2[k] == '1' )
//              pFunc->pOutputs[k] |= bInputCube;
				{
					pFunc->pOutputs[k] = Cudd_bddOr( dd, bTemp =
													 pFunc->pOutputs[k],
													 bInputCube );
					Cudd_Ref( pFunc->pOutputs[k] );
					Cudd_IterDerefBdd( dd, bTemp );
				}
				else if ( pBuff2[k] == '0' )
//              pFunc->pOutputOffs[k] |= bInputCube;
				{
					pFunc->pOutputOffs[k] = Cudd_bddOr( dd, bTemp =
														pFunc->pOutputOffs[k],
														bInputCube );
					Cudd_Ref( pFunc->pOutputOffs[k] );
					Cudd_IterDerefBdd( dd, bTemp );
				}
				else if ( pBuff2[k] == '-' || pBuff2[k] == '2' )
//              pFunc->pOutputDcs[k] |= bInputCube;
				{
					pFunc->pOutputDcs[k] = Cudd_bddOr( dd, bTemp =
													   pFunc->pOutputDcs[k],
													   bInputCube );
					Cudd_Ref( pFunc->pOutputDcs[k] );
					Cudd_IterDerefBdd( dd, bTemp );
				}
				else if ( pBuff2[k] == '~' )
				{
				}
				else			// some other symbol
				{
					fprintf( stderr,
							 "ReadPla(): Unexpected symbol <%c> appears in the output cube in line #%d of the Pla table",
							 pBuff2[k], LineCounter );
					return 0;
				}
		}

		// the input cube should be dereferenced only once,
		// after all the outputs have been considered
		Cudd_IterDerefBdd( dd, bInputCube );

		// scan the next line from the input stream (skip the empty and comment lines)
		while ( ( pTemp = fgets( Str, MAXLINE, Input ) )
				&& ( Str[0] == '\r' || Str[0] == '\n' || Str[0] == '#' ) );

		if ( !pTemp || strlen( Str ) == 0 )
			//fprintf( stderr, "ReadPla(): The cube table in the ESPRESSO file ends abruptly (cannot read the next line from the input stream)");
			break;

		// check for the last line of the table
		if ( Str[0] == '.' && Str[1] == 'e' )
			break;
	}

	if ( nTerms > 0
		 && ( nTerms != LineCounter && nTerms != LineCounter / 2 ) )
	{
		fprintf( stderr,
				 "ReadPla(): The Pla table has different number of lines (%d) compared to what is specified in .p line (%d)",
				 LineCounter, nTerms );
		return 0;
	}

	// write down the number of cubes and literals in the PLA
//  pFunc->nCubes = LineCounter; // this is WRONG if each line is written as two lines!!!
//  pFunc->nLiterals = LiteralCounter;

	// derive other sets from the known
	if ( pFunc->FileType == fd )
		for ( k = 0; k < pFunc->nOutputs; k++ )
		{
//          pFunc->pOutputs[k] -= pFunc->pOutputDcs[k];
//          pFunc->pOutputOffs[k] = !(pFunc->pOutputs[k] | pFunc->pOutputDcs[k]);

			pFunc->pOutputs[k] = Cudd_bddAnd( dd, bTemp =
											  pFunc->pOutputs[k],
											  Cudd_Not( pFunc->
														pOutputDcs[k] ) );
			Cudd_Ref( pFunc->pOutputs[k] );
			Cudd_IterDerefBdd( dd, bTemp );

			Cudd_IterDerefBdd( dd, pFunc->pOutputOffs[k] );
			pFunc->pOutputOffs[k] =
				Cudd_Not( Cudd_bddOr
						  ( dd, pFunc->pOutputs[k], pFunc->pOutputDcs[k] ) );
			Cudd_Ref( pFunc->pOutputOffs[k] );
		}
	else if ( pFunc->FileType == esop )
		for ( k = 0; k < pFunc->nOutputs; k++ )
		{
//          pFunc->pOutputOffs[k] = !pFunc->pOutputs[k];
			Cudd_IterDerefBdd( dd, pFunc->pOutputOffs[k] );
			pFunc->pOutputOffs[k] = Cudd_Not( pFunc->pOutputs[k] );
			Cudd_Ref( pFunc->pOutputOffs[k] );
		}
	else if ( pFunc->FileType == fr )
		for ( k = 0; k < pFunc->nOutputs; k++ )
		{
//          if ( (pFunc->pOutputs[k] & pFunc->pOutputOffs[k]) != bddfalse )
			if ( Cudd_bddIteConstant
				 ( dd, pFunc->pOutputs[k], pFunc->pOutputOffs[k], b0 ) != b0 )
			{
				fprintf( stderr,
						 "ReadPla(): The ON-set and OFF-set have non-empty overlap!" );
				return 0;
			}

//          pFunc->pOutputDcs[k] = !(pFunc->pOutputs[k] | pFunc->pOutputOffs[k]);
			Cudd_IterDerefBdd( dd, pFunc->pOutputDcs[k] );
			pFunc->pOutputDcs[k] =
				Cudd_Not( Cudd_bddOr
						  ( dd, pFunc->pOutputs[k], pFunc->pOutputOffs[k] ) );
			Cudd_Ref( pFunc->pOutputDcs[k] );
		}
	else if ( pFunc->FileType == f )
		for ( k = 0; k < pFunc->nOutputs; k++ )
		{
//          pFunc->pOutputOffs[k] = !pFunc->pOutputs[k];
			Cudd_IterDerefBdd( dd, pFunc->pOutputOffs[k] );
			pFunc->pOutputOffs[k] = Cudd_Not( pFunc->pOutputs[k] );
			Cudd_Ref( pFunc->pOutputOffs[k] );
		}
	else if ( pFunc->FileType == fdr )
		for ( k = 0; k < pFunc->nOutputs; k++ )
		{
//          if ( (pFunc->pOutputs[k] & pFunc->pOutputOffs[k] ) != bddfalse )
			if ( Cudd_bddIteConstant
				 ( dd, pFunc->pOutputs[k], pFunc->pOutputOffs[k], b0 ) != b0 )
			{
				fprintf( stderr,
						 "ReadPla(): The ON-set and OFF-set have non-empty overlap!" );
				return 0;
			}
//          if ( (pFunc->pOutputs[k] & pFunc->pOutputDcs[k] ) != bddfalse )
			if ( Cudd_bddIteConstant
				 ( dd, pFunc->pOutputs[k], pFunc->pOutputDcs[k], b0 ) != b0 )
			{
				fprintf( stderr,
						 "ReadPla(): The ON-set and DC-set have non-empty overlap!" );
				return 0;
			}
//          if ( (pFunc->pOutputDcs[k] & pFunc->pOutputOffs[k] ) != bddfalse )
			if ( Cudd_bddIteConstant
				 ( dd, pFunc->pOutputDcs[k], pFunc->pOutputOffs[k],
				   b0 ) != b0 )
			{
				fprintf( stderr,
						 "ReadPla(): The DC-set and OFF-set have non-empty overlap!" );
				return 0;
			}
//          if ( (pFunc->pOutputs[k] | pFunc->pOutputOffs[k] | pFunc->pOutputDcs[k]) != bddtrue )
//          {
//              fprintf( stderr, "ReadPla(): The sum of ON-set, OFF-set, and DC-set do not cover the whole domain!");
//              return 0;
//          }
		}

	// close the file
//  Input.close();
	fclose( Input );
	return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/

int
ReadBlif( BFunc * pFunc )
// This function opens the input file <pFunc->FileInput> and 
// the variable order file <pFunc->FileVarOrder> (if specified), 
// reads the contents of input file using the BFunc reader, 
// derives BDDs for the primary outputs and returns them 
// in pFunc->pOutputs together with other data about the network
// The function returns 1 on success, 0 on failure
{
	FILE *pFileInput;
	int result, i;
	BnetNode *node;
	BnetNetwork *net;
	DdNode *Temp;

	///////////////////////////////////////////////////////////////
	// the low-level pointer to the BDD manager
	DdManager *dd = pFunc->dd;
	///////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////
	// define reader parameters
	int fUseOriginalOrder = 1;
	// if (fUseOriginalOrder )
	// there are two possibilities
	// (1) take the order from FileVarOrder file
	// (2) take the order from the BLIF structure
	int fUseFileVarOrder = 1;	// set to 0 to have order from BLIF structure
	// other parameters
	int locGlob = BNET_GLOBAL_DD;	// otherwise: BNET_LOCAL_DD
//  int locGlob = BNET_LOCAL_DD; // otherwise: BNET_GLOBAL_DD
	int noDrop = 1;				// set to 0 to have intermediate functions dropped
	int stateOnly = 0;			// set to 1 to derive only next state functions
	int Progress = 0;			// set to 1 to see how the reader proceeds
	int Verbosity = 2;			// other verbosity levels are possible
	///////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////
	// set the pointers to zero
	pFunc->pInputs = NULL;
	pFunc->pInputNames = NULL;
	pFunc->pOutputs = NULL;
	pFunc->pOutputNames = NULL;
	pFunc->pOutputOffs = NULL;
	pFunc->pOutputDcs = NULL;
	///////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////
	// open the input file
	pFileInput = fopen( pFunc->FileInput, "r" );
	if ( pFileInput == NULL )
		return 0;
	///////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////
	// restart the variable counter
	Bnet_ResetCounterOfBddVariables(  );
	// call the reader  
	net = Bnet_ReadNetwork( pFileInput, Verbosity );
	// close the file
	fclose( pFileInput );
	if ( net == NULL )
		return 0;
	//... and optionally echo it to the standard output
//  Bnet_PrintNetwork(net);
	///////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////
	// Build the BDD from the internal network data structure
	///////////////////////////////////////////////////////////////
	// First assign variables to inputs if the order is provided.
	// (Either in the .blif file or in an order file.)
	if ( fUseOriginalOrder )
	{
		// Follow order given in the input file. First primary inputs
		// and then present state variables.
		for ( i = 0; i < net->npis; i++ )
		{
			if ( !st_lookup( net->hash, net->inputs[i], ( char ** ) &node ) )
				return ( 0 );
			result =
				Bnet_BuildNodeBDD( dd, node, net->hash, locGlob, noDrop );
			if ( result == 0 )
				return ( 0 );
		}
		for ( i = 0; i < net->nlatches; i++ )
		{
			if ( !st_lookup
				 ( net->hash, net->latches[i][1], ( char ** ) &node ) )
				return ( 0 );
			result =
				Bnet_BuildNodeBDD( dd, node, net->hash, locGlob, noDrop );
			if ( result == 0 )
				return ( 0 );
		}
	}
	else if ( !fUseOriginalOrder && fUseFileVarOrder )
	{
//      result = Bnet_ReadOrder(dd,(char*)pFunc->FileVarOrder,net,locGlob,noDrop);
//      if (result == 0) 
		return ( 0 );
	}
	else
	{
		result = Bnet_DfsVariableOrder( dd, net );
		if ( result == 0 )
			return ( 0 );
	}

	// At this point the BDDs of all primary inputs and present state
	// variables have been built. 

	if ( locGlob == BNET_LOCAL_DD )
	{
		node = net->nodes;
		while ( node != NULL )
		{
			result =
				Bnet_BuildNodeBDD( dd, node, net->hash, BNET_LOCAL_DD, 1 );
			if ( result == 0 )
				return ( 0 );
			if ( Verbosity > 2 )
			{
				fprintf( stdout, "%s", node->name );
				Cudd_PrintDebug( dd, node->dd, Cudd_ReadSize( dd ),
								 Verbosity );
			}

			/////////////////////////////////////////////////
			// debug print out of the internal node BDDs
			/////////////////////////////////////////////////
//          cout << "The node name is <" << node->name << ">";
//          cout << "  The dd var number is " << node->var << endl;
//          cout << ReadBdd( dd,node->dd ) << endl;
			/////////////////////////////////////////////////

			node = node->next;
		}
	}
	else						// if (locGlob == BNET_GLOBAL_DD)
	{
		// Create BDDs with DFS from the primary outputs and the next
		// state functions. If the inputs had not been ordered yet,
		// this would result in a DFS order for the variables.
		ntrInitializeCount( net, stateOnly );

		if ( stateOnly == 0 )
			for ( i = 0; i < net->npos; i++ )
			{
				if ( !st_lookup
					 ( net->hash, net->outputs[i], ( char ** ) &node ) )
					continue;
				result =
					Bnet_BuildNodeBDD( dd, node, net->hash, BNET_GLOBAL_DD,
									   noDrop );
				if ( result == 0 )
					return ( 0 );
				if ( Progress )
					fprintf( stdout, "%s\n", node->name );

				if ( node->exdc )
				{
					result =
						Bnet_BuildNodeBDD( dd, node->exdc, net->hash,
										   BNET_GLOBAL_DD, noDrop );
					if ( result == 0 )
						return ( 0 );
					if ( Progress )
						fprintf( stdout, "%s - exdc\n", node->name );
				}
				//Cudd_PrintDebug(dd,node->dd,net->ninputs,option->verb);
			}

		for ( i = 0; i < net->nlatches; i++ )
		{
			if ( !st_lookup
				 ( net->hash, net->latches[i][0], ( char ** ) &node ) )
				continue;
			result =
				Bnet_BuildNodeBDD( dd, node, net->hash, BNET_GLOBAL_DD,
								   noDrop );
			if ( result == 0 )
				return ( 0 );
			if ( Progress )
				fprintf( stdout, "%s\n", node->name );
			//Cudd_PrintDebug(dd,node->dd,net->ninputs,option->verb);
		}

		// Make sure all inputs have a DD and dereference the DDs of
		// the nodes that are not reachable from the outputs.
		for ( i = 0; i < net->npis; i++ )
		{
			if ( !st_lookup( net->hash, net->inputs[i], ( char ** ) &node ) )
				return ( 0 );

			result =
				Bnet_BuildNodeBDD( dd, node, net->hash, BNET_GLOBAL_DD,
								   noDrop );
			if ( result == 0 )
				return ( 0 );
			if ( node->count == -1 )
				Cudd_RecursiveDeref( dd, node->dd );
		}
		for ( i = 0; i < net->nlatches; i++ )
		{
			if ( !st_lookup
				 ( net->hash, net->latches[i][1], ( char ** ) &node ) )
				return ( 0 );

			result =
				Bnet_BuildNodeBDD( dd, node, net->hash, BNET_GLOBAL_DD,
								   noDrop );
			if ( result == 0 )
				return ( 0 );
			if ( node->count == -1 )
				Cudd_RecursiveDeref( dd, node->dd );
		}

		// Dispose of the BDDs of the internal nodes 
		// if they have not been dropped already.
		if ( noDrop )
			for ( node = net->nodes; node != NULL; node = node->next )
				if ( node->dd != NULL && node->count != -1 &&
					 ( node->type == BNET_INTERNAL_NODE ||
					   node->type == BNET_INPUT_NODE ||
					   node->type == BNET_CONSTANT_NODE ||
					   node->type == BNET_PRESENT_STATE_NODE ) )
				{
					/////////////////////////////////////////////////
					// debug print out of the internal node BDDs
					/////////////////////////////////////////////////
//              cout << "The node name is <" << node->name << ">";
//              cout << "  The dd var number is " << node->var << endl;
//              cout << ReadBdd( dd,node->dd ) << endl;
					/////////////////////////////////////////////////

					Cudd_RecursiveDeref( dd, node->dd );
					if ( node->type == BNET_INTERNAL_NODE )
						node->dd = NULL;
				}
	}

	// write the minimum amount of information about the network
	if ( net->name == NULL )
		net->name = util_strsav( pFunc->FileGeneric );
	pFunc->Model = util_strsav( net->name );
	pFunc->nInputs = net->npis;
	pFunc->nOutputs = net->npos;
	pFunc->nLatches = net->nlatches;
	pFunc->FileType = blif;

	pFunc->pInputs = ( DdNode ** ) malloc( pFunc->nInputs * sizeof( int ) );
	pFunc->pInputNames =
		( char ** ) malloc( pFunc->nInputs * sizeof( char * ) );

	pFunc->pOutputs = ( DdNode ** ) malloc( pFunc->nOutputs * sizeof( int ) );
	pFunc->pOutputDcs =
		( DdNode ** ) malloc( pFunc->nOutputs * sizeof( int ) );
	pFunc->pOutputOffs =
		( DdNode ** ) malloc( pFunc->nOutputs * sizeof( int ) );
	pFunc->pOutputNames =
		( char ** ) malloc( pFunc->nOutputs * sizeof( char * ) );

	for ( i = 0; i < net->npis; i++ )
	{
		if ( !st_lookup( net->hash, net->inputs[i], ( char ** ) &node ) )
		{
			assert( 0 );
		}
		// assign the inputs
		pFunc->pInputs[node->var] = node->dd;
		pFunc->pInputNames[node->var] = util_strsav( net->inputs[i] );
	}

	for ( i = 0; i < net->npos; i++ )
	{
		if ( !st_lookup( net->hash, net->outputs[i], ( char ** ) &node ) )
		{
			assert( 0 );
		}

		// assign the on-set
		pFunc->pOutputs[i] = node->dd;

		// assign the dc-set
		if ( node->exdc )
		{
			pFunc->pOutputDcs[i] = node->exdc->dd;

			// if exdcs are specified, we should sharp the on-set using the dc-set
			pFunc->pOutputs[i] = Cudd_bddAnd( dd, Temp =
											  pFunc->pOutputs[i],
											  Cudd_Not( pFunc->
														pOutputDcs[i] ) );
			Cudd_Ref( pFunc->pOutputs[i] );
			Cudd_RecursiveDeref( dd, Temp );
		}
		else
		{
			pFunc->pOutputDcs[i] = Cudd_Not( dd->one );
			Cudd_Ref( pFunc->pOutputDcs[i] );
		}

		// assign the off-set
		pFunc->pOutputOffs[i] =
			Cudd_bddOr( dd, pFunc->pOutputs[i], pFunc->pOutputDcs[i] );
		Cudd_Ref( pFunc->pOutputOffs[i] );
		pFunc->pOutputOffs[i] = Cudd_Not( pFunc->pOutputOffs[i] );

		pFunc->pOutputNames[i] = util_strsav( net->outputs[i] );

		// no need to dereference because Net inherits the references from net
	}

	// delete the network object
	Bnet_FreeNetwork( net );

	return 1;
}

//////////////////////////////////////////////////////////////////
///////         Functions Borrowed from NANOTRAV             /////
//////////////////////////////////////////////////////////////////

/**Function********************************************************************

  Synopsis    [Initializes the count fields used to drop DDs.]

  Description [Initializes the count fields used to drop DDs.
  Before actually building the BDDs, we perform a DFS from the outputs
  to initialize the count fields of the nodes.  The initial value of the
  count field will normally coincide with the fanout of the node.
  However, if there are nodes with no path to any primary output or next
  state variable, then the initial value of count for some nodes will be
  less than the fanout. For primary outputs and next state functions we
  add 1, so that we will never try to free their DDs. The count fields
  of the nodes that are not reachable from the outputs are set to -1.]

  SideEffects [Changes the count fields of the network nodes. Uses the
  visited fields.]

  SeeAlso     []

******************************************************************************/
static void
ntrInitializeCount( BnetNetwork * net, int stateOnly )
{
	BnetNode *node;
	int i;

	if ( stateOnly == 0 )
		for ( i = 0; i < net->npos; i++ )
		{
			if ( !st_lookup( net->hash, net->outputs[i], ( char ** ) &node ) )
			{
				fprintf( stdout, "Warning: output %s is not driven!\n",
						 net->outputs[i] );
				continue;
			}
			ntrCountDFS( net, node );
			node->count++;
		}

	for ( i = 0; i < net->nlatches; i++ )
	{
		if ( !st_lookup( net->hash, net->latches[i][0], ( char ** ) &node ) )
		{
			fprintf( stdout, "Warning: latch input %s is not driven!\n",
					 net->outputs[i] );
			continue;
		}
		ntrCountDFS( net, node );
		node->count++;
	}

	/* Clear visited flags. */
	node = net->nodes;
	while ( node != NULL )
	{
		if ( node->visited == 0 )
			node->count = -1;
		else
			node->visited = 0;
		node = node->next;
	}

}								/* end of ntrInitializeCount */


/**Function********************************************************************

  Synopsis    [Does a DFS from a node setting the count field.]

  Description []

  SideEffects [Changes the count and visited fields of the nodes it visits.]

  SeeAlso     [ntrLevelDFS]

******************************************************************************/
static void
ntrCountDFS( BnetNetwork * net, BnetNode * node )
{
	int i;
	BnetNode *auxnd;

	node->count++;

	if ( node->visited == 1 )
		return;

	node->visited = 1;

	for ( i = 0; i < node->ninp; i++ )
	{
		if ( !st_lookup
			 ( net->hash, ( char * ) node->inputs[i], ( char ** ) &auxnd ) )
			exit( 2 );
		ntrCountDFS( net, auxnd );
	}

}								/* end of ntrCountDFS */
