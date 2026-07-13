#define _GNU_SOURCE // d_type needs __USE_MISC to be defined, and __USE_MISC is defined when _GNU_SOURCE is defined
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include "tree.h"

#define MAX_PROCESSES 2048

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

  if (version_info) {
    fprintf(stderr, "pstree (fuquan99) 1.0 !\n");
    fprintf(stderr, "This is a simple implementation of pstree command in Linux.\n");
    exit(EXIT_SUCCESS);
  }

  // printf("show_pids: %d, numeric_sort: %d, version_info: %d\n", show_pids, numeric_sort, version_info);

  // Step 2: Get all processes PIDs from /proc and store them in a list 

  // loop through /proc/[pid] directories and collect PIDs into a list


  struct process_info process_list[MAX_PROCESSES]; // assuming max 2048 processes for simplicity

  // initialize the list 
  for (int i = 0; i < MAX_PROCESSES; i++) {
    process_list[i].pid = -1; // mark as unused
    process_list[i].ppid = -1; // mark as unused
  }

  DIR *dir = opendir("/proc"); 

  if (dir == NULL) {
    perror("opendir");
    exit(EXIT_FAILURE);
  }

  struct dirent *entry;

  int num = 0;

  while ((entry = readdir(dir)) != NULL) {
    // here we only want directories that are numbers (PIDs)
    if (entry->d_type == DT_DIR) {
      // check if the directory name is a number 
      char *endptr;
      long pid = strtol(entry->d_name, &endptr, 10);

      if (*endptr == '\0') { // "hello" : *endptr = "h", "123" : *endptr = '\0'
        // store the PID in the list
        if (num >= MAX_PROCESSES) {
          fprintf(stderr, "Too many processes, skipping PID %ld\n", pid);
          continue;
        } 
        process_list[num].pid = pid;
        num ++;
      }
    }
  }

  // for (int i = 0; i < MAX_PROCESSES; i++) {
  //   if (process_list[i].pid != -1) {
  //     printf("process_list[%d].pid = %d\n", i, process_list[i].pid);
  //   }
  // }

  // Step 3: Assign numbers for each one in the list and get their parent PIDs from /proc/[pid]/stat 

  // Here we may not assign pid for each process, because some process's pid may be very large 
  // So we just use the index in the list as the number for each process, and normally it's ascending order.

  for (int i = 0; i < num; i++) {
    char stat_path[256];
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/status", process_list[i].pid);

    FILE *stat_file = fopen(stat_path, "r");
    if (stat_file == NULL) {
      perror("fopen");
      fprintf(stderr, "Failed to open %s\n", stat_path);
      continue;
    }

    // read the stat file and get the ppid and command name
    char line[256];
    int find_ppid = 0;
    int find_command = 0;
    while (fgets(line, sizeof(line), stat_file) != NULL) {
      // parse the line to get the ppid and command name 
      // \t is a tab , used for format the text in the file.

      if (find_ppid && find_command) {
        break;
      }

      if (strncmp(line, "PPid:", 5) == 0) {
        sscanf(line, "PPid:\t%d", &process_list[i].ppid);
        find_ppid = 1;
      } else if (strncmp(line, "Name:", 5) == 0) {
        sscanf(line, "Name:\t%255s", process_list[i].command);
        find_command = 1;
      }
    }
    fclose(stat_file);

  }

  // for (int i = 0; i < num; i++) {
  //   printf("process_list[%d]: pid = %d, ppid = %d, command = %s\n", i, process_list[i].pid, process_list[i].ppid, process_list[i].command);
  // }

  // Step 4: Build a tree structure of based on the list and sort by parameters 

  // the root of the tree 
  struct tree root = { .pid = 0, .ppid = -1, .command = "root", .num_children = 0, .parent = NULL };

  create_tree(&root, process_list, num, numeric_sort);

  // Step 5: Print the tree structure in a readable format 

  print_tree(&root, show_pids, 0);
}
