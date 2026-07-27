#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "thread.h"
#include "thread-sync.h"
#include <time.h>

#define MAXN 60000
#define MAXT 64

int T, N, M;
char A[MAXN + 1], B[MAXN + 1];
char *sa, *sb;

// 对角线存储：diag[d][i] 表示第d条对角线的第i个元素
int *diag[MAXN * 2];
int diag_len[MAXN * 2];

int result;

sem_t start_sem[MAXT];
sem_t done_sem;

typedef struct {
  int diag_idx;   // 处理哪条对角线
  int start;      // 在对角线上的起始位置
  int count;      // 处理多少个元素
  int a;
  int b;
} subtask;

subtask subtask_queue[MAXT];

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MAX3(x, y, z) MAX(MAX(x, y), z)

void Tworker(int id) {
  int idx = id - 1;

  while (1) {
    P(&start_sem[idx]);

    if (subtask_queue[idx].count < 0) break;

    int d = subtask_queue[idx].diag_idx;
    int start = subtask_queue[idx].start;
    int count = subtask_queue[idx].count;
    
    int *cur = diag[d];
    int *prev = (d > 0) ? diag[d-1] : NULL;
    int *prev2 = (d > 1) ? diag[d-2] : NULL;
    
    int pos_a = subtask_queue[idx].a;
    int pos_b = subtask_queue[idx].b;

    for (int i = 0; i < count; i++) {
      int pos = start + i;

      int a = pos_a + i;
      int b = pos_b - i;
      
      // 三个依赖值
      int up = 0, left = 0, diag_val = 0;

      if (d < M) {
        up = a - 1 >= 0 ? prev[pos - 1] : 0;
      } else {
        up = prev[pos];
      }

      if (d < M) {
        left = b > 0 ? prev[pos] : 0;
        diag_val = a - 1 >= 0 && b > 0 ? prev2[pos - 1] : 0;
      } else {
        left = b > 0 ? prev[pos + 1] : 0;
        if (d == M) {
            diag_val = a - 1 >= 0 && b > 0 ? prev2[pos] : 0;
        } else{
            diag_val = b > 0 ? prev2[pos + 1] : 0;
        }
      }

      cur[pos] = MAX3(up, left, diag_val + (sa[a] == sb[b]));
    }

    V(&done_sem);
  }
}

int main(int argc, char *argv[]) {
  assert(scanf("%s%s", A, B) == 2);
  N = strlen(A);
  M = strlen(B);

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  // 保证 sa 是长的那个
  if (N < M) {
    sa = B; sb = A;
    int tmp = M; M = N; N = tmp;
  } else {
    sa = A; sb = B;
  }

  T = !argv[1] ? 1 : atoi(argv[1]);
  if (T > MAXT) T = MAXT;
  if (T < 1) T = 1;

  printf("Using %d threads, N=%d, M=%d\n", T, N, M);

  // 分配对角线存储
  int total_diags = N + M - 1;
  for (int d = 0; d < total_diags; d++) {
    if (d < M) diag_len[d] = d + 1;
    else if (d < N) diag_len[d] = M;
    else diag_len[d] = N + M - 1 - d;
    
    diag[d] = (int *)calloc(diag_len[d], sizeof(int));
  }

  SEM_INIT(&done_sem, 0);
  for (int i = 0; i < T; i++) {
    SEM_INIT(&start_sem[i], 0);
    create(Tworker);
  }

  // 逐对角线并行计算
  for (int d = 0; d < total_diags; d++) {
    int len = diag_len[d];
    int per = len / T;
    int rem = len % T;
    int pos = 0;

    int pos_a, pos_b;

    if (d < M) {
        pos_a = 0;
        pos_b = d;
    } else {
        pos_a = d - M + 1;
        pos_b = M - 1;
    }

    int active_threads = 0;
    for (int t = 0; t < T; t++) {
      int cnt = per + (t < rem ? 1 : 0);
      if (cnt > 0) {
        subtask_queue[active_threads].diag_idx = d;
        subtask_queue[active_threads].start = pos;
        subtask_queue[active_threads].count = cnt;
        subtask_queue[active_threads].a = pos_a;
        subtask_queue[active_threads].b = pos_b;
        pos += cnt;
        pos_a += cnt;
        pos_b -= cnt;
        V(&start_sem[active_threads]);
        active_threads++;
      }
    }

    for (int t = 0; t < active_threads; t++) {
      P(&done_sem);
    }
  }

  // 终止线程
  for (int t = 0; t < T; t++) {
    subtask_queue[t].count = -1;
    V(&start_sem[t]);
  }

  join();

  clock_gettime(CLOCK_MONOTONIC, &end);
  double elapsed = (end.tv_sec - start.tv_sec) + 
                   (end.tv_nsec - start.tv_nsec) / 1e9;

  result = diag[total_diags - 1][0];
  printf("%d\n", result);
  printf("Time: %.4f s\n", elapsed);

  for (int d = 0; d < total_diags; d++) free(diag[d]);
  return 0;
}
