#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "thread.h"
#include "thread-sync.h"

#define MAXN 10000
#define MAXT 64
int T, N, M;
char A[MAXN + 1], B[MAXN + 1];
int dp[MAXN][MAXN];
int result;

#define DP(x, y) (((x) >= 0 && (y) >= 0) ? dp[x][y] : 0)
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MAX3(x, y, z) MAX(MAX(x, y), z)

typedef struct subtask_t
{
  int pos_a;
  int pos_b;
  int num;
} subtask;

subtask subtask_queue[MAXT];

void Tworker(int id) {
  if (id != 1) {
    // This is a serial implementation
    // Only one worker needs to be activated
    return;
  }

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

int count_tasks(int iter_index) {
  if (iter_index < M) 
    return iter_index+1;
  else if (iter_index < N)
    return M;
  else
    return (N+M-1)-iter_index;
}

void assign_tasks(int total_tasks, int iteration){
  int average = total_tasks / T;
  int remider = total_tasks % T;
  int subtask_to_assign;
  int pos_a,pos_b;

  // initial position of computing DAG from left bottom
  pos_a = 0;
  if (iteration < M) {pos_b = iteration;} else {pos_b = M-1;}

  // For every thread: set the position and tasks number
  // ！！！caution the atomicity of tasks number, and set the number at last
  for (int i = 0; i < T; i++) {
    // compute subtasks number
    subtask_to_assign = average;
    if (i < remider) subtask_to_assign += 1;

    // set position
    subtask_queue[i].pos_a = pos_a;
    subtask_queue[i].pos_b = pos_b;
    pos_a += subtask_to_assign;
    pos_b -= subtask_to_assign;

    // set subtasks number at last
    subtask_queue[i].num = subtask_to_assign;
  }
}

int main(int argc, char *argv[]) {
  // No need to change
  assert(scanf("%s%s", A, B) == 2);
  N = strlen(A);
  M = strlen(B);

  char *sa, *sb;
  int temp;

  // make len(A)>=len(B)
  if (N < M) {
    sa = B; sb = A;
    temp = M; M = N; N = temp;
  }
  else {
    sa = A; sb = B;
  }

  T = !argv[1] ? 1 : atoi(argv[1]);

  // Add preprocessing code here
  // initial subtask queue && create Tworker threads
  for (int i = 0; i < T; i++) {
    subtask_queue[i] = (subtask){
      .num = 0
    };
    create(Tworker);
  }

  int total_tasks;
  for (int iteration = 0; iteration < N+M-1; iteration++) {
    // compute total tasks and assign them
    total_tasks = count_tasks(iteration);
    assign_tasks(total_tasks, iteration);

    // start iteration

    // complete iteration and aggragation
  }

  join();  // Wait for all workers

  printf("%d\n", result);
}
