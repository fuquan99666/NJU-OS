#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <regex.h>
#include <time.h>
#include <string.h>

// extern char **environ;

int top_k = 5;

typedef struct syscall_record {
    char name[64];
    double time;
} syscall_record;

// the regex pattern to match the system call line in strace output 
static const char *re = "([a-zA-Z_][a-zA-Z0-9_]*)\\(.*\\) += +(-?[0-9]+) +<([0-9.]+)>";


/*

our target : sperf COMMAND [ARG]... 
COMMAND is the command we want to test,
ARG is the arguments we want to pass to the command.

We will print the time cost of the system calls which the command
will call .

if the command is so fast , you can just print once .
but if the command is slow, maybe 1 times per second is enough
Well, if the command is blocked by read or ... so that there is no system calls, 
you can wait until the system call is done !

Some constraints:
1. At least top 5 system calls per update.
2. Follow the format of printf("(%d%%)", ratio) .

Good steps to implement it:
1. parse the command and arguments from argv
2. use fork() to create a child process 
3. In child process, we use execve() to run "strace COMMAND [ARG]"
4. While 3, parent process will try to read the output of strace
   and parse the system calls and their time cost.

*/


// For execve() , the envp passed in is used by 
// the command argv[0], execve() itself doesn't use it,
// so strace bad, /bin/strace or /usr/bin/strace is ok.
// and to use ls in strace, we need to pass /bin as envp 

//int main(int argc, char *argv[]) {
  //char *exec_argv[] = { "strace", "ls", NULL, };
  //char *exec_envp[] = { "PATH=/bin", NULL, };
  //// 第一种写法：File not exist ...
  ////execve("strace",          exec_argv, exec_envp);
  //// 第二种写法：Work !
  ////execve("/bin/strace",     exec_argv, exec_envp);
  //// 第三种写法：Work !
  //execve("/usr/bin/strace", exec_argv, exec_envp);
  //perror(argv[0]);
  //exit(EXIT_FAILURE);
//}

syscall_record regex_system_call(char* buffer) {

    regex_t regex;
    regmatch_t matches[4]; // We expect 3 matches: the whole line, the syscall name, and the time cost

    // first , we should compile the regex pattern 
    if (regcomp(&regex, re, REG_EXTENDED) != 0) {
        fprintf(stderr, "Could not compile regex: %s\n", buffer);
        return (syscall_record){0};
    }

    if (regexec(&regex, buffer, 4, matches, 0) == 0) {
        // Extract syscall name
        int name_length = matches[1].rm_eo - matches[1].rm_so;
        char syscall_name[64];
        strncpy(syscall_name, buffer + matches[1].rm_so, name_length);
        syscall_name[name_length] = '\0';

        // Extract time cost
        int time_length = matches[3].rm_eo - matches[3].rm_so;
        char time_str[32];
        strncpy(time_str, buffer + matches[3].rm_so, time_length);
        time_str[time_length] = '\0';
        double time_cost = atof(time_str);

        syscall_record record = {.name = "", .time = time_cost};
        strncpy(record.name, syscall_name, sizeof(record.name) - 1);
        record.name[sizeof(record.name) - 1] = '\0';

        return record;
        // printf("System call: %s, Time cost: %f seconds\n", syscall_name, time_cost);
    } else {
        // printf("No match for line\n");
    }

    regfree(&regex);
    return (syscall_record){0};
}

