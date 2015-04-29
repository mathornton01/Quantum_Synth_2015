/**CFile***********************************************************************

  FileName    [zGraphIO.c]

  PackageName [extra]

  Synopsis    [Experimental version of some ZDD operators.]

  Description [External procedures included in this module:
				<ul>
				<li> Extra_zddGraphRead()
				<li> Extra_zddGraphWrite()
				<li> Extra_zddGraphDumpDot()
				</ul>
			Internal procedures included in this module:
				<ul>
				</ul>
			Static procedures included in this module:
				<ul>
				<li> zddGraphEnumeratePaths_rec()
				<li> PrintFn()
				<li> DumpDotFn()
				<li> zddGraphSetEdge()
				<li> zddGraphSetEdge_aux()
				<li> read_graph_DIMACS_ascii()
				<li> read_graph_DIMACS_bin()
				<li> get_params()
				<li> get_edge()
				</ul>
	      ]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$zGraphIO.c, v.1.2, Nov 24-25, 2000, alanmi $]

******************************************************************************/

#include "util.h"
#include "cuddInt.h"
#include "extra.h"


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/* this code is borrowed from the DIMACS graph reader publicly available at 
 * ftp://dimacs.rutgers.edu/pub/challenge/graph/translators/


/* If you change MAX_NR_VERTICES, change MAX_NR_VERTICESdiv8 to be 
the 1/8th of it */

#define MAX_NR_VERTICES		5000
#define MAX_NR_VERTICESdiv8	625

#define BOOL	char

int Nr_vert = 0, Nr_edges = 0;
BOOL Bitmap[MAX_NR_VERTICES][MAX_NR_VERTICESdiv8];

char masks[ 8 ] = { (char)0x01, (char)0x02, (char)0x04, (char)0x08, 
                    (char)0x10, (char)0x20, (char)0x40, (char)0x80 };

#define MAX_PREAMBLE 10000
static char Preamble[MAX_PREAMBLE];
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/* the structure passed during traversal of the graph represented as a ZDD */
typedef struct
{
	FILE  *fp;                /* the file to output graph edges */
	int    Lev[2];            /* levels of vars in the combination */
	int   *Map;               /* mapping from levels to graph vertices */
} userdata;

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

static DdManager * s_ddman; /* the pointer to the current DD manager */
static DdNode    * s_Graph; /* graph under construction */

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static int zddGraphEnumeratePaths_rec( DdManager *dd, DdNode *S, int(*PrFn)(userdata*), userdata* Data );
static int PrintFn( userdata* p );
static int DumpDotFn( userdata* p );
static void zddGraphSetEdge( register int i, register int j );
static int zddGraphSetEdge_aux( register int i, register int j );

static void read_graph_DIMACS_ascii( FILE* fp );
static void read_graph_DIMACS_bin( FILE* fp );
static int get_params(void);
static BOOL get_edge(  int i,  int j );

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis [Reads the graph specified in DIMACS format and represents it as a ZDD.]

  Description [Reads the graph specified in DIMACS ASCII format and represents it as
         a ZDD using one variable for each nodes of the graph. The graph is stored
		 as a set of pairs of variables, which correspond to all nodes connected
		 by an edge.]

  SideEffects [Increases the number of variables]

  SeeAlso     [Extra_zddGraphWrite, Extra_zddGraphDumpDot]

******************************************************************************/
DdNode *
Extra_zddGraphRead(
  char* FileDimacs,  /* name of the file to read */
  DdManager * dd,    /* the dd manager in which the graph is constructed */
  int * nNodes,      /* upon exit, contains the number of nodes */
  int * nEdges)      /* upon exit, contains the number of edges */
{
    
	FILE* fp;
	int c, fBinary;

	/* open the file */
	if ( FileDimacs == NULL || (fp=fopen(FileDimacs,"r"))==NULL )
		return NULL;

	/* figure out what is the type of the input file: ASCII or binary 
	 * ASCII files have the first line starting with character "c", 
	 * binary files have the first line starting with a number 
	 */
	if ( (c = fgetc(fp)) == EOF )
		return NULL;
	if ( c == 'c' || c == 'p' )
		fBinary = 0;
	else if ( c >= '0' && c <= '9' )
		fBinary = 1;
	else 
	{
		printf("\nExtra_zddGraphRead(): Unknown file type!\n");
		return NULL;
	}

	ungetc(c, fp); 

	/* start the static variables */
	s_ddman = dd;
	s_Graph = dd->zero;
	cuddRef( s_Graph );

	/* read the file */ 
	if ( fBinary )
		read_graph_DIMACS_bin( fp );
	else
		read_graph_DIMACS_ascii( fp );
	fclose( fp );

	/* return the result */
	cuddDeref( s_Graph );
	*nNodes = Nr_vert;
	*nEdges = Nr_edges;
    return s_Graph;

} /* end of Extra_zddGraphRead */


