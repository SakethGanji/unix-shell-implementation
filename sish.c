#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define ARGS_MAX 50
#define PATH_MAX 256
#define HISTORY_SIZE_MAX 100

char* historyCommands[HISTORY_SIZE_MAX];
int historyCount = 0;

void readInput(char** inputString, size_t* len, char** args);
void printPath();
void tokenizeInput(char* inputString, char** args);
void runningExecutable(char** args);
int executeCommand(char* inputString, char** args);
void changeDirectory(char** args);
void clearHistory();
void printHistory();
void executeHistoryOffset(int offset);
void history(char** args);
void addHistoryCommands(char* inputString);

int main(int argc, char *argv[]) {

    char* inputString = NULL;
    size_t len = 0;
    char* args[ARGS_MAX];

    while (1) {
        readInput(&inputString, &len, args);

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

void readInput(char** inputString, size_t* len, char** args) {
    printPath();
    //printf("sish> ");

    if (getline(inputString, len, stdin) == -1) {
        perror("getline");
        exit(EXIT_FAILURE);
    }

    if ((*inputString)[strlen(*inputString) - 1] == '\n') {
        (*inputString)[strlen(*inputString) - 1] = '\0';
    }
    addHistoryCommands(*inputString);
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
    else if (strcmp(args[0], "cd") == 0) {
        changeDirectory(args);
    }
    else if (strcmp(args[0], "history") == 0) {
        history(args);
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

void changeDirectory(char** args) {
    if (args[1] == NULL || strcmp(args[1], "~") == 0) {
        chdir(getenv("HOME"));
    } else {
        if (chdir(args[1]) == -1) {
            perror("sish: cd");
        }
    }
}

void addHistoryCommands(char* inputString) {
    if (historyCount == HISTORY_SIZE_MAX) {
        free(historyCommands[0]);
        for (int i = 1; i < HISTORY_SIZE_MAX; i++) {
            historyCommands[i-1] = historyCommands[i];
        }
        historyCount--;
    }
    historyCommands[historyCount] = strdup(inputString);
    historyCount++;
}

void history(char** args) {
    if (args[1] != NULL && strcmp(args[1], "-c") == 0) {
        clearHistory();
    } else if (args[1] != NULL) {
        int offset = atoi(args[1]);
        executeHistoryOffset(offset);
    } else {
        printHistory();
    }
}

void executeHistoryOffset(int offset) {
    char* inputString;
    char* args[ARGS_MAX];
    if (offset >= 0 && offset < historyCount) {
        inputString = strdup(historyCommands[offset]);
        executeCommand(inputString ,args);
        free(inputString);
    } else {
        printf("sish: history: invalid offset\n");
    }
}

void clearHistory() {
    for (int i = 0; i < historyCount; i++) {
        free(historyCommands[i]);
    }
    historyCount = 0;
}

void printHistory() {
    for (int i = 0; i < historyCount; i++) {
        printf("%d %s\n", i, historyCommands[i]);
    }
}