void read_and_parse_strace_output() {
    char buffer[1024];

    syscall_record records[top_k];
    // init syscall_record
    for (int i = 0; i < top_k; i++) {
        records[i].name[0] = '\0';
        records[i].time = 0.0;
    }


    time_t start_time = time(NULL);
    int record_count = 0;

    while (fgets(buffer, sizeof(buffer), stdin)) {
        // Parse the output of strace here
        // For example, you can look for lines that contain system calls and their time cost
        // and then print the top 5 system calls per update.
        // printf("%s", buffer);
        // 我们每隔1s打印一次top 5的系统调用和它们的时间消耗
        time_t current_time = time(NULL);
        // printf("Current time: %d, Start time: %d\n", current_time, start_time);
        if (current_time - start_time >= 1) {
            // Print top 5 system calls 
            record_count ++;
            start_time = current_time;
            printf("Time: %d\n", record_count);
            // printf("Buffer: %s\n", buffer);

            double total_time = 0.0;
            for (int i = 0; i < top_k; i++) {
                total_time += records[i].time;
            }
            for (int i = 0; i < top_k; i++) {
                double ratio = (total_time > 0) ? (records[i].time / total_time) * 100.0 : 0.0;
                printf("%s (%.2f%%) \n", records[i].name, ratio);
            }

            // reset the records for the next second
            for (int i = 0; i < top_k; i++) {
                records[i].name[0] = '\0';
                records[i].time = 0.0;
            }
        }

        // update the records array with the new system call and its time cost 
        // we can use regex to judge if the line is a system call line or not 
        syscall_record new_call = regex_system_call(buffer);
        if (new_call.time > 0) {
            // loop the current records array to judge
            int min_index = -1;
            int min_time = 999;
            int flag = 0;
            for (int i = 0; i < top_k; i++) {
                // no record yet
                if (records[i].time == 0) {
                    records[i] = (struct syscall_record){.name = "", .time = new_call.time};
                    strncpy(records[i].name, new_call.name, sizeof(records[i].name) - 1);
                    records[i].name[sizeof(records[i].name) - 1] = '\0';
                    flag = 1;
                    break;
                } else if (strcmp(records[i].name, new_call.name) == 0) {
                    // found the same system call, update its time cost
                    records[i].time += new_call.time;
                    flag = 1;
                    break;
                } else {
                    // find the minimum time cost record
                    if (records[i].time < min_time) {
                        min_time = records[i].time;
                        min_index = i;
                    }
                }
            }

            if (flag == 0) {
                // if the new system call's time cost is greater than the minimum time cost record, replace it
                if (new_call.time > min_time && min_index != -1) {
                    records[min_index] = (struct syscall_record){.name = "", .time = new_call.time};
                    strncpy(records[min_index].name, new_call.name, sizeof(records[min_index].name) - 1);
                    records[min_index].name[sizeof(records[min_index].name) - 1] = '\0';
                }
            }
        }

    }
}

int main(int argc, char *argv[]) {
    for (int i = 0; i < argc; i++) {
        assert(argv[i]);
        printf("argv[%d] = %s\n", i, argv[i]);
    }
    assert(!argv[argc]);

    // Step 1: parse the command and arguments from argv
    int new_argc = argc ;
    char *new_argv[new_argc+2];
    for (int i = 0; i < new_argc; i++) {
        new_argv[i+1] = argv[i];
    }
    new_argv[0] = "strace";
    new_argv[1] = "-T";
    new_argv[new_argc+1] = "NULL";

    // Step 2: use fork() to create a child process

    // Wait, wait, wait, before creating child process
    // we should first create pipe so that we can read the 
    // output of child process in parent process ...

    int fildes[2];

    if (pipe(fildes) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }


    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process 
        // Step 3: run execve() to run "strace COMMAND [ARG]"

        // we need to redirect stderr to the pipe's write (strace output is printed to stderr)
        // use dup2 

        // here still a problem is that if the command itself has some output , that maybe influence the strace output.
        // if its output is printed to stdout, we can redirect it to /dev/null, 
        // but if its output is printed to stderr ...
        dup2(fildes[1], STDERR_FILENO);

        int null_fd = open("/dev/null", O_WRONLY);
        if (null_fd == -1) {
            perror("open /dev/null");
            exit(EXIT_FAILURE);
        }
        dup2(null_fd, STDOUT_FILENO);

        close(fildes[0]); // close the read end of the pipe in child
        close(fildes[1]); // close the write end of the pipe in child (after dup2)

        // get the envp 
        char *exec_envp[] = { "PATH=/usr/bin", NULL, };
        execve("/usr/bin/strace", new_argv, exec_envp);


        // the child process will be arranged for strace
        // and running it until over ...
    } else {
        // parent process 
        // Step 4: read the output of strace and parse the system calls and their time cost 

        // redirect the pipe's read to stdin 
        dup2(fildes[0], STDIN_FILENO);
        close(fildes[0]); // close the read end of the pipe in parent (after dup2)
        close(fildes[1]); // close the write end of the pipe in parent

        // start to read and parse the output of child process's strace ...
        read_and_parse_strace_output();

    }

    return 0;
}

