/**CHeaderFile*****************************************************************

  FileName    [extra.h]

  PackageName [extra]

  Synopsis    [Experimental version of some DD-based procedures.]

  Description [This library contains a number of operators and 
  traversal routines developed to extend the functionality of 
  CUDD v.2.3.x, by Fabio Somenzi (http://vlsi.colorado.edu/~fabio/)
  To compile your code with the library, #include "cuddInt.h" and 
  "extra.h" in your source files and link your project to CUDD and 
  this library. Use the library at your own risk and with caution. 
  Notice that debugging of some operators is still not finished.]

  SeeAlso     []

  Author      [Alan Mishchenko]

  Copyright   []

  Revision    [$extra.h, v.1.3, Sep 22, 2001, alanmi $]

******************************************************************************/

#ifndef __EXTRA_H__
#define __EXTRA_H__

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/
#include "util.h"
#include "cuddInt.h"
#include "st.h"

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

/* constants of the manager */
#define		b0     Cudd_Not((dd)->one)
#define		b1              (dd)->one
#define		z0              (dd)->zero
#define		z1              (dd)->one
#define		a0              (dd)->zero
#define		a1              (dd)->one


// hash key macros
#define hashKey1(a,TSIZE) \
((unsigned)(a) % TSIZE)

#define hashKey2(a,b,TSIZE) \
(((unsigned)(a) + (unsigned)(b) * DD_P1) % TSIZE)

#define hashKey3(a,b,c,TSIZE) \
(((((unsigned)(a) + (unsigned)(b)) * DD_P1 + (unsigned)(c)) * DD_P2 ) % TSIZE)

#define hashKey4(a,b,c,d,TSIZE) \
((((((unsigned)(a) + (unsigned)(b)) * DD_P1 + (unsigned)(c)) * DD_P2 + \
   (unsigned)(d)) * DD_P3) % TSIZE)

#define hashKey5(a,b,c,d,e,TSIZE) \
(((((((unsigned)(a) + (unsigned)(b)) * DD_P1 + (unsigned)(c)) * DD_P2 + \
   (unsigned)(d)) * DD_P3 + (unsigned)(e)) * DD_P1) % TSIZE)


// complementation and testing for pointers for hash entries
#define Hash_IsComplement(p)  ((int)((long) (p) & 02))
#define Hash_Regular(p)       ((DdNode*)((unsigned)(p) & ~02))
#define Hash_Not(p)           ((DdNode*)((long)(p) ^ 02))
#define Hash_NotCond(p,c)     ((DdNode*)((long)(p) ^ (02*c)))

////////////////////////////////////////////////////////////////////////
///                      debugging macros                            ///
////////////////////////////////////////////////////////////////////////

#define PRB(f)       printf( #f " = " ); Extra_bddPrint(dd,f); printf( "\n" )
#define PRK(f,n)     Extra_PrintKMap(stdout,dd,(f),Cudd_Not(f),(n),NULL,0); printf( "K-map for function" #f "\n\n" )
#define PRK2(f,g,n)  Extra_PrintKMap(stdout,dd,(f),(g),(n),NULL,0); printf( "K-map for function <" #f ", " #g ">\n\n" ) 
#define PRT(a,t)     printf("%s = ", (a)); printf( "%.2f sec\n", (float)(t)/(float)(CLOCKS_PER_SEC) )

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

/*===========================================================================*/
/*=== BDD-based functions ===================================================*/
/*===========================================================================*/

/*=== bMisc.c ===============================================================*/

/* remaps the function to depend on the topmost variables on the manager. */
EXTERN DdNode * Extra_bddRemapUp ARGS((DdManager * dd, DdNode * bF));
/* finds one cube belonging to the on-set of the function */
EXTERN DdNode * Extra_bddFindOneCube ARGS((DdManager * dd, DdNode * bF));
/* finds one minterm belonging to the on-set of the function */
EXTERN DdNode * Extra_bddFindOneMinterm ARGS((DdManager * dd, DdNode * bF, int nVars));

/* compute the random function with the given density */
EXTERN DdNode * Extra_bddRandomFunc ARGS((DdManager * dd, int n, double d));
/* prints the bdd in the form of disjoint sum of products */
EXTERN void     Extra_bddPrint ARGS((DdManager * dd, DdNode * F));
/* build the set of all tuples of K variables out of N */
EXTERN DdNode * Extra_bddTuples ARGS((DdManager * dd, int K, DdNode *bVarsN));
EXTERN DdNode *  extraBddTuples ARGS((DdManager * dd, DdNode *bVarsK, DdNode *bVarsN));
/* changes the polarity of vars listed in the cube */
EXTERN DdNode * Extra_bddChangePolarity ARGS((DdManager * dd, DdNode * bFunc, DdNode * bVars));
EXTERN DdNode *  extraBddChangePolarity ARGS((DdManager * dd, DdNode * bFunc, DdNode * bVars));

/* converts the bit-string (nbits <= 32) into the BDD cube */
EXTERN DdNode * Extra_bddBitsToCube ARGS((DdManager * dd, int Code, int CodeWidth, DdNode ** pbVars));
/* computes the positive polarty cube composed of the first vars in the array */
EXTERN DdNode * Extra_bddComputeCube ARGS((DdManager * dd, DdNode ** bXVars, int nVars));
/* visualize the BDD/ADD/ZDD */
EXTERN void Extra_bddShow ARGS((DdManager * dd, DdNode * bFunc));
EXTERN void Extra_addShowFromBdd ARGS((DdManager * dd, DdNode * bFunc));
/* performs the boolean difference w.r.t to a cube (AKA unique quontifier) */
EXTERN DdNode * Extra_bddBooleanDiffCube ARGS((DdManager * dd, DdNode * bFunc, DdNode * bCube));
EXTERN DdNode *  extraBddBooleanDiffCube ARGS((DdManager * dd, DdNode * bFunc, DdNode * bCube));


/* find the profile of a DD (the number of nodes on each level) */
EXTERN int * Extra_ProfileNode ARGS((DdManager * dd, DdNode * F, int * Profile));
/* find the profile of a shared set of DDs (the number of nodes on each level) */
EXTERN int * Extra_ProfileNodeSharing ARGS((DdManager * dd, DdNode ** pFuncs, int nFuncs, int * Profile));

/* find the profile of a DD (the number of edges crossing each level) */
EXTERN int * Extra_ProfileWidth ARGS((DdManager * dd, DdNode * F, int * Profile));
/* find the profile of a shared set of DDs (the number of edges crossing each level) */
EXTERN int * Extra_ProfileWidthSharing ARGS((DdManager * dd, DdNode ** pFuncs, int nFuncs, int * Profile));

