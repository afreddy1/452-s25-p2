#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "../src/lab.h"
#include <errno.h>

#define lab_MINOR_VERSION 0  
#define lab_MAJOR_VERSION 1  

void parse_args(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "v")) != -1) {
        switch(opt) {
            case 'v':
                // prints shell version and exits
                printf("Shell Version: %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);
                exit(0);
            default:
                // prints usage message for invalid arguments
                fprintf(stderr, "Usage: %s [-v]\n", argv[0]);
                exit(1);
        }
    }
}

char *get_prompt(const char *env) {
    const char *prompt_value = getenv(env);

    if(prompt_value == NULL || strlen(prompt_value) == 0) {
        prompt_value = "shell>";
    }

    // allocates memeory for the prompt string
    char *result = malloc(strlen(prompt_value) + 1);

    if (result == NULL) {
        perror("malloc failed");
        exit (1);
    }

    strcpy(result, prompt_value);
    return result; 
}

char *trim_white(char *line) {
    if(line == NULL) return NULL;

    while (isspace((unsigned char)*line)) line++; //moves pointer forward to skip leading whitespace

    if(*line == '\0') return line; // returns empty string if oonly whitespace

    char *end = line + strlen(line) - 1;

    while( end > line && isspace((unsigned char)*end)) end--;

    *(end + 1) = '\0'; // marks the end of the string
    
    return line;
}

void sh_init(struct shell *sh) {
    sh->shell_terminal = STDIN_FILENO;
    sh->shell_is_interactive = isatty(sh->shell_terminal);
    sh->shell_pgid = getpid();
    setpgid(sh->shell_terminal, sh->shell_pgid); 
    tcsetpgrp(sh->shell_terminal, sh->shell_pgid);
    sh->prompt = get_prompt("MY_PROMPT");
}


int change_dir(char **dir) {
    const char *path;

    // If no directory is provided, change to the user's home directory
    if (dir == NULL || dir[0] == NULL || dir[1] == NULL ) {
        path = getenv("HOME");
        if (path == NULL) {
            fprintf(stderr, "cd: HOME not set\n");
            return -1;

        }
    } else {
        path = dir[0];
    }

    //printf("DEBUG: Attempting to cd into: %s\n", path); /////debugggg statement

    if (chdir(path) != 0) {
        fprintf(stderr, "cd: %s: %s\n", path, strerror(errno));
        return -1;
    }

    //printf("DEBUG: Successfully changed directory\n"); ////debug statement
    return 0;
}

char **cmd_parse(char const *line) {
    if (line == NULL) return NULL;

    long max_args = sysconf(_SC_ARG_MAX);
    char **argv = malloc(sizeof(char *) * max_args + 1);
    if(!argv) {
        perror("malloc failed");
        exit(1);
    }
     int argc = 0;
     char *line_copy = strdup(line); // makes copy of the input line
     char *token = strtok(line_copy, " \t"); // tokenize input

     while (token && argc < max_args) { // stores in argv array
        argv[argc++] = strdup(token);
        token = strtok(NULL, " \t");
     }
     argv[argc] = NULL; //end of the list
     free(line_copy); // frees copied line
     
     return argv;  // returns list of words
}

bool do_builtin(struct shell *sh, char **argv) {
    if (argv[0] == NULL) {
        return false;
    }

    // 'exit' command
    if (strcmp(argv[0], "exit") == 0) {
            sh_destroy(sh);
            exit(0);
        }

    if (strcmp(argv[0], "cd") == 0) {
        char *dir = argv[1];

        if(dir == NULL) {
            dir = getenv("HOME"); // default to home directory if no argument
        }
        
        if (dir == NULL) {
            fprintf(stderr, "cd: HOME not set\n");
            return true;
        }

        if (change_dir(argv) == 0) {
            return true;
        } else {
            perror("cd failed");
            return true;
        }
    }

    
    return false;
}


void cmd_free(char **line) {
    if (line == NULL) return;

    for (int i =0; line[i] != NULL; i++) {
        free(line[i]);
    
    } 
    free(line);
}

void sh_destroy(struct shell *sh) {
    if (sh->prompt != NULL) {
        free(sh->prompt);

    }
}