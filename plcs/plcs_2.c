#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "thread.h"
#include "thread-sync.h"
#include <time.h>

#define MAXN 40000
#define MAXT 64

int T, N, M;
char A[MAXN + 1], B[MAXN + 1];
char *sa, *sb;

int dp[MAXN][MAXN];
int result;

sem_t start_sem[MAXT];
sem_t done_sem;

#define DP(x, y) (((x) >= 0 && (y) >= 0) ? dp[x][y] : 0)
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MAX3(x, y, z) MAX(MAX(x, y), z)

typedef struct subtask_t {
  int pos_a;
  int pos_b;
  int num;
} subtask;

subtask subtask_queue[MAXT];


void Tworker(int id) {
  int idx = id - 1;

  while (1) {
    P(&start_sem[idx]);

    if (subtask_queue[idx].num < 0) {
      break;
    }

    int pos_a = subtask_queue[idx].pos_a;
    int pos_b = subtask_queue[idx].pos_b;
    int num = subtask_queue[idx].num;

    for (int i = 0; i < num; i++) {
      int a = pos_a + i;
      int b = pos_b - i;

      int skip_a = DP(a -1, b);
      int skip_b = DP(a, b - 1);
      int take_both = DP(a - 1, b - 1) + (sa[a] == sb[b]);
      dp[a][b] = MAX3(skip_a, skip_b, take_both);
    }

    V(&done_sem);
  }
}

int count_tasks(int iter_index) {
  if (iter_index < M) {
    return iter_index + 1;
  } else if (iter_index < N) {
    return M;
  } else {
    return (N + M - 1) - iter_index;
  }
}

void assign_tasks(int total_tasks, int iteration) {
  int average, remainder;
  // based on the total tasks, we use different threads
  average = total_tasks / T;
  remainder = total_tasks % T;
  int pos_a, pos_b;

  if (iteration < M) {
    pos_a = 0;
    pos_b = iteration;
  } else {
    pos_a = iteration - M + 1;
    pos_b = M - 1;
  }

  for (int i = 0; i < T; i++) {
    int subtask_num = average + (i < remainder ? 1 : 0);
    subtask_queue[i].pos_a = pos_a;
    subtask_queue[i].pos_b = pos_b;
    subtask_queue[i].num = subtask_num;
    pos_a += subtask_num;
    pos_b -= subtask_num;
  }
}

int main(int argc, char *argv[]) {
  assert(scanf("%s%s", A, B) == 2);
  N = strlen(A);
  M = strlen(B);

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  int temp;
  if (N < M) {
    sa = B;
    sb = A;
    temp = M;
    M = N;
    N = temp;
  } else {
    sa = A;
    sb = B;
  }

  T = !argv[1] ? 1 : atoi(argv[1]);
  if (T > MAXT) T = MAXT;
  if (T < 1) T = 1;

  printf("Using %d threads\n", T);
  printf("Length of sa: %d, Length of sb: %d\n", N, M);

  SEM_INIT(&done_sem, 0);
  for (int i = 0; i < T; i++) {
    SEM_INIT(&start_sem[i], 0);
    subtask_queue[i].num = 0;
    create(Tworker);
  }


    for (int iteration = 0; iteration < N + M - 1; iteration++) {
        int total_tasks = count_tasks(iteration);
        assign_tasks(total_tasks, iteration);

        for (int i = 0; i < T; i++) {
        V(&start_sem[i]);
        }

        for (int i = 0; i < T; i++) {
        P(&done_sem);
        }
    }

  for (int i = 0; i < T; i++) {
    subtask_queue[i].num = -1;
    V(&start_sem[i]);
  }

  join();

  clock_gettime(CLOCK_MONOTONIC, &end);
  double time_spent = (double)(end.tv_sec - start.tv_sec) +
                      (double)(end.tv_nsec - start.tv_nsec) / 1e9;

  result = dp[N - 1][M - 1];
  printf("%d\n", result);
  printf("Time spent: %f seconds\n", time_spent);

  return 0;
}