/* find the maximum width of the BDD */
EXTERN int Extra_ProfileWidthMax ARGS((DdManager * dd, DdNode * F));
/* find the maximum width of the BDD */
EXTERN int Extra_ProfileWidthSharingMax ARGS((DdManager * dd, DdNode ** pFuncs, int nFuncs));

/* find the profile of a DD (the number of edges of each length) */
EXTERN int * Extra_ProfileEdge ARGS((DdManager * dd, DdNode * F, int * Profile));
/* find the profile of a shared set of DDs (the number of edges of each length) */
EXTERN int * Extra_ProfileEdgeSharing ARGS((DdManager * dd, DdNode ** pFuncs, int nFuncs, int * Profile));
/* permutes variables the array of functions */
EXTERN void Extra_bddPermuteArray ARGS((DdManager * dd, DdNode ** bNodesIn, DdNode ** bNodesOut, int nNodes, int *permut));

/* finds the smallest integer larger of equal than the logarithm. */
EXTERN int Extra_Base2Log ARGS((unsigned Num));
/* returns the power of two as a double */
EXTERN double Extra_Power2 ARGS((unsigned Num));

/*=== bSupp.c =============================================================*/

/* returns the size of the support */
EXTERN int Extra_bddSuppSize ARGS((DdManager * dd, DdNode * bSupp));
/* returns 1 if the support contains the given BDD variable */
EXTERN int Extra_bddSuppContainVar ARGS((DdManager * dd, DdNode * bS, DdNode * bVar));
/* returns 1 if two supports represented as BDD cubes are overlapping */
EXTERN int Extra_bddSuppOverlapping ARGS((DdManager * dd, DdNode * S1, DdNode * S2));
/* returns the number of different vars in two supports */
EXTERN int Extra_bddSuppDifferentVars ARGS((DdManager * dd, DdNode * S1, DdNode * S2, int DiffMax));
/* checks the support containment */
EXTERN int Extra_bddSuppCheckContainment ARGS((DdManager * dd, DdNode * bL, DdNode * bH, DdNode ** bLarge, DdNode ** bSmall));

/* get support of the DD as an array of integers */
EXTERN int * Extra_SupportArray ARGS((DdManager * dd, DdNode * F, int * support));
/* get support of the array of DDs as an array of integers */
EXTERN int * Extra_VectorSupportArray ARGS((DdManager * dd, DdNode ** F, int n, int * support));
/* get support of support and stores it in cache */
EXTERN DdNode * Extra_SupportCache ARGS((DdManager * dd, DdNode * f));
/* get support of the DD as a ZDD */
EXTERN DdNode * Extra_zddSupport ARGS((DdManager * dd, DdNode * f));
/* get support as a negative polarity BDD cube */
EXTERN DdNode * Extra_bddSupportNegativeCube ARGS((DdManager * dd, DdNode * f));

/*=== bDecomp.c =============================================================*/

/* disjointly decomposes the function with the given set of variables */
EXTERN int Extra_bddDecompose ARGS((DdManager * dd, DdNode * F, DdNode * Vars, DdNode * NewVar, DdNode ** FuncH, DdNode ** FuncG));

/*=== bEncoding.c =============================================================*/

/* performs the binary encoding of the set of function using the given vars */
EXTERN DdNode * Extra_bddEncodingBinary ARGS((DdManager * dd, DdNode ** pbFuncs, int nFuncs, DdNode ** pbVars, int nVars));
/* solves the column encoding problem using a sophisticated method */
EXTERN DdNode * Extra_bddEncodingNonStrict ARGS((DdManager * dd, DdNode ** pbColumns, int nColumns, DdNode * bVarsCol, DdNode ** pCVars, int nMulti, int * pSimple));

/*=== bNodePaths.c =============================================================*/

/* for each node in the BDD, computes the sum of paths leading to it from the root */
EXTERN st_table * Extra_bddNodePaths ARGS((DdManager * dd, DdNode * bFunc));
/* for each node in the ADD, computes the sum of paths leading to it from the root */
EXTERN st_table * Extra_addNodePaths ARGS((DdManager * dd, DdNode * bFunc));
/* collects the nodes under the cut and, for each node, computes the sum of paths leading to it from the root */
EXTERN st_table * Extra_bddNodePathsUnderCut ARGS((DdManager * dd, DdNode * bFunc, int CutLevel));
/* collects the nodes under the cut starting from the given set of ADD nodes */
EXTERN int Extra_bddNodePathsUnderCutArray ARGS((DdManager * dd, DdNode ** paNodes, DdNode ** pbCubes, int nNodes, DdNode ** paNodesRes, DdNode ** pbCubesRes, int CutLevel));

/*=== bzShift.c =================================================================*/

/* shifts the BDD up/down by one variable */
EXTERN DdNode * Extra_bddShift ARGS((DdManager * dd, DdNode * bF, int fShiftUp));
EXTERN DdNode *  extraBddShift ARGS((DdManager * dd, DdNode * bF, DdNode * bFlag));
/* shifts the XDD up/down by one variable */
EXTERN DdNode * Extra_zddShift ARGS((DdManager * dd, DdNode * zF, int fShiftUp));
EXTERN DdNode *  extraZddShift ARGS((DdManager * dd, DdNode * zF, DdNode * zFlag));
/* swaps two variables in the BDD */
EXTERN DdNode * Extra_bddSwapVars ARGS((DdManager * dd, DdNode * bF, int iVar1, int iVar2));
EXTERN DdNode *  extraBddSwapVars ARGS((DdManager * dd, DdNode * bF, DdNode * bVars));

/*=== bCache.c =================================================================*/

/* the internal signature counter */
EXTERN unsigned g_CacheSignature;
/* the internal cache size */
EXTERN unsigned g_CacheTableSize;

EXTERN void Extra_CacheAllocate ARGS((int nEntries));
EXTERN void Extra_CacheClear ARGS(());
EXTERN void Extra_CacheDelocate ARGS(());

EXTERN int Extra_CheckColumnCompatibility ARGS((DdManager * dd, DdNode * bF1, DdNode * bF2, DdNode * bC1, DdNode * bC2));
EXTERN int Extra_CheckColumnCompatibilityGroup ARGS((DdManager * dd, DdNode * bF1, DdNode * bF2, DdNode * bC1, DdNode * bC2, DdNode * bVars));
EXTERN int Extra_CheckRootFunctionIdentity ARGS((DdManager * dd, DdNode * bF1, DdNode * bF2, DdNode * bC1, DdNode * bC2));
EXTERN int Extra_CheckOrBiDecomposability ARGS((DdManager * dd, DdNode * bQ, DdNode * bR, DdNode * bVa, DdNode * bVb));

/*=== bImage.c =================================================================*/

