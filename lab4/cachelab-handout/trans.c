

/*
*trans.c - Matrix transpose B = A^T
*
* Each transpose function must have a prototype of the form:
*void trans(int M, int N, int A[N][M], int B[M][N]);
- 8 -
*
* A transpose function is evaluated by counting the number of misses
*on a 1KB direct mapped cache with a block size of 32 bytes.
*/
#include "cachelab.h"
#include <stdio.h>
    int
    is_transpose(int M, int N, int A[N][M], int B[M][N]);
/*
*transpose_submit - This is the solution transpose function that you
*will be graded on for Part B of the assignment. Do not change
*the description string "Transpose submission", as the driver
*searches for that string to identify the transpose function to
*be graded.
*/
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
    if ((M == 32 && N == 32) || (M == 61 && N == 67)) {
        int i, j, I, J;
        for (I = 0; I < N; I += 8) {
            for (J = 0; J < M; J += 8) {
                for (i = I; i < I + 8 && i < N; ++i) {
                    for (j = J; j < J + 8 && j < M; ++j) { //先转置不会发生冲突的部分，再转置会冲突的部分，避免冲突不命中
                        if (j < M && ((j * N + i) & 0xf8) != ((i * M + j) & 0xf8))
                            B[j][i] = A[i][j];
                    }
                    for (j = M < J + 8 ? M - 1 : J + 7; j >= J; --j) {
                        if (j < M && ((j * N + i) & 0xf8) == ((i * M + j) & 0xf8))
                            B[j][i] = A[i][j];
                    }
                }
            }
        }
    }
    if (M == 64 && N == 64) {
        for (int i = 0; i < N; i += 8) {
            for (int j = 0; j < M; j += 8) {
                for (int k = i; k < i + 4; k++) { //A->Q,B->W
                    int t0 = A[k][j];
                    int t1 = A[k][j + 1];
                    int t2 = A[k][j + 2];
                    int t3 = A[k][j + 3];
                    int t4 = A[k][j + 4];
                    int t5 = A[k][j + 5];
                    int t6 = A[k][j + 6];
                    int t7 = A[k][j + 7];
                    B[j][k] = t0;
                    B[j + 1][k] = t1;
                    B[j + 2][k] = t2;
                    B[j + 3][k] = t3;
                    B[j][k + 4] = t4;
                    B[j + 1][k + 4] = t5;
                    B[j + 2][k + 4] = t6;
                    B[j + 3][k + 4] = t7;
                }
                for (int k = j; k < j + 4; k++) { //C->W,原W->E,
                    int t0 = A[i + 4][k];
                    int t1 = A[i + 5][k];
                    int t2 = A[i + 6][k];
                    int t3 = A[i + 7][k];
                    int t4 = B[k][i + 4];
                    int t5 = B[k][i + 5];
                    int t6 = B[k][i + 6];
                    int t7 = B[k][i + 7];
                    B[k][i + 4] = t0;
                    B[k][i + 5] = t1;
                    B[k][i + 6] = t2;
                    B[k][i + 7] = t3;
                    B[k + 4][i] = t4;
                    B[k + 4][i + 1] = t5;
                    B[k + 4][i + 2] = t6;
                    B[k + 4][i + 3] = t7;
                }
                for (int k = i; k < i + 4; k++) { //D->R
                    int t1 = A[k + 4][j + 4];
                    int t2 = A[k + 4][j + 5];
                    int t3 = A[k + 4][j + 6];
                    int t4 = A[k + 4][j + 7];
                    B[j + 4][k + 4] = t1;
                    B[j + 5][k + 4] = t2;
                    B[j + 6][k + 4] = t3;
                    B[j + 7][k + 4] = t4;
                }
            }
        }
    }
}
/*
*You can define additional transpose functions below. We've defined
*a simple one below to help you get started.
*/

    /*
* trans - A simple baseline transpose function, not optimized for the cache.
*/
    char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N]) {
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
* functions with the driver. At runtime, the driver will
* evaluate each of the registered functions and summarize their
* performance. This is a handy way to experiment with different
* transpose strategies.
*/
void registerFunctions() {
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);
    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
}
/*
* is_transpose - This helper function checks if B is the transpose of
* A. You can check the correctness of your transpose by calling
* it before returning from the transpose function.
*/
int is_transpose(int M, int N, int A[N][M], int B[M][N]) {
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