/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
int
Extra_zddGraphWrite(
  char* FileDimacs,
  char* Preamble,
  DdManager * dd,
  DdNode * Graph)
{
	DdNode *Supp, *SuppCopy;
	int i, size = 0;
	int *MapLevel2Vertex;
	userdata Data;

	/* open the output file */
    FILE* pFileOutput = fopen( FileDimacs, "w" );
	if ( pFileOutput == NULL )
		return 0;

	/* compute the support of the graph */
	Supp = Extra_zddSupport( dd, Graph );
	if ( Supp == NULL )
		return 0;
	Cudd_Ref( Supp );
	SuppCopy = Supp;

	/* allocate memory for mapping */
	MapLevel2Vertex = ALLOC( int, dd->sizeZ );
	if (MapLevel2Vertex == NULL) 
	{
		dd->errorCode = CUDD_MEMORY_OUT;
		return 0;
	}
	for( i = 0; i < dd->sizeZ; i++ )
		MapLevel2Vertex[ i ] = -12;

	/* find the mapping of var levels to vertex numbers */
	for( size = 0; Supp != dd->one; size++, Supp = cuddT( Supp ) )
		MapLevel2Vertex[ dd->permZ[ Supp->index ] ] = size;

	Cudd_RecursiveDerefZdd( dd, SuppCopy );

	/* start writing file */
	if ( Preamble )
		fprintf( pFileOutput, "%s", Preamble );

	fprintf( pFileOutput, "p edge %d %d\n", size, Cudd_zddCount( dd, Graph ) );

	/* call the graph traversal function which prints edges */
	Data.fp = pFileOutput;       /* the file to output edges */
	Data.Lev[0] = -1;            /* the level of the first var */
	Data.Lev[1] = -1;            /* the level of the second var */
	Data.Map = MapLevel2Vertex;  /* mapping from levels to graph vertices */

	if ( zddGraphEnumeratePaths_rec( dd, Graph, PrintFn, &Data ) == 0 )
	{
	    fclose( pFileOutput );
		return 0;
	}

	/* close the file*/
    fclose( pFileOutput );

	/* clean up */
	FREE( MapLevel2Vertex );
	Data.Map = NULL;

	return 1;
} /* Extra_zddGraphWrite */

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