/* allocates the data structure for multiple image computation */
//EXTERN int  Extra_bddImageAllocate ARGS((DdManager * dd, int nInputs, int nVarsMax, DdNode ** pbXVars, DdNode ** pbYVars));
/* performs one instance of image computation */
//EXTERN DdNode * Extra_bddImageCompute ARGS((DdNode ** bParts, int nParts, DdNode * bCareSet, int fPartitioned, int fReplace));
/* delocates the data structure for image computation */
//EXTERN void Extra_bddImageDelocate ARGS(());

EXTERN DdNode * 
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
  int fMonolitic );     // use the naive image computation algorithm


/*=== bTransfer.c =================================================================*/

/* convert a {A,B}DD from a manager to another with variable remapping */
EXTERN DdNode * Extra_TransferPermute ARGS((DdManager * ddSource, DdManager * ddDestination, DdNode * f, int * Permute));

/*=== bSPFD.c =================================================================*/

/* computes the unique info provided by the variable */
EXTERN DdNode * Extra_bddInfoUnique ARGS((DdManager * dd, DdNode * bF, int iVar, int nVars, int Set1, int Set2, int Step));
/* computes the complete info provided by the variable */
EXTERN DdNode * Extra_bddInfoComplete ARGS((DdManager * dd, DdNode * bF, int iVar, int nVars, int Set1, int Set2, int Step));
/* computes the equal info in the relation */
EXTERN DdNode * Extra_bddInfoEqual ARGS((DdManager * dd, DdNode * bF, int iVar, int nVars, int Set1, int Set2, int Step));

/*=== bSymm.c =================================================================*/

typedef struct {
	int nVars;      // the number of variables in the support
	int nVarsMax;   // the number of variables in the DD manager
	int nSymms;     // the number of pair-wise symmetries
	int nNodes;     // the number of nodes in a ZDD (if applicable)
	int * pVars;    // the list of all variables present in the support
	char ** pSymms; // the symmetry information
} symminfo;

/* computes the classical symmetry information for the function - recursive */
EXTERN symminfo * Extra_SymmPairsComputeFast ARGS((DdManager * dd, DdNode * bFunc));
/* computes the classical symmetry information for the function - using smart ideas */
EXTERN symminfo * Extra_SymmPairsComputeSlow ARGS((DdManager * dd, DdNode * bFunc));
/* computes the classical symmetry information for the function - using naive approach */
EXTERN symminfo * Extra_SymmPairsComputeNaive ARGS((DdManager * dd, DdNode * bFunc));
EXTERN int        Extra_bddCheckVarsSymmetricNaive ARGS((DdManager * dd, DdNode * bF, int iVar1, int iVar2));

/* allocated the data structure */
EXTERN symminfo * Extra_SymmPairsAllocate ARGS((int nVars));
/* delocated the data structure */
EXTERN void Extra_SymmPairsDissolve ARGS((symminfo *));
/* print the contents the data structure */
EXTERN void Extra_SymmPairsPrint ARGS((symminfo *));
/* converts the ZDD into the symminfo structure */
EXTERN symminfo * Extra_SymmPairsCreateFromZdd ARGS((DdManager * dd, DdNode * zPairs, DdNode * bVars));

/* computes the classical symmetry information as a ZDD */
EXTERN DdNode * Extra_zddSymmPairsCompute ARGS((DdManager * dd, DdNode * bF, DdNode * bVars));
EXTERN DdNode *  extraZddSymmPairsCompute ARGS((DdManager * dd, DdNode * bF, DdNode * bVars));
/* returns a singleton-set ZDD containing all variables that are symmetric with the given one */
EXTERN DdNode * Extra_zddGetSymmetricVars ARGS((DdManager * dd, DdNode * bF, DdNode * bG, DdNode * bVars));
EXTERN DdNode *  extraZddGetSymmetricVars ARGS((DdManager * dd, DdNode * bF, DdNode * bG, DdNode * bVars));
/* converts a set of variables into a set of singleton subsets */
EXTERN DdNode * Extra_zddGetSingletons ARGS((DdManager * dd, DdNode * bVars));
EXTERN DdNode *  extraZddGetSingletons ARGS((DdManager * dd, DdNode * bVars));
/* filters the set of variables using the support of the function */
EXTERN DdNode * Extra_bddReduceVarSet ARGS((DdManager * dd, DdNode * bVars, DdNode * bF));
EXTERN DdNode *  extraBddReduceVarSet ARGS((DdManager * dd, DdNode * bVars, DdNode * bF));

/* checks the possibility that the two vars are symmetric */
EXTERN int      Extra_bddCheckVarsSymmetric ARGS((DdManager * dd, DdNode * bF, int iVar1, int iVar2));
EXTERN DdNode *  extraBddCheckVarsSymmetric ARGS((DdManager * dd, DdNode * bF, DdNode * bVars));

/*=== bEquivN.c =================================================================*/

/* given the function, computes a unique representative of its N-equivalence class */
EXTERN DdNode * Extra_bddNCanonicalForm ARGS((DdManager * dd, DdNode *  bF, DdNode ** bP));
EXTERN DdNode *  extraBddNCanonicalForm ARGS((DdManager * dd, DdNode *  bF, DdNode *  bVars, DdNode ** bP));

/* compares the truth vectors of two Boolean functions */
EXTERN int Extra_bddCompareTruthVectors ARGS((DdManager * dd, DdNode * bF, DdNode * bG));

/* extract positive polarity vars from the mixed polarity cube */
EXTERN DdNode * Extra_bddCubeExtractPositiveVars ARGS((DdManager * dd, DdNode * bCube));
EXTERN DdNode *  extraBddCubeExtractPositiveVars ARGS((DdManager * dd, DdNode * bCube));
/* converts a mixed polarity cube into a positive polarity one */
EXTERN DdNode * Extra_bddCubeConvertIntoPositiveVars ARGS((DdManager * dd, DdNode * bCube));
EXTERN DdNode *  extraBddCubeConvertIntoPositiveVars ARGS((DdManager * dd, DdNode * bCube));
/* scrolls through the cube as long as the cube's topmost var is above the given level */
EXTERN DdNode * extraCubeScroll ARGS((DdManager * dd, DdNode *  bCube, int Level));

/*=== bEquivNPN.c =================================================================*/


/*===========================================================================*/
/*=== ZDD-based functions ===================================================*/
/*===========================================================================*/

/*=== zCC.c =================================================================*/

/* fast implicit heuristic CC solver */
EXTERN DdNode * Extra_zddSolveCC ARGS((DdManager *dd, DdNode *zRows, DdNode *zCols, int fColsCoverRows, int fVerbose));

/*=== zCovers.c ==============================================================*/

