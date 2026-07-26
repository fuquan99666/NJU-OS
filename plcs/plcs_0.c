// A easy implementation of LCS problem, one thread to compute the result.

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>

#define MAXN 60000
int N, M;
char A[MAXN + 1], B[MAXN + 1];
int dp[MAXN][MAXN];
int result;

#define DP(x, y) (((x) >= 0 && (y) >= 0) ? dp[x][y] : 0)
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MAX3(x, y, z) MAX(MAX(x, y), z)


void plcs() {

  for (int i = 0; i < N; i++) {
    for (int j = 0; j < M; j++) {
      // Always try to make DP code more readable
      int skip_a = DP(i - 1, j);
      int skip_b = DP(i, j - 1);
      int take_both = DP(i - 1, j - 1) + (A[i] == B[j]);
      dp[i][j] = MAX3(skip_a, skip_b, take_both);
    }
  }

  result = dp[N - 1][M - 1];
}


int main(int argc, char *argv[]) {
  // No need to change
  assert(scanf("%s%s", A, B) == 2);
  N = strlen(A);
  M = strlen(B);

  // Directly use one thread to compute the result 
  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  plcs();
  clock_gettime(CLOCK_MONOTONIC, &end);
  double time_spent = (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / 1e9;


  printf("%d\n", result);
  printf("Time spent: %f seconds\n", time_spent);
}