int Extra_zddGraphDumpDot( 
  char * FileDot,
  DdManager * dd,
  DdNode * Graph)
{
	DdNode *Supp, *SuppCopy;
	int i, size = 0;
	int *MapLevel2Vertex;
	userdata Data;

	/* open the output file */
    FILE* pFileDot = fopen( FileDot, "w" );
	if ( pFileDot == NULL )
		return 0;

	/* compute the support of the graph */
	Supp = Extra_zddSupport( dd, Graph );
	if ( Supp == NULL )
		return 0;
	Cudd_Ref( Supp );
	SuppCopy = Supp;

	/* allocate memory for mapping */
	MapLevel2Vertex = ALLOC( int, dd->sizeZ );
	if (MapLevel2Vertex == NULL) 
	{
		dd->errorCode = CUDD_MEMORY_OUT;
		return 0;
	}
	for( i = 0; i < dd->sizeZ; i++ )
		MapLevel2Vertex[ i ] = -12;

	/* find the mapping of var levels to vertex numbers */
	for( size = 0; Supp != dd->one; size++, Supp = cuddT( Supp ) )
		MapLevel2Vertex[ dd->permZ[ Supp->index ] ] = size;

	Cudd_RecursiveDerefZdd( dd, SuppCopy );

	/* start dumping dot */
	fprintf( pFileDot, "graph G {\n" );
	fprintf( pFileDot, "    size = \"7.5,10\";\n" );
	fprintf( pFileDot, "    center = true;\n" );
	fprintf( pFileDot, "    node [shape=circle, style=filled];\n" );
	fprintf( pFileDot, "    edge [len=3];\n" ); 
	fprintf( pFileDot, "\n" ); 

	/* call the graph traversal function which prints edges */
	Data.fp = pFileDot;          /* the file to output edges */
	Data.Lev[0] = -1;            /* the level of the first var */
	Data.Lev[1] = -1;            /* the level of the second var */
	Data.Map = MapLevel2Vertex;  /* mapping from levels to graph vertices */

	if ( zddGraphEnumeratePaths_rec( dd, Graph, DumpDotFn, &Data ) == 0 )
	{
	    fclose( pFileDot );
		return 0;
	}

	/* stop dumping dot */
	fprintf( pFileDot, "}\n" );
    fclose( pFileDot );

	/* clean up */
	FREE( MapLevel2Vertex );
	Data.Map = NULL;

	return 1;
} /* Extra_zddGraphDumpDot */


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Enumerates edges of the graph represented as a ZDD.]

  Description [Enumerates edges of the graph represented as a ZDD and
               calls the function PrFn( Data ) for every path found.
			   Call to function PrFn() should not cause variable reordering.
			   This function returns 1, if printing went successfully;
			   otherwise it returns 0. The reason of failure may be that the
			   graph contains a combination with more or less than two elements.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
int zddGraphEnumeratePaths_rec( 
  DdManager *dd,           /* the manager */
  DdNode *G,               /* the graph */
  int(*PrFn)(userdata*),   /* the printing function */
  userdata* Data )         /* user-specific data */
{
	int LastNum;

	if ( G == dd->zero )
		return 1;
	if ( G == dd->one )
		return PrFn(Data);
		
	/* the given var has positive polarity */
	/* fill in this variable */
	if ( Data->Lev[0] == -1 )
		LastNum = 0;
	else if ( Data->Lev[1] == -1 )
		LastNum = 1;
	else
		return 0;
	Data->Lev[ LastNum ] = dd->permZ[ G->index ];

	if ( zddGraphEnumeratePaths_rec( dd, cuddT(G), PrFn, Data ) == 0 )
		return 0;

	/* clear this variable */
	Data->Lev[ LastNum ] = -1;

	/* the given var has negative polarity */
	/* no need to fill in this variable */
	if ( zddGraphEnumeratePaths_rec( dd, cuddE(G), PrFn, Data ) == 0 )
		return 0;
	return 1;
}


/**Function********************************************************************

  Synopsis    [Simple edge to print edge into DIMACS file.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
int PrintFn( userdata* p )
{
	if ( p->Lev[0] == -1 || p->Lev[1] == -1 )
		return 0;
	if ( p->Map[ p->Lev[0] ] < 0 || p->Map[ p->Lev[1] ] < 0 )
		return 0;

	fprintf( p->fp, "e %d %d\n", p->Map[ p->Lev[0] ] + 1,
		                         p->Map[ p->Lev[1] ] + 1 );
	return 1;
}


/**Function********************************************************************

  Synopsis    [Simple edge to print edge into DOT file.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
int DumpDotFn( userdata* p )
{
	if ( p->Lev[0] == -1 || p->Lev[1] == -1 )
		return 0;
	if ( p->Map[ p->Lev[0] ] < 0 || p->Map[ p->Lev[1] ] < 0 )
		return 0;

	fprintf( p->fp, "    %d -- %d;\n", p->Map[ p->Lev[0] ] + 1,
		                               p->Map[ p->Lev[1] ] + 1 );
	return 1;
}

/**Function********************************************************************

  Synopsis    [Builds the combination of two nodes to be added to the graph.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void zddGraphSetEdge( 
  register int i,    /* the first node */
  register int j)    /* the second node */
{
    int	autoDynZ = s_ddman->autoDynZ;
    s_ddman->autoDynZ = 0;
    do 
	{
		s_ddman->reordered = 0;
		(int) zddGraphSetEdge_aux(i, j);
    } 
	while (s_ddman->reordered == 1);
    s_ddman->autoDynZ = autoDynZ;

} /* end of zddGraphSetEdge */


/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/

int zddGraphSetEdge_aux( register int i, register int j )
{
	DdNode *zTemp, *zComb;
	int VarLow, VarHigh;

	/* find the levels of variables i and j */
	int LevelI = (i >= s_ddman->sizeZ) ? i : s_ddman->permZ[i];
	int LevelJ = (j >= s_ddman->sizeZ) ? j : s_ddman->permZ[j];

	/* find which variable is higher/lower in the order */
	if ( LevelI < LevelJ )
	{
		VarHigh = i;
		VarLow = j;
	}
	else
	{
		VarHigh = j;
		VarLow = i;
	}

	/* first, create the node for the lower variable */
	zComb = cuddZddGetNode( s_ddman, VarLow, s_ddman->one, s_ddman->zero );
	if ( zComb == NULL )
		return 0;
	cuddRef( zComb );

	/* next, create the node for the higher variable */
	zComb = cuddZddGetNode( s_ddman, VarHigh, zTemp = zComb, s_ddman->zero );
	if ( zComb == NULL )
	{
		Cudd_RecursiveDerefZdd( s_ddman, zTemp );
		return 0;
	}
	cuddRef( zComb );
	cuddDeref( zTemp );

	/* add to the graph under construction */
	s_Graph = cuddZddUnion( s_ddman, zTemp = s_Graph, zComb );
	if ( s_Graph == NULL )
	{
		s_Graph = zTemp;
		Cudd_RecursiveDerefZdd( s_ddman, zComb );
		return 0;
	}
	cuddRef( s_Graph );
	Cudd_RecursiveDerefZdd( s_ddman, zTemp );
	Cudd_RecursiveDerefZdd( s_ddman, zComb );
	return 1;

} /* zddGraphSetEdge_aux */


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/* This code is borrowed from the DIMACS graph reader publicly available at 
 * ftp://dimacs.rutgers.edu/pub/challenge/graph/translators/
 *                                                 -alanmi, Nov 25, 2000
 */

