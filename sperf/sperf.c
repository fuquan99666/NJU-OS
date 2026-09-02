#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

// extern char **environ;

/*

our target : sperf COMMAND [ARG]... 
COMMAND is the command we want to test,
ARG is the arguments we want to pass to the command.

We will print the time cost of the system calls which the command
will call .

if the command is so fast , you can just print once .
but if the command is slow, maybe 10 times per second is enough
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

void read_and_parse_strace_output() {
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), stdin)) {
        // Parse the output of strace here
        // For example, you can look for lines that contain system calls and their time cost
        // and then print the top 5 system calls per update.
        printf("%s", buffer);
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
    char *new_argv[new_argc+1];
    for (int i = 0; i < new_argc; i++) {
        new_argv[i] = argv[i];
    }
    new_argv[0] = "strace";
    new_argv[new_argc] = NULL;

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

