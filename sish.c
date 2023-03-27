#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define ARGS_MAX 50
#define PATH_MAX 256
#define HISTORY_SIZE_MAX 100

void readInput(char** inputString, size_t* len, char** historyCommands, int* historyCount);
void tokenizeInput(char* inputString, char*** args, int* pipeCount);
int executeCommand(char* inputString, char*** args, int* pipeCount, char** historyCommands, int* historyCount);
void executableCommands(char*** args, int pipeCount);
void changeDirectory(char*** args);
void addHistoryCommands(char* inputString, char** historyCommands, int* historyCount);
void history(char*** args, int* pipeCount, char** historyCommands, int* historyCount);
void executeHistoryOffset(int offset, int* pipeCount, char** historyCommands, int* historyCount);


//runs an infinite loop that reads the input from the user, executes the command, and repeats.
int main() {
    char* inputString = NULL;
    size_t len = 0;

    char* historyCommands[HISTORY_SIZE_MAX];
    int historyCount = 0;

    char** args[ARGS_MAX + 1] = { NULL };
    int pipeCount = 0;

    while (1) {
        readInput(&inputString, &len, historyCommands, &historyCount);

        int commandStatus = executeCommand(inputString, args, &pipeCount, historyCommands, &historyCount);

        if (commandStatus == 1) { //if the command is empty,
            continue;
        }
        else if (commandStatus == 2) { //if the command is exit,
            free(inputString);
            exit(EXIT_SUCCESS);
        }
    }
}

//prints the current working directory
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

//reads the input string from the user, stores it in inputString. It adds the command to the historyCommands array
void readInput(char** inputString, size_t* len, char** historyCommands, int* historyCount) {
    printf("sish> ");
    //printPath();

    if (getline(inputString, len, stdin) == -1) { //if the input is empty
        perror("getline");
        exit(EXIT_FAILURE);
    }

    if ((*inputString)[strlen(*inputString) - 1] == '\n') {
        (*inputString)[strlen(*inputString) - 1] = '\0';
    }
    addHistoryCommands(*inputString, historyCommands, historyCount );
}

//tokenizes the input string into multiple arguments, separated by whitespace or pipes. each argument in the args array and counts the number of pipes in the input
void tokenizeInput(char* inputString, char*** args, int* pipeCount) {
    char* inputToken;
    char* inputSavePtr;
    int argsIndex = 0;
    int pipeIndex = 0;

    char* pipeToken;
    char* pipeSavePtr;

    pipeToken = strtok_r(inputString, "|", &pipeSavePtr);
    while (pipeToken != NULL) { //for each pipe
        args[pipeIndex] = malloc(sizeof(char*) * (ARGS_MAX + 1));

        if (args[pipeIndex] == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        argsIndex = 0;
        inputToken = strtok_r(pipeToken, " \t\n", &inputSavePtr);
        while (inputToken != NULL) { //for each argument - " "
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

//executes the command in args and calls functions to handle built-in commands if match
int executeCommand(char* inputString, char*** args, int* pipeCount, char** historyCommands, int* historyCount) {
    tokenizeInput(inputString, args, pipeCount);

    if (args[0] == NULL || args[0][0] == NULL) { //if the command is empty
        return 1;
    }
    else if (strcmp(args[0][0], "exit") == 0) { //if the command is exit
        return 2;
    }
    else if (strcmp(args[0][0], "cd") == 0) { //if the command is cd
        changeDirectory(args);
    }
    else if (strcmp(args[0][0], "history") == 0) { //if the command is history
        history(args, pipeCount, historyCommands, historyCount);
    }
    else { //if the command is not built-in
        executableCommands(args, *pipeCount);
    }


    return 0;
}

//changes working directory to the one specified in the args array
void changeDirectory(char*** args) {
    if (args[0][1] == NULL){
        fprintf(stderr, "sish: no argument given\n");
    }
    else if ( strcmp(args[0][1], "~") == 0) { //if no directory is specified, change to home directory
        chdir(getenv("HOME"));
    } else {
        if (chdir(args[0][1]) == -1) {
            perror("sish: cd");
        }
    }
}

// the input string to the historyCommands array and removes the oldest command if the array is full.
void addHistoryCommands(char* inputString, char** historyCommands, int* historyCount) {
    if (*historyCount > HISTORY_SIZE_MAX) { //if the history is full
        free(historyCommands[0]);
        for (int i = 1; i < HISTORY_SIZE_MAX; i++) {
            historyCommands[i-1] = strdup(historyCommands[i]);
        }
        (*historyCount)--;
    }
    historyCommands[*historyCount] = strdup(inputString); //adds the input string to the historyCommands array
    (*historyCount)++;
}

//prints the history of commands. If the first argument is -c, it clears the history. If an offset is specified, it prints the command at that offset.
void history(char*** args, int* pipeCount, char** historyCommands, int* historyCount) {
    if (args[0][1] != NULL && strcmp(args[0][1], "-c") == 0) { //if the first argument is -c
        for (int i = 0; i < *historyCount; i++) {
            free(historyCommands[i]);
        }
        *historyCount = 0;
    } else if (args[0][1] != NULL) { //if an offset is specified
        int offset = atoi(args[0][1]);
        executeHistoryOffset(offset, pipeCount, historyCommands, historyCount);
    } else {
        for (int i = 0; i < *historyCount; i++) { //if no arguments are specified
            printf("%d %s\n", i, historyCommands[i]);
        }
    }
}

//executes the command at the specified offset in the historyCommands array
void executeHistoryOffset(int offset, int* pipeCount, char** historyCommands, int* historyCount) {
    char* inputString;
    char** args[ARGS_MAX + 1] = { NULL };
    if (offset >= 0 && offset < *historyCount) {
        inputString = strdup(historyCommands[offset]);
        executeCommand(inputString, args, pipeCount, historyCommands, historyCount);
        free(inputString);
    } else {
        printf("sish: history: invalid offset\n");
    }
}

//runs multiple commands in a pipeline
void executableCommands(char*** args, int pipeCount) {
    int fds[pipeCount][2];

    for (int i = 0; i < pipeCount; i++) {
        if (pipe(fds[i]) == -1) { // create pipes
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    int pid;
    for (int i = 0; i <= pipeCount; i++) {

        if ((pid = fork()) == -1) { // create child process
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0) { // child process
            if (i > 0) { // if not first command
                if (dup2(fds[i-1][0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            if (i < pipeCount) { // if not last command
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
            // execute command
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