int get_params(void)
/* getting Nr_vert and Nr_edge from the preamble string, 
   containing Dimacs format "p ??? num num" */
{
	char c, *tmp;
	char * pp = Preamble;
	int stop = 0;
	tmp = (char *)calloc(100, sizeof(char));
	
	Nr_vert = Nr_edges = 0;
	
	while (!stop && (c = *pp++) != '\0'){
		switch (c)
		  {
			case 'c':
			  while ((c = *pp++) != '\n' && c != '\0');
			  break;
			  
			case 'p':
			  sscanf(pp, "%s %d %d\n", tmp, &Nr_vert, &Nr_edges);
			  stop = 1;
			  break;
			  
			default:
			  break;
		  }
	}
	
	free(tmp);
	
	if (Nr_vert == 0 || Nr_edges == 0)
	  return 0;  /* error */
	else
	  return 1;
	
}

void read_graph_DIMACS_ascii( FILE* fp )
{
	int c, oc;
	char * pp = Preamble;
	int i,j;

	for(oc = '\0' ;(c = fgetc(fp)) != EOF && (oc != '\n' || c != 'e')
		; oc = *pp++ = c);
 
	ungetc(c, fp); 
	*pp = '\0';
	get_params();

	while ((c = fgetc(fp)) != EOF){
		switch (c)
		  {
			case 'e':
			  if (!fscanf(fp, "%d %d", &i, &j))
				{ printf("ERROR: corrupted inputfile\n"); exit(10); }
			  
			  if ( i == 0 || j == 0 )
  				{ printf("ERROR: n >= 1, but input file has node number 0\n"); exit(10); }
			  if (i > j) 
				zddGraphSetEdge(i-1, j-1);
			  else
				zddGraphSetEdge(j-1, i-1);
			  break;
			  
			case '\n':
			  
			default:
			  break;
		  }
	}
	fclose(fp);
}


BOOL get_edge(  int i,  int j )
{
	int byte, bit;
	char mask;
	
	bit  = 7-(j & 0x00000007);
	byte = j >> 3;
	
	mask = masks[bit];
	return( (Bitmap[i][byte] & mask)==mask );
}


void read_graph_DIMACS_bin( FILE* fp )
{
	int i,j, length = 0;

	if (!fscanf(fp, "%d\n", &length))
	  { printf("ERROR: Corrupted preamble.\n"); exit(10); }

	if(length >= MAX_PREAMBLE)
	  { printf("ERROR: Too long preamble.\n"); exit(10); }
		   
	fread(Preamble, 1, length, fp);
	Preamble[length] = '\0';
	
	if (!get_params())
		  { printf("ERROR: Corrupted preamble.\n"); exit(10); }

	for ( i = 0
		 ; i < Nr_vert && fread(Bitmap[i], 1, (int)((i + 8)/8), fp)
		 ; i++ );
	fclose(fp);

/*
	{
		int i,j;
		FILE *fp;
		
		if ( (fp=fopen("Test.gra","w"))==NULL )
		  { printf("ERROR: Cannot open outfile\n"); exit(10); }
		
		fprintf(fp, Preamble);
		
		for ( i = 0; i<Nr_vert; i++ )
		  {
			  for ( j=0; j<=i; j++ )
				if ( get_edge(i,j) ) fprintf(fp,"e %d %d\n",i+1,j+1 );
		  }
		
		fclose(fp);

	}
*/
	/* ------------------------------------------------------- */	
	/* set the edges of the graph */		
	for ( i = 0; i<Nr_vert; i++ )
	  {
		  for ( j=0; j<=i; j++ )
	/*		if ( get_edge(i,j) ) fprintf(fp,"e %d %d\n",i+1,j+1 ); */
			if ( get_edge(i,j) && i != j ) 
				zddGraphSetEdge( i, j );
	  }
	/* ------------------------------------------------------- */	
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