/* the result of this operation is primes contained in the product of cubes */
EXTERN DdNode * Extra_zddPrimeProduct ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode *  extraZddPrimeProduct ARGS((DdManager *dd, DdNode *f, DdNode *g));
/* an alternative implementation of the cover product */
EXTERN DdNode * Extra_zddProductAlt ARGS((DdManager *dd, DdNode *f, DdNode *g));
EXTERN DdNode *  extraZddProductAlt ARGS((DdManager *dd, DdNode *f, DdNode *g));
/* returns the set of cubes pair-wise unate with the given cube */
EXTERN DdNode * Extra_zddCompatible ARGS((DdManager * dd, DdNode * zCover, DdNode * zCube));
EXTERN DdNode *  extraZddCompatible ARGS((DdManager * dd, DdNode * zCover, DdNode * zCube));
/* a wrapper for the call to Extra_zddIsop() */
EXTERN DdNode * Extra_zddIsopCover ARGS((DdManager * dd, DdNode * F1, DdNode * F12));
/* a wrapper for the call to Extra_zddIsopCover() and Extra_zddPrintCover() */
EXTERN void Extra_zddIsopPrintCover ARGS((DdManager * dd, DdNode * F1, DdNode * F12));
/* an alternative ISOP cover computation (faster than Extra_zddIsop()) */
EXTERN DdNode * Extra_zddIsopCoverAlt ARGS((DdManager * dd, DdNode * F1, DdNode * F12));
EXTERN DdNode *  extraZddIsopCoverAlt ARGS((DdManager * dd, DdNode * F1, DdNode * F12));
/* computes the disjoint cube cover produced by the bdd paths */
EXTERN DdNode * Extra_zddDisjointCover ARGS((DdManager * dd, DdNode * F));
/* performs resolution on the set of clauses (S) w.r.t. variables in zdd Vars */
EXTERN DdNode * Extra_zddResolve ARGS((DdManager * dd, DdNode * S, DdNode * Vars));
/* cubes from zC that are not contained by cubes from zD over area bA */
EXTERN DdNode * Extra_zddNotContainedCubesOverArea ARGS((DdManager * dd, DdNode * zC, DdNode * zD, DdNode * bA));
EXTERN DdNode *  extraZddNotContainedCubesOverArea ARGS((DdManager * dd, DdNode * zC, DdNode * zD, DdNode * bA));

/* finds cofactors of the cover w.r.t. the top-most variable without creating new nodes */
EXTERN void extraDecomposeCover ARGS((DdManager* dd, DdNode *C, DdNode **zC0, DdNode **zC1, DdNode **zC2 ));
/* composes the cover from the three subcovers using the given variable (returns NULL = reordering)*/
EXTERN DdNode * extraComposeCover ARGS((DdManager* dd, DdNode * zC0, DdNode * zC1, DdNode * zC2, int TopVar ));

/* selects one cube from a ZDD representing the cube cover */
EXTERN DdNode * Extra_zddSelectOneCube ARGS((DdManager * dd, DdNode * zS));
EXTERN DdNode *  extraZddSelectOneCube ARGS((DdManager * dd, DdNode * zS));
/* selects one subset from a ZDD representing the set of subsets */
EXTERN DdNode * Extra_zddSelectOneSubset ARGS((DdManager * dd, DdNode * zS));
EXTERN DdNode *  extraZddSelectOneSubset ARGS((DdManager * dd, DdNode * zS));

/* checks unateness of the cover */
EXTERN int Extra_zddCheckUnateness ARGS((DdManager * dd, DdNode * zCover));


/*=== zCoverPrint.c ================================================================*/

/* writes the BDD into a BLIF file as one SOP block */
EXTERN void Extra_WriteFunctionSop ARGS((DdManager * dd, DdNode * bFunc, DdNode * bFuncDc, char ** pNames, char * OutputName, char * FileName));
EXTERN void  extraWriteFunctionSop ARGS((FILE * pFile, DdManager * dd, DdNode * zCover, int levPrev, int nLevels, const char * AddOn, int * VarMask));

/* writes the {A,B}DD into a BLIF file as a network of MUXes */
EXTERN void Extra_WriteFunctionMuxes ARGS((DdManager * dd, DdNode * bFunc, char ** pNames, char * OutputName, char * FileName));
EXTERN void  extraWriteFunctionMuxes ARGS((FILE * pFile, DdNode * Func, char * OutputName, char * Prefix, char ** InputNames));

/* verifies the multi-output function against the one written into the file */
EXTERN int Extra_PerformVerification ARGS((DdManager * dd, DdNode ** pOutputs, DdNode ** pOutputOffs, int nOutputs, char * FileName));
/* verify that the function falls within the given range */
EXTERN int Extra_VerifyRange ARGS((DdManager * dd, DdNode * bFunc, DdNode * bLower, DdNode * bUpper));


/*=== zFactor.c ================================================================*/

/* counting the number of literals in the factored form */
EXTERN int Extra_bddFactoredFormLiterals ARGS((DdManager * dd, DdNode * bOnSet, DdNode * bOnDcSet));
EXTERN DdNode * Extra_zddFactoredFormLiterals ARGS((DdManager * dd, DdNode * zCover));
EXTERN DdNode * Extra_zddLFLiterals ARGS((DdManager * dd, DdNode * zCover, DdNode * zCube));

/* computing a quick divisor */
EXTERN DdNode * Extra_zddQuickDivisor ARGS((DdManager * dd, DdNode * zCover));
EXTERN DdNode * Extra_zddLevel0Kernel ARGS((DdManager * dd, DdNode * zCover));

/* division with quotient and remainder */
EXTERN void Extra_zddDivision ARGS((DdManager * dd, DdNode * zCover, DdNode * zDiv, DdNode ** zQuo, DdNode ** zRem));
/* the common cube */
EXTERN DdNode * Extra_zddCommonCubeFast ARGS((DdManager * dd, DdNode * zCover));
/* the cube of literals that occur more than once */
EXTERN DdNode * Extra_zddMoreThanOnceCubeFast ARGS((DdManager * dd, DdNode * zCover));

/* making the cover cube-free */
EXTERN DdNode * Extra_zddMakeCubeFree ARGS((DdManager * dd, DdNode * zCover, int iZVar));
/* testing whether the cover is cube-free */
EXTERN int Extra_zddTestCubeFree ARGS((DdManager * dd, DdNode * zCover));

/* counts the number of literals in the simple cover */
EXTERN int Extra_zddCountLiteralsSimple ARGS((DdManager * dd, DdNode * zCover));
/* tests whether the cover contains more than one cube */
EXTERN int Extra_zddMoreThanOneCube ARGS((DdManager * dd, DdNode * zCover));
/* the cube from levels */
EXTERN DdNode * Extra_zddCombinationFromLevels ARGS((DdManager * dd, int * pLevels, int nVars));

