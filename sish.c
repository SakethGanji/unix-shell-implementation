#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define ARGS_MAX 50
#define PATH_MAX 256
#define HISTORY_SIZE_MAX 100

void readInput(char** inputString, size_t* len);
void tokenizeInput(char* inputString, char*** args, int* pipeCount);
void changeDirectory(char*** args);
int executeCommand(char* inputString, char*** args, int* pipeCount);
void executableCommands(char*** args, int pipeCount);
void addHistoryCommands(char* inputString);
void history(char*** args, int* pipeCount);
void executeHistoryOffset(int offset, int* pipeCount);
void clearHistory();
void printHistory();

char* historyCommands[HISTORY_SIZE_MAX];
int historyCount = 0;

int main() {

    char* inputString = NULL;
    size_t len = 0;
    char** args[ARGS_MAX + 1] = { NULL };
    int pipeCount = 0;

    while (1) {
        readInput(&inputString, &len);

        int commandStatus = executeCommand(inputString, args, &pipeCount);

        if (commandStatus == 1) {
            continue;
        }
        else if (commandStatus == 2) {
            free(inputString);
            exit(EXIT_SUCCESS);
        }
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

void readInput(char** inputString, size_t* len) {
    //printf("sish> ");
    printPath();

    if (getline(inputString, len, stdin) == -1) {
        perror("getline");
        exit(EXIT_FAILURE);
    }

    if ((*inputString)[strlen(*inputString) - 1] == '\n') {
        (*inputString)[strlen(*inputString) - 1] = '\0';
    }
    addHistoryCommands(*inputString);
}

void tokenizeInput(char* inputString, char*** args, int* pipeCount) {
    char* inputToken;
    char* inputSavePtr;
    int argsIndex = 0;
    int pipeIndex = 0;

    char* pipeToken;
    char* pipeSavePtr;

    pipeToken = strtok_r(inputString, "|", &pipeSavePtr);
    while (pipeToken != NULL) {
        args[pipeIndex] = malloc(sizeof(char*) * (ARGS_MAX + 1));

        if (args[pipeIndex] == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        argsIndex = 0;
        inputToken = strtok_r(pipeToken, " \t\n", &inputSavePtr);
        while (inputToken != NULL) {
            args[pipeIndex][argsIndex] = inputToken;
            inputToken = strtok_r(NULL, " \t\n", &inputSavePtr);
            argsIndex++;
        }
        args[pipeIndex][argsIndex] = NULL;
        pipeIndex++;

        pipeToken = strtok_r(NULL, "|", &pipeSavePtr);
    }

    *pipeCount = pipeIndex - 1;
}

int executeCommand(char* inputString, char*** args, int* pipeCount) {
    tokenizeInput(inputString, args, pipeCount);

    if (args[0] == NULL || args[0][0] == NULL) {
        return 1;
    }
    else if (strcmp(args[0][0], "exit") == 0) {
        return 2;
    }
    else if (strcmp(args[0][0], "cd") == 0) {
        changeDirectory(args);
    }
    else if (strcmp(args[0][0], "history") == 0) {
        history(args, pipeCount);
    }
    else {
        executableCommands(args, *pipeCount);
    }

    return 0;
}


void changeDirectory(char*** args) {
    if (args[0][1] == NULL || strcmp(args[0][1], "~") == 0) {
        chdir(getenv("HOME"));
    } else {
        if (chdir(args[0][1]) == -1) {
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

void history(char*** args, int* pipeCount) {
    if (args[0][1] != NULL && strcmp(args[0][1], "-c") == 0) {
        clearHistory();
    } else if (args[0][1] != NULL) {
        int offset = atoi(args[0][1]);
        executeHistoryOffset(offset, pipeCount);
    } else {
        printHistory();
    }
}

void executeHistoryOffset(int offset, int* pipeCount) {
    char* inputString;
    char** args[ARGS_MAX + 1] = { NULL };
    if (offset >= 0 && offset < historyCount) {
        inputString = strdup(historyCommands[offset]);
        executeCommand(inputString, args, pipeCount);
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

void executableCommands(char*** args, int pipeCount) {
    int fds[pipeCount][2];

    for (int i = 0; i < pipeCount; i++) {
        if (pipe(fds[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    int pid;
    for (int i = 0; i <= pipeCount; i++) {

        if ((pid = fork()) == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0) {
            if (i > 0) {
                if (dup2(fds[i-1][0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            if (i < pipeCount) {
                if (dup2(fds[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            // close child process pipes
            for (int j = 0; j < pipeCount; j++) {
                if (close(fds[j][0]) == -1) {
                    perror("close");
                    exit(EXIT_FAILURE);
                }
                if (close(fds[j][1]) == -1) {
                    perror("close");
                    exit(EXIT_FAILURE);
                }
            }

            if (execvp(args[i][0], args[i]) == -1) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        }
    }

    // close parent process pipes
    for (int i = 0; i < pipeCount; i++) {
        if (close(fds[i][0]) == -1) {
            perror("close");
            exit(EXIT_FAILURE);
        }
        if (close(fds[i][1]) == -1) {
            perror("close");
            exit(EXIT_FAILURE);
        }
    }

    // wait child processes
    for (int i = 0; i <= pipeCount; i++) {
        int status;
        if (wait(&status) == -1) {
            perror("wait");
            exit(EXIT_FAILURE);
        }

    }
}

