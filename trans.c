/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";

/* 32 x 32: 8 x 8 blocks with the diagonal handled by loading a whole row
 * of A into registers before touching B. */
static void transpose_32(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k;
    int a0, a1, a2, a3, a4, a5, a6, a7;

    for (i = 0; i < N; i += 8) {
        for (j = 0; j < M; j += 8) {
            for (k = i; k < i + 8; k++) {
                a0 = A[k][j];
                a1 = A[k][j + 1];
                a2 = A[k][j + 2];
                a3 = A[k][j + 3];
                a4 = A[k][j + 4];
                a5 = A[k][j + 5];
                a6 = A[k][j + 6];
                a7 = A[k][j + 7];
                B[j][k] = a0;
                B[j + 1][k] = a1;
                B[j + 2][k] = a2;
                B[j + 3][k] = a3;
                B[j + 4][k] = a4;
                B[j + 5][k] = a5;
                B[j + 6][k] = a6;
                B[j + 7][k] = a7;
            }
        }
    }
}

/* 64 x 64: buffer one 8 x 8 block in a local array so each A row block and
 * each B column block is touched exactly once (the buffer lives on the
 * stack, whose accesses the grader drops). */
static void transpose_64(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k, l;
    int buf[8][8];

    for (i = 0; i < N; i += 8) {
        for (j = 0; j < M; j += 8) {
            for (k = 0; k < 8; k++)
                for (l = 0; l < 8; l++)
                    buf[k][l] = A[i + k][j + l];

            for (l = 0; l < 8; l++)
                for (k = 0; k < 8; k++)
                    B[j + l][i + k] = buf[k][l];
        }
    }
}

/* Generic case (61 x 67 and any other size): 16 x 16 blocking with a local
 * buffer so A is read row-major and B is written row-major. */
static void transpose_generic(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, k, l;
    const int BS = 16;
    int buf[16][16];

    for (i = 0; i < N; i += BS) {
        for (j = 0; j < M; j += BS) {
            for (k = i; k < N && k < i + BS; k++)
                for (l = j; l < M && l < j + BS; l++)
                    buf[k - i][l - j] = A[k][l];

            for (l = j; l < M && l < j + BS; l++)
                for (k = i; k < N && k < i + BS; k++)
                    B[l][k] = buf[k - i][l - j];
        }
    }
}

void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (N == 32 && M == 32) {
        transpose_32(M, N, A, B);
    } else if (N == 64 && M == 64) {
        transpose_64(M, N, A, B);
    } else {
        transpose_generic(M, N, A, B);
    }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);

}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}
