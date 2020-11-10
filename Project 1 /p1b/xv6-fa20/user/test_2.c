#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[]) {
    int x1, x2, x3, x4, x5;
    int pipefd[2];
    pipe(pipefd);
    int pid = fork();
    if (!pid) {
        // child
        close(pipefd[0]);
        x1 = getwritecount();
        write(pipefd[1], &x1, sizeof(int));
        x2 = getwritecount();
        write(pipefd[1], &x2, sizeof(int));
        close(pipefd[1]);
        exit();
    }
    // parent
    close(pipefd[1]);
    read(pipefd[0], &x1, sizeof(int));
    read(pipefd[0], &x2, sizeof(int));
    close(pipefd[0]);

    char hello[10] = "Hello ";
    char world[10] = "World!\n";

    x3 = getwritecount();
    (void) write(1, hello, 6);

    x4 = getwritecount();
    int i;
    for (i = 0; i < 7; i++) {
        (void) write(1, world + i, 1);
    }
    x5 = getwritecount();
    printf(1, "\nXV6_TEST_OUTPUT %d %d %d %d %d\n", x1, x2, x3, x4, x5);
    exit();
}
