#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define WRSTR() write(fd, str, strlen(str))
#define NEWLINE(X) write(X, "\n", strlen("\n"))

int main(int argc, char *argv[], char *envp[])
{
    char str[128];

    // Открываю файл
    int fd = open("/lab5/log/output.txt", (O_WRONLY | O_CREAT | O_TRUNC | O_SYNC), 0666);
    if (fd == -1) return 1;

    // Записываю в файл argc и argv
    sprintf(str, "argc: %i\n\nargv:\n", argc); WRSTR();
    for (int i = 0; i < argc; i++) {
        sprintf(str, "$%i: ", i); WRSTR();
        write(fd, argv[i], strlen(argv[i]));
        NEWLINE(fd);
    }
    NEWLINE(fd);

    // Записываю в файл envp
    sprintf(str, "envp:\n"); WRSTR();
    for (int i = 0; envp[i] != NULL; i++) {
        write(fd, envp[i], strlen(envp[i]));
        NEWLINE(fd);
    }

    close(fd);

    sprintf(str, "Hello, World!\n"); write(1, str, strlen(str));

    return 0;
}
