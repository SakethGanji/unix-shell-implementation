#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 50

int main(int argc, char *argv[]) {

    char* inputString = NULL;
    size_t len = 0;

    char* inputToken;
    char* inputSavePtr;

    char* args[MAX_ARGS];

    while (1) {
        printf("sish> ");

        if (getline(&inputString, &len, stdin) == -1) {
            perror("getline");
            exit(EXIT_FAILURE);
        }
        if (inputString[strlen(inputString) - 1] == '\n') {
            inputString[strlen(inputString) - 1] = '\0';
        }

        inputToken = strtok_r(inputString, "  \n", &inputSavePtr);
        int argsIndex = 0;
        while (inputToken != NULL) {
            args[argsIndex] = inputToken;
            inputToken = strtok_r(NULL, "  \n", &inputSavePtr);
            argsIndex++;
        }
        args[argsIndex] = NULL;

        if (args[0] == NULL) {
            continue;
        }
        else if (strcmp(args[0], "exit") == 0) {
            free(inputString);
            exit(EXIT_SUCCESS);
        }

        pid_t pid = fork();
        if (pid < 0) {
            fprintf(stderr, "sish: fork 1\n");
        }
        else if (pid == 0) {
            execvp(args[0], args);
            fprintf(stderr, "sish: Command '%s' not found\n", args[0]);
            exit(1);
        }
        else {
            waitpid(pid, NULL, 0);
        }
    }
}