/* determines common literals */
EXTERN int Extra_zddCommonLiterals ARGS((DdManager * dd, DdNode * zCover, int iZVar, int * pLevels, int * pLiterals));
/* determines the set of literals that occur more than once */
EXTERN int Extra_zddMoreThanOneLiteralSet ARGS((DdManager * dd, DdNode * zCover, int StartLevel, int * pVars, int * pCounters));
/* tests whether the given literal literal occurs more than once */
EXTERN int      Extra_zddMoreThanOneLiteral ARGS((DdManager * dd, DdNode * zCover, int iZVar));
EXTERN DdNode *  extraZddMoreThanOneLiteral ARGS((DdManager * dd, DdNode * zCover, DdNode * zVar));


/*=== zRondo.c ================================================================*/

/* computes a minimum single-output cover using Rondo */
EXTERN DdNode * Extra_zddRondo ARGS((DdManager * dd, DdNode * F1, DdNode * F12));


/*=== zISOP.c ================================================================*/

/* improvements to the Irredundant Prime Cover computation */
EXTERN DdNode * Extra_zddIsopCoverAllVars ARGS((DdManager * dd, DdNode * F1, DdNode * F12));
EXTERN DdNode *  extraZddIsopCoverAllVars ARGS((DdManager * dd, DdNode * F1, DdNode * F12, DdNode * bS));

EXTERN DdNode * Extra_zddIsopCoverUnateVars ARGS((DdManager * dd, DdNode * F1, DdNode * F12));
EXTERN DdNode *  extraZddIsopCoverUnateVars ARGS((DdManager * dd, DdNode * F1, DdNode * F12, DdNode * bS));

/* computes an ISOP cover with a random ordering of variables */
EXTERN DdNode * Extra_zddIsopCoverRandom ARGS((DdManager * dd, DdNode * F1, DdNode * F12));

EXTERN DdNode * Extra_zddIsopCoverReduced ARGS((DdManager * dd, DdNode * F1, DdNode * F12));
EXTERN DdNode *  extraZddIsopCoverReduced ARGS((DdManager * dd, DdNode * F1, DdNode * F12));

/*=== zMaxMin.c ==============================================================*/

/* maximal/minimimal */
EXTERN DdNode * Extra_zddMaximal ARGS((DdManager *dd, DdNode *S));
EXTERN DdNode *  extraZddMaximal ARGS((DdManager *dd, DdNode *S));
EXTERN DdNode * Extra_zddMinimal ARGS((DdManager *dd, DdNode *S));
EXTERN DdNode *  extraZddMinimal ARGS((DdManager *dd, DdNode *S));
/* maximal/minimal of the union of two sets of subsets */
EXTERN DdNode * Extra_zddMaxUnion ARGS((DdManager *dd, DdNode *S, DdNode *T));
EXTERN DdNode *  extraZddMaxUnion ARGS((DdManager *dd, DdNode *S, DdNode *T));
EXTERN DdNode * Extra_zddMinUnion ARGS((DdManager *dd, DdNode *S, DdNode *T));
EXTERN DdNode *  extraZddMinUnion ARGS((DdManager *dd, DdNode *S, DdNode *T));
/* dot/cross products */
EXTERN DdNode * Extra_zddDotProduct ARGS((DdManager *dd, DdNode *S, DdNode *T));
EXTERN DdNode *  extraZddDotProduct ARGS((DdManager *dd, DdNode *S, DdNode *T));
EXTERN DdNode * Extra_zddCrossProduct ARGS((DdManager *dd, DdNode *S, DdNode *T));
EXTERN DdNode *  extraZddCrossProduct ARGS((DdManager *dd, DdNode *S, DdNode *T));
EXTERN DdNode * Extra_zddMaxDotProduct ARGS((DdManager *dd, DdNode *S, DdNode *T));
EXTERN DdNode *  extraZddMaxDotProduct ARGS((DdManager *dd, DdNode *S, DdNode *T));

/*=== zSubSet.c ==============================================================*/

/* subset/supset operations */
EXTERN DdNode * Extra_zddSubSet    ARGS((DdManager *dd, DdNode *X, DdNode *Y));
EXTERN DdNode *  extraZddSubSet    ARGS((DdManager *dd, DdNode *X, DdNode *Y));
EXTERN DdNode * Extra_zddSupSet    ARGS((DdManager *dd, DdNode *X, DdNode *Y));
EXTERN DdNode *  extraZddSupSet    ARGS((DdManager *dd, DdNode *X, DdNode *Y));
EXTERN DdNode * Extra_zddNotSubSet ARGS((DdManager *dd, DdNode *X, DdNode *Y));
EXTERN DdNode *  extraZddNotSubSet ARGS((DdManager *dd, DdNode *X, DdNode *Y));
EXTERN DdNode * Extra_zddNotSupSet ARGS((DdManager *dd, DdNode *X, DdNode *Y));
EXTERN DdNode *  extraZddNotSupSet ARGS((DdManager *dd, DdNode *X, DdNode *Y));
EXTERN DdNode * Extra_zddMaxNotSupSet ARGS((DdManager *dd, DdNode *X, DdNode *Y));
EXTERN DdNode *  extraZddMaxNotSupSet ARGS((DdManager *dd, DdNode *X, DdNode *Y));
/* check whether the empty combination belongs to the set of subsets */
EXTERN int Extra_zddEmptyBelongs  ARGS((DdManager *dd, DdNode* zS ));
/* check whether the set consists of one subset only */
EXTERN int Extra_zddIsOneSubset ARGS((DdManager *dd, DdNode* zS ));

/*=== zGraphFuncs.c ==============================================================*/

/* construct the set of cliques */
EXTERN DdNode * Extra_zddCliques ARGS((DdManager *dd, DdNode *G, int fMaximal)); 
/* construct the set of all maximal cliques */
EXTERN DdNode * Extra_zddMaxCliques ARGS((DdManager *dd, DdNode *G)); 
/* incrementally contruct the set of cliques */
EXTERN DdNode * Extra_zddIncremCliques ARGS((DdManager *dd, DdNode *G, DdNode *C)); 
EXTERN DdNode *  extraZddIncremCliques ARGS((DdManager *dd, DdNode *G, DdNode *C)); 

/*=== zGraphIO.c ==============================================================*/
   
/* read the graph in DIMACS format (both ASCII and binary)*/
EXTERN DdNode * Extra_zddGraphRead ARGS((char* FileDimacs, DdManager *dd, int *nNodes, int *nEdges)); 
/* write the graph in binary DIMACS format */
EXTERN int Extra_zddGraphWrite ARGS((char* FileDimacs, char* Preamble, DdManager *dd, DdNode *Graph));
/* dump the graph into file in DOT format */
EXTERN int Extra_zddGraphDumpDot ARGS((char * FileDot, DdManager * dd, DdNode * Graph));

/*=== zLitCount.c ==============================================================*/

/* count the number of times each variable occurs in the combinations */
EXTERN int * Extra_zddLitCount ARGS((DdManager * dd, DdNode * Set));

