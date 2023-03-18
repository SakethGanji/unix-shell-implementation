#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define ARGS_MAX 50


void readInput(char** inputString, size_t* len);
void tokenizeInput(char* inputString, char*** args, int* pipeCount);
void executeCommands(char*** args, int pipeCount);


int main() {

    char* inputString = NULL;
    size_t len = 0;
    char** args[ARGS_MAX + 1] = { NULL };
    int pipeCount = 0;

    while (1) {
        readInput(&inputString, &len);

        tokenizeInput(inputString, args, &pipeCount);

        if (args[0] == NULL || args[0][0] == NULL) {
            continue;
        }
        else if (strcmp(args[0][0], "exit") == 0) {
            free(inputString);
            exit(EXIT_SUCCESS);
        }

/*        for (int i = 0; i <= pipeCount; i++) {
            printf("Pipe section %d:\n", i);
            for (int j = 0; args[i][j] != NULL; j++) {
                printf("\targs[%d][%d]: %s\n", i, j, args[i][j]);
            }
        }*/

        executeCommands(args, pipeCount);
    }
}

void readInput(char** inputString, size_t* len) {
    printf("sish> ");

    if (getline(inputString, len, stdin) == -1) {
        perror("getline");
        exit(EXIT_FAILURE);
    }

    if ((*inputString)[strlen(*inputString) - 1] == '\n') {
        (*inputString)[strlen(*inputString) - 1] = '\0';
    }
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

void executeCommands(char*** args, int pipeCount) {
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
        } else if (pid == 0) {
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
