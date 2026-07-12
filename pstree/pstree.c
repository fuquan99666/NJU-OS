#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

int main(int argc, char *argv[]) {
  // for (int i = 0; i < argc; i++) {
  //   assert(argv[i]);
  //   printf("argv[%d] = %s\n", i, argv[i]);
  // }
  // assert(!argv[argc]);

  // Step 1: Get parameters from command line and set flag variables accordingly

  /*
      -p or --show-pids: Show PIDs for each process in the tree
      -n or --numeric-sort: Sort child processes of one parent by PID instead of name 
      -V or --version: version information 
  */

  // 对于参数解析，你可以自己实现一个简单的解析器，或者使用现成的 getopt(3) and getopt_long(3) 来处理

  int opt;
  int show_pids = 0;
  int numeric_sort = 0;
  int version_info = 0;

  static struct option long_option[] = {
    {"show-pids", no_argument, 0, 'p'},
    {"numeric-sort", no_argument, 0, 'n'},
    {"version", no_argument, 0, 'V'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
  };

  int long_index = 0; // here because we just return short option for long options , so it seems not necessary .
  
  while ((opt = getopt_long(argc, argv, "pnVh", long_option, &long_index)) != -1) {
    switch (opt)
    {
    case 'p':
      show_pids = 1;
      break;
    case 'n':
      numeric_sort = 1;
      break;
    case 'V':
      version_info = 1;
      break;
    case 'h':
      printf("Usage: %s [options]\n", argv[0]);
      printf("Options:\n");
      printf("  -p, --show-pids       Show PIDs for each process in the tree\n");
      printf("  -n, --numeric-sort    Sort child processes of one parent by PID instead of name\n");
      printf("  -V, --version         Show version information\n");
      printf("  -h, --help            Show this help message\n");
      exit(0);
    default:
      printf("You may have provided an invalid option.\n");
      printf("Please use -h or --help for usage information.\n");
      exit(1);
    }
  }

  printf("show_pids: %d, numeric_sort: %d, version_info: %d\n", show_pids, numeric_sort, version_info);

  // Step 2: Get all processes PIDs from /proc and store them in a list 


  // Step 3: Assign numbers for each one in the list and get their parent PIDs from /proc/[pid]/stat 

  // Step 4: Build a tree structure of based on the list and sort by parameters 

  // Step 5: Print the tree structure in a readable format 

}