/*=== zPerm.c ==============================================================*/

/* quantifications */
EXTERN DdNode * Extra_zddExistAbstract ARGS((DdManager *manager, DdNode *F, DdNode *cube));
EXTERN DdNode *  extraZddExistAbstractRecur ARGS((DdManager *manager, DdNode *F, DdNode *cube));
/* changes the value of several variables in the ZDD */
EXTERN DdNode * Extra_zddChangeVars ARGS((DdManager *manager, DdNode *F, DdNode *cube));
EXTERN DdNode *  extraZddChangeVars ARGS((DdManager *manager, DdNode *F, DdNode *cube));
/* permutes variables in ZDD */
EXTERN DdNode * Extra_zddPermute  ARGS((DdManager *dd, DdNode *N, int *permut));
/* computes combinations in F with vars in Cube having the negative polarity */
EXTERN DdNode * Extra_zddCofactor0 ARGS((DdManager * dd, DdNode * f, DdNode * cube));
EXTERN DdNode *  extraZddCofactor0 ARGS((DdManager * dd, DdNode * f, DdNode * cube));
/* computes combinations in F with vars in Cube having the positive polarity */
EXTERN DdNode * Extra_zddCofactor1 ARGS((DdManager * dd, DdNode * f, DdNode * cube, int fIncludeVars));
EXTERN DdNode *  extraZddCofactor1 ARGS((DdManager * dd, DdNode * f, DdNode * cube, int fIncludeVars));


/*=== zMisc.c ==============================================================*/

/* build a ZDD for a combination of variables */
EXTERN DdNode * Extra_zddCombination ARGS((DdManager *dd, int* VarValues, int nVars));
EXTERN DdNode *  extraZddCombination ARGS((DdManager *dd, int *VarValues, int nVars ));
/* the set of all possible combinations of the given set of variables */
EXTERN DdNode * Extra_zddUniverse ARGS((DdManager * dd, DdNode * VarSet));
EXTERN DdNode *  extraZddUniverse ARGS((DdManager * dd, DdNode * VarSet));
/* build the set of all tuples of K variables out of N */
EXTERN DdNode * Extra_zddTuples ARGS((DdManager * dd, int K, DdNode *zVarsN));
EXTERN DdNode *  extraZddTuples ARGS((DdManager * dd, DdNode *zVarsK, DdNode *zVarsN));
/* build the set of all tuples of K variables out of N from the BDD cube */
EXTERN DdNode * Extra_zddTuplesFromBdd ARGS((DdManager * dd, int K, DdNode *bVarsN));
EXTERN DdNode *  extraZddTuplesFromBdd ARGS((DdManager * dd, DdNode *bVarsK, DdNode *bVarsN));
/* convert the set of singleton combinations into one combination */
EXTERN DdNode * Extra_zddSinglesToComb ARGS((DdManager * dd, DdNode * Singles));
EXTERN DdNode *  extraZddSinglesToComb ARGS(( DdManager * dd, DdNode * Singles ));
/* returns the set of combinations containing the max/min number of elements */
EXTERN DdNode * Extra_zddMaximum ARGS((DdManager * dd, DdNode * S, int * nVars));
EXTERN DdNode *  extraZddMaximum ARGS((DdManager * dd, DdNode * S, int * nVars));
EXTERN DdNode * Extra_zddMinimum ARGS((DdManager * dd, DdNode * S, int * nVars));
EXTERN DdNode *  extraZddMinimum ARGS((DdManager * dd, DdNode * S, int * nVars));
/* returns the random set of k combinations of n elements with average density d */
EXTERN DdNode * Extra_zddRandomSet ARGS((DdManager * dd, int n, int k, double d));

/*=== zUCP1c.c ==============================================================*/

/* solve the set covering problem over ZDD encoded sets */
EXTERN DdNode * Extra_zddSolveUCP1 ARGS((DdManager *dd, DdNode *S));

/*=== zUCP2c.c ==============================================================*/

/* derives the single-output function used for SOP minimization */
EXTERN int Extra_bddGetSingleOutput ARGS((DdManager * dd,  
	DdNode ** bIns, int nIns, DdNode ** bOuts, DdNode ** bOutDcs, int nOuts,
	DdNode ** bFuncOn, DdNode ** bFuncOnDc));

/* performs the implicit reduction of the covering matrix */
EXTERN DdNode * Extra_zddReduceMatrix ARGS((DdManager * dd, DdNode * bRows, DdNode * zCols, 
	DdNode ** zRowsCC, DdNode ** zColsCC, int fVerbose));

/* solves the CC approximately using ISOP */
EXTERN DdNode * Extra_zddSolveCCbyISOP ARGS((DdManager * dd, DdNode * zSolPart, DdNode * bFuncOnSet, DdNode * bFuncOnDcSet));

/* prints statistics for BDDs and ZDDs */
EXTERN void printStatsZdd ARGS((char* Name, DdManager* dd, DdNode* Zdd, long Time));
EXTERN void printStatsBdd ARGS((char* Name, DdManager* dd, DdNode* Bdd, long Time));

/* generates all primes of the boolean functions */
EXTERN DdNode * Extra_zddPrimes ARGS((DdManager* dd, DdNode* F ));
EXTERN DdNode *  extraZddPrimes ARGS((DdManager *dd, DdNode* F));
/* computation of the maximals of sets of dominating rows/columns */
EXTERN DdNode * Extra_zddMaxRowDom ARGS((DdManager *dd, DdNode* F, DdNode* P ));
EXTERN DdNode *  extraZddMaxRowDom ARGS((DdManager *dd, DdNode* F, DdNode* P));
EXTERN DdNode *  extraZddMaxColDomAux ARGS((DdManager *dd, DdNode* Q, DdNode* P));
EXTERN DdNode * Extra_zddMaxColDom ARGS((DdManager *dd, DdNode* Q, DdNode* P ));
EXTERN DdNode *  extraZddMaxColDom ARGS((DdManager *dd, DdNode* Q, DdNode* P));

/* counts the total number of literals */
EXTERN double Extra_zddLitCountTotal ARGS((DdManager* dd, const DdNode* Cover, double* nPaths ));


/*=== zExor.c ==============================================================*/

/* computes the Exclusive-OR-type union of two cube sets */
EXTERN DdNode	* Extra_zddUnionExor ARGS((DdManager * dd, DdNode * S, DdNode * T));

/* given two sets of cubes, computes the set of pair-wise supercubes */
EXTERN DdNode * Extra_zddSupercubes ARGS((DdManager *dd, DdNode * zA, DdNode * zB));
EXTERN DdNode *  extraZddSupercubes ARGS((DdManager *dd, DdNode * zA, DdNode * zB));

