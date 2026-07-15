#include "tree.h"


int compare_by_pid(const void *a, const void *b) {
    struct tree *tree_a = *(struct tree **)a;
    struct tree *tree_b = *(struct tree **)b;

    return (tree_a->pid - tree_b->pid);
};

int compare_by_command(const void *a, const void *b) {
    struct tree *tree_a = *(struct tree **)a;
    struct tree *tree_b = *(struct tree **)b;

    return strcmp(tree_a->command, tree_b->command);
};


void create_tree(struct tree *root, struct process_info *process_list, int num_processes, int numeric_sort) {
    for (int i = 0; i < num_processes; i++) {
        if (process_list[i].ppid == root->pid) {

            if (root->num_children >= MAX_CHILDREN) {
                fprintf(stderr, "Too many children for PID %d, skipping PID %d\n", root->pid, process_list[i].pid);
                continue;
            }

            // create a new tree node for this process and add it to the root's children
            struct tree *child = malloc(sizeof(struct tree));
            
            if (child == NULL) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }

            child->pid = process_list[i].pid;
            child->ppid = process_list[i].ppid;
            child->parent = root;

            strncpy(child->command, process_list[i].command, sizeof(child->command));
            child->num_children = 0;

            root->childrens[root->num_children] = child;
            root->num_children++;

            create_tree(child, process_list, num_processes, numeric_sort);
        }

    }

    // sort the children of this node by PID or command name 
    if (numeric_sort) {
        // sort by PID

        qsort(root->childrens, root->num_children, sizeof(struct tree *), compare_by_pid);

    } else {
        // sort by command name

        qsort(root->childrens, root->num_children, sizeof(struct tree *), compare_by_command);

    }

}


// to print a good tree , we need to track the depth of the tree and some information .
void print_tree(struct tree *root, int show_pids, int col) {
    if (root == NULL) {
        return ;
    }

    // print the current node 
    
    // if pid = 0 , skip it 
    if (root->pid != 0) {
        // print the command name 
        printf("%s", root->command);

        col += strlen(root->command);

        // if show_pids is true, print the PID in ()
        if (show_pids) {
            printf("(%d)", root->pid);
            char pid_str[32];
            int len = snprintf(pid_str, sizeof(pid_str), "(%d)", root->pid);
            if (len < 0) {
                perror("snprintf");
                exit(EXIT_FAILURE);
            }
            col += len ;
        }

    }


    // print the children of this node 

    if (root->num_children == 1) {
        printf("---");
        print_tree(root->childrens[0], show_pids, col+3);
        return;
    } else if (root->num_children > 1) {
        for (int i = 0; i < root->num_children; i++) {

            if (i != 0) {
                for (int j = 0; j < col; j++) {
                    printf(" ");
                }
            }

            if (i == 0) {
                printf("-+-");
                print_tree(root->childrens[i], show_pids, col + 3);
            } else if (i == root->num_children - 1) {
                printf(" `-");
                print_tree(root->childrens[i], show_pids, col + 3);
            } else {
                printf(" |-");
                print_tree(root->childrens[i], show_pids, col + 3);
            }
        }

        return; 

    } else {
        // no children 
        printf("\n");
        return;
    }

}


// here we use int *width to track the col of each parent node
// level is the num of parent node .
void print_tree_1(struct tree *root, int show_pids, int level, int col) {

    if (root == NULL) {
        return ;
    }
    
    // print the current node 
    
    // if pid = 0 , skip it 
    if (root->pid != 0) {
        // print the command name 
        printf("%s", root->command);

        col += strlen(root->command);

        // if show_pids is true, print the PID in ()
        if (show_pids) {
            printf("(%d)", root->pid);
            char pid_str[32];
            int len = snprintf(pid_str, sizeof(pid_str), "(%d)", root->pid);
            if (len < 0) {
                perror("snprintf");
                exit(EXIT_FAILURE);
            }
            col += len ;
        }

    }


    if (root->num_children == 0) {
        // no children , just a leaf
        printf("\n");

    } else {

        // add this node's col to width[level]
        if (width == NULL) {
            width = malloc(sizeof(int) * 64);
            if (width == NULL) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }
        }   

        if (last == NULL) {
            last = malloc(sizeof(int) * 64);
            if (last == NULL) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }
        }

        width[level] = col+1;
        last[level] = 0;
        level += 1;


        // loop through the children and print them 
        for (int i = 0; i < root->num_children; i++) {

            if (i == root->num_children - 1) {
                last[level-1] = 1;
            }

            if (i == 0) {
                if (root->num_children == 1) {
                    printf("---");
                } else {
                    printf("-+-");
                }
                print_tree_1(root->childrens[i], show_pids, level, col + 3);
            } else {
                // print the vertical line for the previous parent node 
                for (int j = 0; j < level; j++) {
                    int start,end;
                    if (j == 0) {
                        start = 0;
                    } else {
                        start = width[j-1]+1;
                    }
                    end = width[j];
                    for (int k = start; k < end; k++) {
                        printf(" ");
                    }
                    if (j < level-1) {
                        if (last[j] == 0) {
                            printf("|");
                        } else {
                            printf(" ");
                        }
                    }
                }

                if (i == root->num_children - 1) {
                    printf("`-");
                } else {
                    printf("|-");
                }

                print_tree_1(root->childrens[i], show_pids, level, col + 3);
            }
        }

        // after printing all children , we need to reset the level to the previous level
        level -= 1;
    }
}