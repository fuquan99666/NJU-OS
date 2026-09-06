#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dlfcn.h>

/*

In this lab, we will try to implement a simple unix shell ,
which can accept expression or a function (both return type is int), and 
if the function is first defined, we just store it in memory, or we just 
evaluate the expression or function call and print the result.

Notice:
1. You can't call any glibc function, all function called is defined by input .
2. Make sure not crash , even some input is not valid, for example, "1 + 2 *" or some wrong function ... Just give some error message .

Steps:
0. the start code already provides a basic loop of crepl.
1. parse the input (expression or function), if the head is int, it is a function define or it is a expression you need to evaluate.
2. For a function define, we can compile it to a shared object file, and then load it to memory, so that we can call it when later expression include it .
   And we can just save the shared object file to /tmp (which is safe and easy). When we call the object file, we need to get its return value, 
   in this lab, we can't use system or popen (which simplifies the problem), but we can use the method of pipe + fork + exec to get the return value of the child process ...



Insights:
编译和解释并没有明确的边界...
Actually, this lab is a C JIT (just in time) compiler, which is a kind of compiler that can compile code at runtime and execute it immediately.
*/

// maybe we can use a data structure to store the function name and its address
typedef struct Function {
    char name[256];
} Function;

Function functions[256];
int function_count = 0;
int expression_count = 0; // used for wrapper name 


void define_function(const char *line) {
    // compile the function to a shared object file and save to /tmp 
    // Well, you may think "??? I seem not to learn a C programmar to compile a C function in dynamic way ..."
    // hhh, don't forget that we have a system call execve, which can execute a program in a new process !
    
    // And this lab let us use pipe + fork + exec ...
    // And we must create a child process to do function call because if we execve in the main process, crepl is gone ...
    // at last, we return "/tmp/func_123.so" to the parent process through pipe ...

    char template_c[] = "/tmp/func_XXXXXX.c";
    int fd_c = mkstemps(template_c, 2);
    if (fd_c == -1) {
        perror("mkstemps");
        exit(EXIT_FAILURE);
    }

    // write the function to the temporary C file
    write(fd_c, line, strlen(line));
    close(fd_c);


    // use mkstemp to create a unique temporary file name in /tmp, and then use it to save the shared object file.
    char template[] = "/tmp/func_XXXXXX.so";
    int fd = mkstemps(template, 3); // 3 is the length of ".so" as suffix
    if (fd == -1) {
        perror("mkstemps");
        exit(EXIT_FAILURE);
    }

    close(fd); // we don't need the file descriptor, just the name of the file

    // fork again ...
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {

        char *argv[] = { "gcc", "-shared", "-fPIC", "-Wno-implicit-function-declaration","-Wl,--unresolved-symbols=ignore-all" ,"-o", template, template_c, NULL };
        execvp("gcc", argv);
    } else {
        // the son son process is compiling the function
        // the son process just need to wait
        int status;
        // use wait

        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

        // first get the name of function from the line
        char func_name[256];
        sscanf(line, "int %[a-zA-Z0-9_]", func_name);
        printf("%s\n", func_name);

        // so now the sonson process is done
        printf("%s", template);
        exit(0);
    }
}


void register_function_from_pipe(int pipe_fd) {
    // read the func_name and template name from the pipe
    char func_name[256];
    char template[256];

    FILE *pipe_stream = fdopen(pipe_fd, "r");
    if (!pipe_stream) {
        perror("fdopen");
        exit(EXIT_FAILURE);
    }

    if (!fgets(func_name, sizeof(func_name), pipe_stream)) {
        perror("fgets");
        exit(EXIT_FAILURE);
    }

    // fgets will read the newline character, so we need to remove it
    func_name[strcspn(func_name, "\n")] = 0;

    if (!fgets(template, sizeof(template), pipe_stream)) {
        perror("fgets");
        exit(EXIT_FAILURE);
    }

    fclose(pipe_stream);

    // register the function from the shared object file using dlopen and dlsym
    void *handle = dlopen(template, RTLD_GLOBAL | RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    dlerror(); // Clear any existing error since last dlopen call ...

    // store the function in our function array
    if (function_count < 256) {
        strncpy(functions[function_count].name, func_name, sizeof(functions[function_count].name));
        function_count++;
    } else {
        fprintf(stderr, "Function array is full!\n");
        exit(EXIT_FAILURE);
    }

    char *error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "%s\n", error);
        exit(EXIT_FAILURE);
    }

    printf("Register function[%s] from [%s]\n", func_name, template);
}