/* selects cubes from zA that have a distance-1 cube in zB */
EXTERN DdNode * Extra_zddSelectDist1Cubes ARGS((DdManager *dd, DdNode * zA, DdNode * zB));
EXTERN DdNode *  extraZddSelectDist1Cubes ARGS((DdManager *dd, DdNode * zA, DdNode * zB));

/* computes the set of fast ESOP covers for the multi-output function */
EXTERN int Extra_zddFastEsopCoverArray ARGS((DdManager * dd, DdNode ** bFs, DdNode ** zCs, int nFs));
/* computes a fast ESOP cover for the single-output function */
EXTERN DdNode * Extra_zddFastEsopCover ARGS((DdManager * dd, DdNode * bF, st_table * Visited, int * pnCubes));


/*=== zEspresso.c ==============================================================*/

/* a wrapper around the call to Espresso minimization of a single-output function */
EXTERN DdNode * Extra_zddEspresso ARGS((DdManager *dd, DdNode * zCover, DdNode * zCoverDC, int * nCubes, int * pPhases));
EXTERN DdNode * Extra_bddEspresso ARGS((DdManager *dd, DdNode * bOn,    DdNode * bOnDc,    int * nCubes, int * pPhases));

/* a wrapper around the call to Espresso minimization of a multi-output function */
EXTERN DdNode * Extra_zddEspressoMO ARGS((DdManager *dd, DdNode * zCover, DdNode * zCoverDC, int nIns, int nOuts, int * nCubes, int * pPhases));


/*===========================================================================*/
/*=== ZDD/BDD-based functions for heuristic SOP minimization ================*/
/*===========================================================================*/

/*=== hmEssen.c =============================================================*/

EXTERN DdNode* Extra_zddEssential ARGS((DdManager *dd, DdNode *zC, DdNode *bFuncOn));
EXTERN DdNode*  extraZddEssential ARGS((DdManager *dd, DdNode *zC, DdNode *bFuncOn));

/*=== hmExpand.c =============================================================*/

EXTERN DdNode* Extra_zddExpand ARGS((DdManager *dd, DdNode *zC, DdNode *bA, int fTopFirst));
EXTERN DdNode*  extraZddExpand ARGS((DdManager *dd, DdNode *zC, DdNode *bA, int fTopFirst));

/*=== hmIrred.c =============================================================*/

EXTERN DdNode* Extra_zddIrredundant ARGS((DdManager *dd, DdNode *zC, DdNode * bFuncOn));
EXTERN DdNode* Extra_zddMinimumIrredundant ARGS((DdManager *dd, DdNode *zC));
EXTERN DdNode*  extraZddMinimumIrredundant ARGS((DdManager *dd, DdNode *zC));
EXTERN DdNode* Extra_zddMinimumIrredundantExplicit ARGS((DdManager *dd, DdNode *zC, DdNode *bA));
EXTERN DdNode* Extra_bddCombination ARGS((DdManager * dd, int * VarValues));

/*=== hmLast.c =============================================================*/

EXTERN DdNode* Extra_zddLastGasp ARGS((DdManager *dd, DdNode *zC, DdNode * bFuncOn, DdNode *bFuncOnDc));
EXTERN DdNode* Extra_zddWrapAreaIntoCubes ARGS((DdManager *dd, DdNode *zC, DdNode *bA));
EXTERN DdNode*  extraZddWrapAreaIntoCubes ARGS((DdManager *dd, DdNode *zC, DdNode *bA));
EXTERN DdNode* Extra_zddCubesCoveringTwoAndMore ARGS((DdManager *dd, DdNode *zC, DdNode *zD));
EXTERN DdNode*  extraZddCubesCoveringTwoAndMore ARGS((DdManager *dd, DdNode *zC, DdNode *zD));

/*=== hmRandom.c =============================================================*/

/*=== hmReduce.c =============================================================*/

EXTERN DdNode* Extra_zddReduce ARGS((DdManager *dd, DdNode *zC, DdNode *bA, int fTopFirst));
EXTERN DdNode* extraZddReduce ARGS((DdManager *dd, DdNode *zC, DdNode *bA, int fTopFirst));

/*=== hmRondo.c =============================================================*/

/*=== hmUtil.c =============================================================*/

EXTERN DdNode * Extra_zddCoveredByArea ARGS((DdManager *dd, DdNode *zC, DdNode *bA));
EXTERN DdNode *  extraZddCoveredByArea ARGS((DdManager *dd, DdNode *zC, DdNode *bA));
EXTERN DdNode * Extra_zddNotCoveredByCover ARGS((DdManager *dd, DdNode *zC, DdNode *zD));
EXTERN DdNode *  extraZddNotCoveredByCover ARGS((DdManager *dd, DdNode *zC, DdNode *zD));
EXTERN DdNode * Extra_zddOverlappingWithArea ARGS((DdManager *dd, DdNode *zC, DdNode *bA));
EXTERN DdNode *  extraZddOverlappingWithArea ARGS((DdManager *dd, DdNode *zC, DdNode *bA));
EXTERN DdNode * Extra_zddConvertToBdd ARGS((DdManager *dd, DdNode *zC));
EXTERN DdNode *  extraZddConvertToBdd ARGS((DdManager *dd, DdNode *zC));
EXTERN DdNode * Extra_zddConvertEsopToBdd ARGS((DdManager *dd, DdNode *zC));
EXTERN DdNode *  extraZddConvertEsopToBdd ARGS((DdManager *dd, DdNode *zC));
EXTERN DdNode * Extra_zddConvertToBddAndAdd ARGS((DdManager *dd, DdNode *zC, DdNode *bA));
EXTERN DdNode *  extraZddConvertToBddAndAdd ARGS((DdManager *dd, DdNode *zC, DdNode *bA));
EXTERN DdNode * Extra_zddSingleCoveredArea ARGS((DdManager *dd, DdNode *zC));
EXTERN DdNode *  extraZddSingleCoveredArea ARGS((DdManager *dd, DdNode *zC));
EXTERN DdNode * Extra_zddConvertBddCubeIntoZddCube ARGS((DdManager *dd, DdNode *bCube));
EXTERN int      Extra_zddVerifyCover ARGS((DdManager * dd, DdNode * zC, DdNode * bFuncOn, DdNode * bFuncOnDc));


/*=== aCoverStats.c =============================================================*/

/* computes the BDD of the area covered by the max number of cubes in a ZDD. */
EXTERN DdNode * Extra_zddGetMostCoveredArea ARGS((DdManager * dd, DdNode * zC, int * nOverlaps));
EXTERN DdNode *  extraZddGetMostCoveredArea ARGS((DdManager * dd, DdNode * zC));


/*=== aMisc.c =============================================================*/

