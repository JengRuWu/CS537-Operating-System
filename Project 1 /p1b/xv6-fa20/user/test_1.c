#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[]) {
    int i;
    int x1, x2, x3, x4, x5;
    char hello[10] = "Hello ";
    char world[10] = "World!\n";
    x1 = getwritecount();
    x2 = getwritecount();
    (void) write(1, hello, 6);
    x3 = getwritecount();
    x4 = getwritecount();

    for (i = 0; i < 7; i++) {
        (void) write(1, world + i, 1);
    }
    x5 = getwritecount();
    printf(1, "\nXV6_TEST_OUTPUT %d %d %d %d %d\n", x1, x2, x3, x4, x5);
    exit();
}
