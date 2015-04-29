#include "sparse.h"

main(argc, argv)
char **argv;
{
    sm_matrix *A;

    if (! sm_read(fopen(argv[1], "r"), &A)) {
	fprintf(perror(argv[1]));
	exit(1);
    }
    printf("A is %d rows %d cols %d elements\n", A->nrows, A->ncols,
		sm_num_elements(A));
}