/* counts the number of different constant nodes of the ADD */
EXTERN int Extra_addCountConst ARGS((DdManager * dd, DdNode * aFunc));
/* counts the the number of different constant nodes of the array of ADDs */
EXTERN int Extra_addCountConstArray ARGS((DdManager * dd, DdNode ** paFuncs, int nFuncs));
/* finds the minimum value terminal node in the array of ADDs */
EXTERN DdNode * Extra_addFindMinArray ARGS((DdManager * dd, DdNode ** paFuncs, int nFuncs));
/* finds the maximum value terminal node in the array of ADDs */
EXTERN DdNode * Extra_addFindMaxArray ARGS((DdManager * dd, DdNode ** paFuncs, int nFuncs));
/* absolute value of an ADD */
EXTERN DdNode * Extra_addAbs ARGS((DdManager * dd, DdNode * f));
/* the encoded set of absolute values of the constant nodes of an ADD */
EXTERN DdNode * Extra_bddAddConstants ARGS((DdManager * dd, DdNode * aFunc));
/* determines whether this is an ADD or a BDD */
EXTERN int Extra_addDetectAdd ARGS((DdManager * dd, DdNode * Func));
/* restructure the ADD by replacing negative terminals with their abs value */
EXTERN DdNode * Cudd_addAbs ARGS((DdManager * dd, DdNode * f));


/*=== aSpectra.c =============================================================*/

/* Walsh spectrum computation */
EXTERN DdNode * Extra_bddWalsh ARGS((DdManager * dd, DdNode * bFunc, DdNode * bVars));
EXTERN DdNode *  extraBddWalsh ARGS((DdManager * dd, DdNode * bFunc, DdNode * bVars));
/* Reed-Muller spectrum computation */
EXTERN DdNode * Extra_bddReedMuller ARGS((DdManager * dd, DdNode * bFunc, DdNode * bVars));
EXTERN DdNode *  extraBddReedMuller ARGS((DdManager * dd, DdNode * bFunc, DdNode * bVars));
/* Haar spectrum computation */
EXTERN DdNode * Extra_bddHaar ARGS((DdManager * dd, DdNode * bFunc, DdNode * bVars));
EXTERN DdNode *  extraBddHaar ARGS((DdManager * dd, DdNode * bFunc, DdNode * bVars));
/* inverse Haar spectrum computation */
EXTERN DdNode * Extra_bddHaarInverse ARGS((DdManager * dd, DdNode * aFunc, DdNode * bVars));
EXTERN DdNode *  extraBddHaarInverse ARGS((DdManager * dd, DdNode * aFunc, DdNode * aSteps, DdNode * bVars, DdNode * bVarsAll, int nVarsAll, int * InverseMap));
/* remapping from the natural order into the sequential order */
EXTERN DdNode * Extra_addRemapNatural2Sequential ARGS((DdManager * dd, DdNode * aSource, DdNode * bVars));
/* addition function to update the negative-variable-assignment cube value in the ADD */
EXTERN DdNode * extraAddUpdateZeroCubeValue ARGS((DdManager * dd, DdNode * aFunc, DdNode * bVars, DdNode * aNode));
/* the generalized ITE for ADDs */
EXTERN DdNode * extraAddIteRecurGeneral ARGS((DdManager * dd, DdNode * bX, DdNode * aF, DdNode * aG));


/*=== bNet.c ================================================================*/

/* simple-minded package to read a blif file, by Fabio Somenzi */
#include "bnet.h"

/*=== bNetRead.c =============================================================*/
///                                                                  ///
///            Implementation of Pla/Blif File Reader                ///
///   using "simple-minded package Bnet" provided by Fabio Somenzi   ///
///   (parts of the code are also borrowed from Nanotrav Project)    ///
///                                                                  ///
///               Alan Mishchenko  <alanmi@ee.pdx.edu>               ///
///                                                                  ///
///  Ver. 1.0. Started - Sep 11, 2000  Last update - Sep 19, 2000    ///
///  Ver. 1.1. Started - Sep 20, 2000. Last update - Sep 23, 2000    ///
///  Ver. 1.2. Started - Feb 1,  2001  Last update - Feb 1,  2001    ///
///                                                                  ///

#define MAXINPUTS   1000       // the maximum number of inputs
#define MAXOUTPUTS  1000       // the maximum number of outputs
#define MAXLINE     10000      // the maximum length of one line in the Blif/Pla file
#define MAXLINES    1000000    // the maximum number of lines in the file

/* The structure representing multi-input multi-output incompletely 
   specified boolean function after reading the input file */
typedef struct {
	char * FileInput;     // the source file name
	char * FileGeneric;   // the generic file name (excluding ".pla" or ".blif")
	char * FileOutput;    // the output file name
	char * Model;         // the network name (Blif model, or FileGeneric for Pla files)
	int nInputs;          // the number of primary inputs
	int nOutputs;         // the number of primary outputs
	int nLatches;         // the number of latches
	int FileType;         // the file type (0 for Blif; one of the few possibilities for Pla)
	DdManager * dd;       // the current bdd manager
	DdNode ** pInputs;    // the elementary bdds for primary inputs
	DdNode ** pOutputs;   // the bdd for the primary output functions
	DdNode ** pOutputDcs; // the bdd for the don't care sets of the primary output functions (Pla files)
	DdNode ** pOutputOffs;// the bdd for the offsets of output functions
	char ** pInputNames;  // the names of primary inputs
	char ** pOutputNames; // the names of primary outputs
} BFunc;


/* The reader file types */
enum { blif, fd, fr, f, fdr, esop };

/* Derived the BFunc structure */
EXTERN int Extra_ReadFile ARGS((BFunc * pFunc));

/* Delocates the BFunc object when out of use */
EXTERN void Extra_Dissolve ARGS((BFunc * pFunc));


/*=== bVisUtils.c ================================================================*/

///            K-map visualization using pseudo graphics             ///
///             Version 1.0. Started - August 20, 2000               ///
///          Version 2.0. Added to EXTRA - July 17, 2001             ///

/* displays the Karnaugh Map of a function */
EXTERN void Extra_PrintKMap ARGS((FILE * pFile, DdManager * dd, DdNode * OnSet, DdNode * OffSet, int nVars, DdNode ** XVars, int fSuppType));
// If the pointer to the array of variables XVars is NULL,
// fSuppType determines how the support will be determined.
// fSuppType == 0 -- takes the first nVars of the manager
// fSuppType == 1 -- takes the topmost nVars of the manager
// fSuppType == 2 -- determines support from the on-set and the off-set
/* displays the Karnaugh Map of a relation */
EXTERN void Extra_PrintKMapRelation ARGS((FILE * pFile, DdManager * dd, DdNode * OnSet, DdNode * OffSet, int nXVars, int nYVars, DdNode ** XVars, DdNode ** YVars));


/**AutomaticEnd***************************************************************/

#endif /* __EXTRA_H__ */
