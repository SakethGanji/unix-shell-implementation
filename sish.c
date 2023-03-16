#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define ARGS_MAX 50
#define PATH_MAX 256


void readInput(char** inputString, size_t* len);
void printPath();
void tokenizeInput(char* inputString, char** args);
void runningExecutable(char** args);
int executeCommand(char* inputString, char** args);


int main() {

    char* inputString = NULL;
    size_t len = 0;
    char* args[ARGS_MAX];

    while (1) {
        readInput(&inputString, &len);

        int commandStatus = executeCommand(inputString, args);

        if (commandStatus == 1) {
            continue;
        }
        else if (commandStatus == 2) {
            free(inputString);
            exit(EXIT_SUCCESS);
        }

    }
}

void readInput(char** inputString, size_t* len) {
    printPath();
    //printf("sish> ");

    if (getline(inputString, len, stdin) == -1) {
        perror("getline");
        exit(EXIT_FAILURE);
    }

    if ((*inputString)[strlen(*inputString) - 1] == '\n') {
        (*inputString)[strlen(*inputString) - 1] = '\0';
    }
}

void printPath() {
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

int executeCommand(char* inputString, char **args) {
    //addHistoryCommands(inputString);
    tokenizeInput(inputString, args);

    if (args[0] == NULL) {
        return 1;
    }
    else if (strcmp(args[0], "exit") == 0) {
        return 2;
    }
    else {
        runningExecutable(args);
    }
    return 0;
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