int evaluate_expression(char *line) {
    // for a expression, of course we can't directly run it like python
    // and I don't want to implement a C interpreter ...
    // so we just use a wrapper to make it like a function, and then compile it to a shared object file 
    // so the method is like the define_function ...
    // for example "int __expr_wrapper_x()" , which x is a unique number(each expression has one)

    char wrapper_name[256];
    snprintf(wrapper_name, sizeof(wrapper_name), "__expr_wrapper_%d", expression_count++);


    // then append "{ return <line>; }"
    char wrapper_code[8192];

    // before append the expression, add all functiion declaration （actually you can only add 
    // the function used in the expression, I am lazy...

    // Declare is necessary, compiler will warn it .
    // But you can not define that function, which the dynamic linker will find it in global symbol table when you call it.

    char declarations[4096] = "";
    for (int i = 0; i < function_count; i++) {
        strcat(declarations, "int ");
        strcat(declarations, functions[i].name);
        strcat(declarations, "(...);\n"); // 偷个懒，不想去解析函数参数了，直接用省略号
    }

    snprintf(wrapper_code, sizeof(wrapper_code),
        "%s\n"
        "int %s() { return %s; }",
        declarations, wrapper_name, line
    );

    // then we can use the same method as define_function to compile it to a shared object file and load it to memory
    // and then we can call the function and get the return value

    // create a temporary C file to save the wrapper code
    char template_c[] = "/tmp/expr_XXXXXX.c";
    int fd_c = mkstemps(template_c, 2);
    if (fd_c == -1) {
        perror("mkstemps");
        exit(EXIT_FAILURE);
    }

    // write the wrapper code to the temporary C file
    // printf("wrapper_code: %s\n", wrapper_code);
    write(fd_c, wrapper_code, strlen(wrapper_code));
    close(fd_c);

    // create a temporary shared object file to save the compiled code
    char template_so[] = "/tmp/expr_XXXXXX.so";
    int fd_so = mkstemps(template_so, 3);
    if (fd_so == -1) {
        perror("mkstemps");
        exit(EXIT_FAILURE);
    }

    close(fd_so);

    // fork a child process to compile the wrapper code
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // child process
        char *argv[] = { "gcc", "-shared", "-fPIC", "-Wl,--unresolved-symbols=ignore-all", "-o", template_so, template_c, NULL };
        execvp("gcc", argv);
    } else {
        // parent process
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
        // now the wrapper code is compiled to a shared object file, we can load it to memory and call it
        void *handle = dlopen(template_so, RTLD_LAZY | RTLD_GLOBAL);
        if (!handle) {
            fprintf(stderr, "dlopen failed: %s\n", dlerror());
            exit(EXIT_FAILURE);
        }

        dlerror(); // Clear any existing error since last dlopen call ...
        __attribute__((unused)) typeof(int (void) )* func;
        func = (typeof(int (void) )*)dlsym(handle, wrapper_name);

        // just call it 
        int result = func();
        return result;
    }
    return 0; // should not reach here
}

int main(int argc, char *argv[]) {
    static char line[4096];

    while (1) {
        printf("crepl> ");
        fflush(stdout);
        // Notice that our printf is line buffer, so because printf("crepl> ") doesn't have \n, it will not flush the buffer
        // so if we add sleep(1) here, you can notice a delay (1s)
        // 神奇的是fget 函数会好心flush stdout, this is not a standard of C, just a defensive action of glibc ...
        // sleep(1);
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }


        // To be implemented.
        // if input is Hello\n, line is Hello\n (notice \n is also included!)
        // printf("Got %zu chars.\n", strlen(line));

        // step 1: parse the input (expression or function), if the head is int, it is a function define or it is a evaluable expression.

        int is_function = 0;
        // just compare the first 3 chars
        if (strncmp(line, "int", 3) == 0) {
            is_function = 1;
        }

        if (is_function) {
            printf("OK.\n");

            // create a pipe to get the info of child process (where we will compile the function and save .so  to /tmp)

            int fildes[2];
            if (pipe(fildes) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }

            pid_t pid = fork();
            if  (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                // child process
                dup2(fildes[1], STDOUT_FILENO); // redirect stdout to pipe' write end 
                close(fildes[0]);
                close(fildes[1]);

                // Step 2: define the function
                define_function(line);
                
            } else {
                // parent process
                // dup2(fildes[0], STDIN_FILENO); // redirect stdin to pipe's read end 

                close(fildes[1]);

                // read the output of child process from pipe , maybe "/tmp/func_123.so"
                // then use dlopen to load the shared object file and dlsym to get the function pointer, then we can call it 
                // when a expression include the function call.

                // different from the sperf.c , here we can't redirect stdin to pipe's read end,
                // because we are in a loop, and the parent process will read the next line from keyboard !!!
                // so just read the fildes[0] directly ...

                register_function_from_pipe(fildes[0]);
                close(fildes[0]);

            }


        } else {
            // Step 2: evaluate the expression 
            int result = evaluate_expression(line);
            printf("= %d\n", result);
        }
    }
}
