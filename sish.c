#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 50
#define PATH_MAX 256

void readInput(char* inputString, size_t* len, char** args);
void tokenizeInput(char* inputString, char** args);
void runningExecutable(char** args);
void executeCommand(char** args);
void changeDirectory(char** args);


int main(int argc, char *argv[]) {

    char* inputString = NULL;
    size_t len = 0;

    char* args[MAX_ARGS];

    while (1) {
        readInput(inputString, &len, args);

        if (args[0] == NULL) {
            continue;
        }
        else if (strcmp(args[0], "exit") == 0) {
            free(inputString);
            exit(EXIT_SUCCESS);
        }
        else{
            executeCommand(args);
        }

    }
}

void printPrompt() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("sish:%s> ", cwd);
    }
    else {
        perror("getcwd()");
        printf("sish> ");
    }
    fflush(stdout);
}

void readInput(char* inputString, size_t* len, char** args) {
    printPrompt();
    //printf("sish> ");

    if (getline(&inputString, len, stdin) == -1) {
        perror("getline");
        exit(EXIT_FAILURE);
    }

    if (inputString[strlen(inputString) - 1] == '\n') {
        inputString[strlen(inputString) - 1] = '\0';
    }

    tokenizeInput(inputString, args);
}

void tokenizeInput(char* inputString, char** args) {
    char* inputToken;
    char* inputSavePtr;
    int argsIndex = 0;

    inputToken = strtok_r(inputString, " \t\n", &inputSavePtr);
    while (inputToken != NULL) {
        args[argsIndex] = inputToken;
        inputToken = strtok_r(NULL, " \t\n", &inputSavePtr);
        argsIndex++;
    }
    args[argsIndex] = NULL;
}

void runningExecutable(char** args) {
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "sish: fork 1\n");
    }
    else if (pid == 0) {
        execvp(args[0], args);
        fprintf(stderr, "sish: Command '%s' not found\n", args[0]);
        exit(EXIT_FAILURE);
    }
    else {
        waitpid(pid, NULL, 0);
    }
}

void executeCommand(char** args) {
    if (strcmp(args[0], "cd") == 0) {
        changeDirectory(args);
    } else {
        runningExecutable(args);
    }
}

void changeDirectory(char** args) {
    if (args[1] == NULL || strcmp(args[1], "~") == 0) {
        chdir(getenv("HOME"));
    } else {
        if (chdir(args[1]) == -1) {
            perror("sish: cd");
        }
    }
}